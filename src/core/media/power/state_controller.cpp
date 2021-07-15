/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "core/media/logging.h"

#include <core/media/power/state_controller.h>

#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>
#include <QWeakPointer>

#include <functional>

// We allow the tests to reconfigure this value
#ifndef DISPLAY_RELEASE_INTERVAL
#define DISPLAY_RELEASE_INTERVAL 4000
#endif

using namespace core::ubuntu::media::power;

namespace core {
namespace ubuntu {
namespace media {
namespace power {

uint qHash(SystemState state, uint seed) {
    return ::qHash(static_cast<uint>(state), seed);
}

class DisplayInterface {
public:
    DisplayInterface():
        m_interface(QStringLiteral("com.canonical.Unity.Screen"),
                    QStringLiteral("/com/canonical/Unity/Screen"),
                    QStringLiteral("com.canonical.Unity.Screen"),
                    QDBusConnection::systemBus()),
        m_requestId(-1)
    {
    }

    using Callback = std::function<void(void)>;

    void requestDisplayOn(const Callback &cb) {
        if (m_requestId >= 0) {
            MH_ERROR("Display ON was already requested!");
            return;
        }

        QDBusPendingCall call = m_interface.asyncCall("keepDisplayOn");
        auto watcher = new QDBusPendingCallWatcher(call);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         [this, cb](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<int32_t> reply = *watcher;
            if (reply.isError()) {
                MH_ERROR() << "Error requesting display ON:" << reply.error().message();
            } else {
                m_requestId = reply.value();
                cb();
            }
            watcher->deleteLater();
        });
    }

    void releaseDisplayOn(const Callback &cb) {
        if (m_requestId < 0) {
            MH_WARNING("Display ON was not requested!");
            return;
        }

        QDBusPendingCall call = m_interface.asyncCall("removeDisplayOnRequest", m_requestId);
        auto watcher = new QDBusPendingCallWatcher(call);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         [this, cb](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<void> reply = *watcher;
            if (reply.isError()) {
                MH_ERROR() << "Error releasing display ON:" << reply.error().message();
            } else {
                m_requestId = -1;
                cb();
            }
            watcher->deleteLater();
        });
    }

private:
    QDBusInterface m_interface;
    int32_t m_requestId;
};

class SystemStateInterface {
public:
    SystemStateInterface():
        m_interface(QStringLiteral("com.canonical.powerd"),
                    QStringLiteral("/com/canonical/powerd"),
                    QStringLiteral("com.canonical.powerd"),
                    QDBusConnection::systemBus())
    {
    }

    using Callback = std::function<void(SystemState state)>;

    void requestSystemState(SystemState state,
                            const Callback &cb)
    {
        MH_TRACE("");

        if (state == media::power::SystemState::suspend) {
            return;
        }

        QDBusPendingCall call =
            m_interface.asyncCall("requestSysState",
                                  QStringLiteral("media-hub-playback_lock"),
                                  static_cast<int32_t>(state));
        auto watcher = new QDBusPendingCallWatcher(call);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         [this, state, cb](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<QString> reply = *watcher;
            if (reply.isError()) {
                MH_ERROR() << "Error requesting system state:" << reply.error().message();
            } else {
                m_cookieStore.insert(state, reply.value());
                cb(state);
            }
            watcher->deleteLater();
        });
    }

    void releaseSystemState(media::power::SystemState state,
                            const Callback &cb)
    {
        if (state == media::power::SystemState::suspend) {
            return;
        }
        const auto i = m_cookieStore.find(state);
        if (i == m_cookieStore.end()) {
            return; // state was never requested
        }

        QDBusPendingCall call = m_interface.asyncCall("clearSysState", i.value());
        auto watcher = new QDBusPendingCallWatcher(call);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         [this, state, cb](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<void> reply = *watcher;
            if (reply.isError()) {
                MH_ERROR() << "Error releasing system state:" << reply.error().message();
            } else {
                m_cookieStore.remove(state);
                cb(state);
            }
            watcher->deleteLater();
        });
    }

private:
    QDBusInterface m_interface;
    QHash<SystemState, QString> m_cookieStore;
};

class StateControllerPrivate {
public:
    StateControllerPrivate(StateController *q);

private:
    friend class StateController;
    DisplayInterface m_display;
    int m_displayLockCount = 0;
    SystemStateInterface m_system;
    int m_systemLockCount = 0;
    QTimer m_displayReleaseTimer;
};

struct CreatableStateController: public StateController
{
    using StateController::StateController;
};

}}}} // namespace

StateControllerPrivate::StateControllerPrivate(StateController *q)
{
    m_displayReleaseTimer.setSingleShot(true);
    m_displayReleaseTimer.setTimerType(Qt::VeryCoarseTimer);
    m_displayReleaseTimer.setInterval(DISPLAY_RELEASE_INTERVAL);
    m_displayReleaseTimer.callOnTimeout(q, [this, q]() {
        auto emitReleaseSignal = [q]() {
            Q_EMIT q->displayOnReleased();
        };
        m_display.releaseDisplayOn(emitReleaseSignal);
    });
}

StateController::StateController():
    QObject(),
    d_ptr(new StateControllerPrivate(this))
{
}

StateController::~StateController() = default;

QSharedPointer<StateController> StateController::instance()
{
    static QWeakPointer<CreatableStateController> weakRef;

    QSharedPointer<CreatableStateController> strong = weakRef.toStrongRef();
    if (!strong) {
        strong = QSharedPointer<CreatableStateController>::create();
        weakRef = strong;
    }
    return strong;
}

void StateController::requestDisplayOn()
{
    Q_D(StateController);
    if (++d->m_displayLockCount == 1) {
        MH_INFO("Requesting new display wakelock.");
        d->m_display.requestDisplayOn([this]() {
            Q_EMIT displayOnAcquired();
        });
    }
}

void StateController::releaseDisplayOn()
{
    Q_D(StateController);
    if (--d->m_displayLockCount == 0) {
        MH_INFO("Clearing display wakelock.");
        d->m_displayReleaseTimer.start();
    }
}

void StateController::requestSystemState(SystemState state)
{
    Q_D(StateController);
    if (++d->m_systemLockCount == 1) {
        MH_INFO("Requesting new system wakelock.");
        d->m_system.requestSystemState(state, [this](SystemState state) {
            Q_EMIT systemStateAcquired(state);
        });
    }
}

void StateController::releaseSystemState(SystemState state)
{
    Q_D(StateController);
    if (--d->m_systemLockCount == 0) {
        MH_INFO("Clearing system wakelock.");
        d->m_system.releaseSystemState(state, [this](SystemState state) {
            Q_EMIT systemStateReleased(state);
        });
    }
}
