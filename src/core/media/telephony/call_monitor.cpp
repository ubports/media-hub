/*
 * Copyright (C) 2014 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *      Author: Justin McPherson <justin.mcpherson@canonical.com>
 */


#include "call_monitor.h"

#include "qtbridge.h"
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/SimpleCallObserver>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingAccount>

#include <list>
#include <mutex>

namespace media = core::ubuntu::media;

namespace
{
namespace impl
{
class TelepathyCallMonitor : public QObject
{
    Q_OBJECT
public:
    TelepathyCallMonitor(const Tp::AccountPtr& account):
        mAccount(account),
        mCallObserver(Tp::SimpleCallObserver::create(mAccount)) {
        connect(mCallObserver.data(), SIGNAL(callStarted(Tp::CallChannelPtr)), SIGNAL(offHook()));
        connect(mCallObserver.data(), SIGNAL(callEnded(Tp::CallChannelPtr,QString,QString)), SIGNAL(onHook()));
        connect(mCallObserver.data(), SIGNAL(streamedMediaCallStarted(Tp::StreamedMediaChannelPtr)), SIGNAL(offHook()));
        connect(mCallObserver.data(), SIGNAL(streamedMediaCallEnded(Tp::StreamedMediaChannelPtr,QString,QString)), SIGNAL(onHook()));
    }

Q_SIGNALS:
    void offHook();
    void onHook();

private:
    Tp::AccountPtr mAccount;
    Tp::SimpleCallObserverPtr mCallObserver;
};


class TelepathyBridge : public QObject
{
    Q_OBJECT
public:
    TelepathyBridge():
        QObject(0) {
        Tp::registerTypes();

        QTimer::singleShot(0, this, SLOT(accountManagerSetup()));
    }

    ~TelepathyBridge() {
        for (std::list<TelepathyCallMonitor*>::iterator it = mCallMonitors.begin();
            it != mCallMonitors.end();
            ++it) {
            delete *it;
        }
    }

    void on_change(const std::function<void(media::telephony::CallMonitor::State)>& func) {
        std::lock_guard<std::mutex> l(cb_lock);
        cb = func;
    }

private Q_SLOTS:
    void accountManagerSetup() {
        mAccountManager = Tp::AccountManager::create();
        connect(mAccountManager->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(accountManagerReady(Tp::PendingOperation*)));
    }

    void accountManagerReady(Tp::PendingOperation* operation) {
        if (operation->isError()) {
            std::cerr << "TelepathyBridge: Operation failed (accountManagerReady)" << std::endl;
            QTimer::singleShot(1000, this, SLOT(accountManagerSetup())); // again
            return;
        }

        Q_FOREACH(const Tp::AccountPtr& account, mAccountManager->allAccounts()) {
            connect(account->becomeReady(Tp::Account::FeatureCapabilities),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    SLOT(accountReady(Tp::PendingOperation*)));
        }

        connect(mAccountManager.data(), SIGNAL(newAccount(Tp::AccountPtr)), SLOT(newAccount(Tp::AccountPtr)));
    }

    void newAccount(const Tp::AccountPtr& account)
    {
        connect(account->becomeReady(Tp::Account::FeatureCapabilities),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(accountReady(Tp::PendingOperation*)));
    }

    void accountReady(Tp::PendingOperation* operation) {
        if (operation->isError()) {
            std::cerr << "TelepathyAccount: Operation failed (accountReady)" << std::endl;
            return;
        }

        Tp::PendingReady* pendingReady = qobject_cast<Tp::PendingReady*>(operation);
        if (pendingReady == 0) {
            std::cerr << "Rejecting account because could not understand ready status" << std::endl;
            return;
        }

        checkAndAddAccount(Tp::AccountPtr::qObjectCast(pendingReady->proxy()));
    }

    void offHook()
    {
        std::lock_guard<std::mutex> l(cb_lock);
        if (cb)
            cb(media::telephony::CallMonitor::State::OffHook);
    }

    void onHook()
    {
        std::lock_guard<std::mutex> l(cb_lock);
        if (cb)
            cb(media::telephony::CallMonitor::State::OnHook);
    }

private:
    std::mutex cb_lock;
    std::function<void (media::telephony::CallMonitor::State)>   cb;
    Tp::AccountManagerPtr mAccountManager;
    std::list<TelepathyCallMonitor*> mCallMonitors;

    void checkAndAddAccount(const Tp::AccountPtr& account)
    {
        Tp::ConnectionCapabilities caps = account->capabilities();

        // anything call like, perhaps overkill?
        if (caps.audioCalls() || caps.videoCalls() || caps.videoCallsWithAudio() || caps.streamedMediaCalls()) {
            auto tcm = new TelepathyCallMonitor(account);
            connect(tcm, SIGNAL(offHook()), SLOT(offHook()));
            connect(tcm, SIGNAL(onHook()), SLOT(onHook()));
            mCallMonitors.push_back(tcm);
        }
    }
};

struct CallMonitor : public media::telephony::CallMonitor
{
    CallMonitor()
    {
        try {
            std::thread([this]() {
                qt::core::world::build_and_run(0, nullptr, [this]() {
                    qt::core::world::enter_with_task([this]() {
                        mBridge = new TelepathyBridge();
                    });
                });
            }).detach();
        } catch(const std::system_error& error) {
            std::cerr << "exception(std::system_error) in CallMonitor thread start" << error.what() << std::endl;
        } catch(...) {
            std::cerr << "exception(...) in CallMonitor thread start" << std::endl;
        }

        mBridge->on_change([this](CallMonitor::State state)
        {
            call_state_changed(state);
        });
    }

    ~CallMonitor()
    {
        delete mBridge;
    }

    const core::Signal<media::telephony::CallMonitor::State>& on_call_state_changed() const
    {
        return call_state_changed;
    }

    TelepathyBridge *mBridge;
    core::Signal<media::telephony::CallMonitor::State> call_state_changed;
};
}
}

media::telephony::CallMonitor::Ptr media::telephony::make_platform_default_call_monitor()
{
    return std::make_shared<impl::CallMonitor>();
}

#include "call_monitor.moc"

