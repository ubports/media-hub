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

#include <core/signal.h>

#include <functional>
#include <memory>

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
struct CallMonitor
{
    // Save us some typing.
    typedef std::shared_ptr<CallMonitor> Ptr;

    // All known call states
    enum class State
    {
        // No current call.
        OffHook,
        // Call in progress.
        OnHook
    };

    CallMonitor() = default;
    virtual ~CallMonitor() = default;

    // Emitted whenever the current call state of the system changes.
    virtual const core::Signal<State>& on_call_state_changed() const = 0;
};

// Returns a platform default implementation of CallMonitor.
CallMonitor::Ptr make_platform_default_call_monitor();
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_TELEPHONY_CALL_MONITOR_H_
