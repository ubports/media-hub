/*
 * Copyright (C) 2013-2014 Canonical Ltd
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
 * Author: Jim Hodapp <jim.hodapp@canonical.com>
 */

#ifndef CORE_UBUNTU_MEDIA_APPARMOR_DBUS_H_
#define CORE_UBUNTU_MEDIA_APPARMOR_DBUS_H_

#include <core/dbus/bus.h>
#include <core/dbus/macros.h>
#include <core/dbus/object.h>
#include <core/dbus/service.h>

#include <string>
#include <chrono>

// TODO(tvoss): This really should live in trust-store, providing a straightforward
// way for parties involved in managing trust relationships to query peers' apparmor
// profiles. Please see https://bugs.launchpad.net/trust-store/+bug/1350736 for the
// related bug
namespace org
{
namespace freedesktop
{
namespace dbus
{
struct DBus
{
    static const std::string& name()
    {
        static const std::string s = "org.freedesktop.DBus";
        return s;
    }

    // Gets the AppArmor confinement string associated with the unique connection name. If
    // D-Bus is not performing AppArmor mediation, the
    // org.freedesktop.DBus.Error.AppArmorSecurityContextUnknown error is returned.
    DBUS_CPP_METHOD_DEF(GetConnectionAppArmorSecurityContext, DBus)

    struct Stub
    {
        // Creates a new stub instance for the given object to access
        // DBus functionality.
        Stub(const core::dbus::Object::Ptr& object) : object{object}
        {
        }

        // Creates a new stub instance for the given bus connection
        Stub(const core::dbus::Bus::Ptr& bus)
            : object
              {
                  core::dbus::Service::use_service<org::freedesktop::dbus::DBus>(bus)
                      ->object_for_path(core::dbus::types::ObjectPath{"/org/freedesktop/DBus"})
              }
        {
        }

        // Gets the AppArmor confinement string associated with the unique connection name. If
        // D-Bus is not performing AppArmor mediation, the
        // org.freedesktop.DBus.Error.AppArmorSecurityContextUnknown error is returned.
        //
        // Invokes the given handler on completion.
        void get_connection_app_armor_security_async(
                    const std::string& name,
                    std::function<void(const std::string&)> handler)
        {
            object->invoke_method_asynchronously_with_callback<GetConnectionAppArmorSecurityContext, std::string>(
                        [handler](const core::dbus::Result<std::string>& result)
                        {
                            if (not result.is_error()) handler(result.value());
                        }, name);
        }

        core::dbus::Object::Ptr object;
    };
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_APPARMOR_DBUS_H_
