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

#include <core/media/external_services.h>

#include <core/property.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace power
{
// Enumerates all known power states of a display.
enum class DisplayState
{
    // The display is off.
    off = 0,
    // The display is on.
    on = 1
};

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

// Interface that enables observation of the system power state.
struct StateController
{
    // To save us some typing.
    typedef std::shared_ptr<StateController> Ptr;

    // When acquired, ensures that the system stays active,
    // and decreases the reference count when released.
    template<typename State>
    struct Lock
    {
        // To save us some typing.
        typedef std::shared_ptr<Lock> Ptr;

        Lock() = default;
        virtual ~Lock() = default;

        // Informs the system that the caller would like
        // the system to stay active.
        virtual void request_acquire(State state) = 0;
        // Informs the system that the caller does not
        // require the system to stay active anymore.
        virtual void request_release(State state) = 0;

        // Emitted whenever the acquire request completes.
        virtual const core::Signal<State>& acquired() const = 0;
        // Emitted whenever the release request completes.
        virtual const core::Signal<State>& released() const = 0;
    };

    StateController() = default;
    virtual ~StateController() = default;

    // Returns a power::StateController::Lock<DisplayState> instance.
    virtual Lock<DisplayState>::Ptr display_state_lock() = 0;
    // Returns a power::StateController::Lock<SystemState> instance.
    virtual Lock<SystemState>::Ptr system_state_lock() = 0;
};

// Creates a StateController instance that connects to the platform default
// services to control system and display power states.
StateController::Ptr make_platform_default_state_controller(core::ubuntu::media::helper::ExternalServices&);
}
}
}
}
#endif // CORE_UBUNTU_MEDIA_POWER_STATE_CONTROLLER_H_
