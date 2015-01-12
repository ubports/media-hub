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


namespace {
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

    void on_change(const std::function<void(CallMonitor::State)>& func) {
        std::lock_guard<std::mutex> l(cb_lock);
        cb = func;
    }

private Q_SLOTS:
    void accountManagerSetup() {
        mAccountManager = Tp::AccountManager::create(Tp::AccountFactory::create(QDBusConnection::sessionBus(),
                                                                                Tp::Account::FeatureCore),
                                                     Tp::ConnectionFactory::create(QDBusConnection::sessionBus(),
                                                                                   Tp::Connection::FeatureCore));
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
            cb(CallMonitor::OffHook);
    }

    void onHook()
    {
        std::lock_guard<std::mutex> l(cb_lock);
        if (cb)
            cb(CallMonitor::OnHook);
    }

private:
    std::mutex cb_lock;
    std::function<void (CallMonitor::State)>   cb;
    Tp::AccountManagerPtr mAccountManager;
    std::list<TelepathyCallMonitor*> mCallMonitors;

    void checkAndAddAccount(const Tp::AccountPtr& account)
    {
        Tp::ConnectionCapabilities caps = account->capabilities();
        // TODO: Later on we will need to filter for the right capabilities, and also allow dynamic account detection
        // Don't check caps for now as a workaround for https://bugs.launchpad.net/ubuntu/+source/media-hub/+bug/1409125
        // at least until we are able to find out the root cause of it (check rev 107 for the caps check)
        auto tcm = new TelepathyCallMonitor(account);
        connect(tcm, SIGNAL(offHook()), SLOT(offHook()));
        connect(tcm, SIGNAL(onHook()), SLOT(onHook()));
        mCallMonitors.push_back(tcm);
    }
};
}

class CallMonitorPrivate
{
public:
    CallMonitorPrivate() {
        mBridge = nullptr;
        try {
            std::thread([this]() {
                qt::core::world::build_and_run(0, nullptr, [this]() {
                    qt::core::world::enter_with_task([this]() {
                        std::cout << "CallMonitor: Creating TelepathyBridge" << std::endl;
                        mBridge = new TelepathyBridge();
                        cv.notify_all();
                    });
                });
            }).detach();
        } catch(const std::system_error& error) {
            std::cerr << "exception(std::system_error) in CallMonitor thread start" << error.what() << std::endl;
        } catch(...) {
            std::cerr << "exception(...) in CallMonitor thread start" << std::endl;
        }
        // Wait until telepathy bridge is set, so we can hook up the change signals
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait_for(lck, std::chrono::seconds(3));
    }

    ~CallMonitorPrivate() {
        qt::core::world::destroy();
    }

    TelepathyBridge *mBridge;

private:
    std::mutex mtx;
    std::condition_variable cv;
};


CallMonitor::CallMonitor():
    d(new CallMonitorPrivate)
{
}

CallMonitor::~CallMonitor()
{
    delete d->mBridge;
    delete d;
}

void CallMonitor::on_change(const std::function<void(CallMonitor::State)>& func)
{
    if (d->mBridge != nullptr) {
        std::cout << "CallMonitor: Setting up callback for TelepathyBridge on_change" << std::endl;
        d->mBridge->on_change(func);
    } else
        std::cerr << "TelepathyBridge: Failed to hook on_change signal, bridge not yet set" << std::endl;
}

#include "call_monitor.moc"

