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

#include "core/media/logging.h"

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/SimpleCallObserver>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/PendingAccount>

namespace media = core::ubuntu::media;

using namespace media::telephony;

namespace core {
namespace ubuntu {
namespace media {

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

namespace telephony {

class CallMonitorPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(CallMonitor)

public:
    CallMonitorPrivate(CallMonitor *q):
        QObject(0),
        m_callState(CallMonitor::State::OffHook),
        q_ptr(q)
    {
        Tp::registerTypes();

        QTimer::singleShot(0, this, SLOT(accountManagerSetup()));
    }

    ~CallMonitorPrivate() {
        for (std::list<TelepathyCallMonitor*>::iterator it = mCallMonitors.begin();
            it != mCallMonitors.end();
            ++it) {
            delete *it;
        }
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
        static uint8_t retries = 0;
        if (operation->isError()) {
            MH_ERROR("TelepathyBridge: Operation failed (accountManagerReady)");
            if (retries < 10) {
                QTimer::singleShot(1000, this, SLOT(accountManagerSetup())); // again
                ++retries;
            }
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
            MH_ERROR("TelepathyAccount: Operation failed (accountReady)");
            return;
        }

        Tp::PendingReady* pendingReady = qobject_cast<Tp::PendingReady*>(operation);
        if (pendingReady == 0) {
            MH_ERROR("Rejecting account because could not understand ready status");
            return;
        }

        checkAndAddAccount(Tp::AccountPtr::qObjectCast(pendingReady->proxy()));
    }

    void offHook()
    {
        Q_Q(CallMonitor);
        m_callState = media::telephony::CallMonitor::State::OffHook;
        Q_EMIT q->callStateChanged();
    }

    void onHook()
    {
        Q_Q(CallMonitor);
        m_callState = media::telephony::CallMonitor::State::OnHook;
        Q_EMIT q->callStateChanged();
    }

private:
    Tp::AccountManagerPtr mAccountManager;
    std::list<TelepathyCallMonitor*> mCallMonitors;
    media::telephony::CallMonitor::State m_callState;
    CallMonitor *q_ptr;

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

} // namespace
}}} // namespace

CallMonitor::CallMonitor(QObject *parent):
    QObject(parent),
    d_ptr(new CallMonitorPrivate(this))
{
}

CallMonitor::~CallMonitor() = default;

CallMonitor::State CallMonitor::callState() const
{
    Q_D(const CallMonitor);
    return d->m_callState;
}

#include "call_monitor.moc"

