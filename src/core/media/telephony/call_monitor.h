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


#ifndef CORE_UBUNTU_MEDIA_TELEPHONY_CALL_MONITOR_H_
#define CORE_UBUNTU_MEDIA_TELEPHONY_CALL_MONITOR_H_

#include <QObject>
#include <QScopedPointer>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace telephony
{
// CallMonitor models the ability to observe and react
// to changes of the overall call state of the system.
class CallMonitorPrivate;
class CallMonitor: public QObject
{
    Q_OBJECT

public:
    // All known call states
    enum class State
    {
        // No current call.
        OffHook,
        // Call in progress.
        OnHook
    };

    CallMonitor(QObject *parent = nullptr);
    virtual ~CallMonitor();

    State callState() const;

Q_SIGNALS:
    void callStateChanged();

private:
    QScopedPointer<CallMonitorPrivate> d_ptr;
    Q_DECLARE_PRIVATE(CallMonitor)
};

}
}
}
}

#endif // CORE_UBUNTU_MEDIA_TELEPHONY_CALL_MONITOR_H_
