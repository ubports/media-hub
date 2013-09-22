/*
 * Copyright © 2013 Canonical Ltd.
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

#include "the_session_bus.h"

#include <org/freedesktop/dbus/asio/executor.h>

namespace
{
std::once_flag once;
}

org::freedesktop::dbus::Bus::Ptr the_session_bus()
{
    static org::freedesktop::dbus::Bus::Ptr bus
            = std::make_shared<org::freedesktop::dbus::Bus>(
                org::freedesktop::dbus::WellKnownBus::session);
    static org::freedesktop::dbus::Executor::Ptr executor
            = std::make_shared<org::freedesktop::dbus::asio::Executor>(bus);

    std::call_once(once, [](){bus->install_executor(executor);});

    return bus;
}
