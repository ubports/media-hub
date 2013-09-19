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

#ifndef THE_SESSION_BUS_H_
#define THE_SESSION_BUS_H_

#include <org/freedesktop/dbus/bus.h>

inline org::freedesktop::dbus::Bus::Ptr the_session_bus()
{
    static org::freedesktop::dbus::Bus::Ptr bus
            = std::make_shared<org::freedesktop::dbus::Bus>(org::freedesktop::dbus::WellKnownBus::session);
    return bus;
}

#endif // THE_SESSION_BUS_H_
