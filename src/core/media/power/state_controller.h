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
#ifndef CORE_UBUNTU_MEDIA_POWER_STATE_CONTROLLER_H_
#define CORE_UBUNTU_MEDIA_POWER_STATE_CONTROLLER_H_

#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace power
{
Q_NAMESPACE

// Enumerates known power states of the system.
enum class SystemState
{
    // Note that callers will be notified of suspend state changes
    // but may not request this state.
    suspend = 0,
    // The Active state will prevent system suspend
    active = 1,
    // Substate of Active with disabled proximity based blanking
    blank_on_proximity = 2
};
Q_ENUM_NS(SystemState)

// Interface that enables observation of the system power state.
class StateControllerPrivate;
class StateController: public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<StateController> Ptr;
    static QSharedPointer<StateController> instance();
    ~StateController();

    void requestDisplayOn();
    void releaseDisplayOn();

    void requestSystemState(SystemState state);
    void releaseSystemState(SystemState state);

Q_SIGNALS:
    void displayOnAcquired();
    void displayOnReleased();

    void systemStateAcquired(SystemState state);
    void systemStateReleased(SystemState state);

protected:
    StateController();

private:
    Q_DECLARE_PRIVATE(StateController)
    QScopedPointer<StateControllerPrivate> d_ptr;
};

}
}
}
}
#endif // CORE_UBUNTU_MEDIA_POWER_STATE_CONTROLLER_H_
