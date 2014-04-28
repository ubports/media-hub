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
 *      Author: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#ifndef APPARMOR_H_DBUS_
#define APPARMOR_H_DBUS_

#include <string>
#include <chrono>

namespace core
{

struct Apparmor
{
    static std::string& name()
    {
        static std::string s = "org.freedesktop.DBus";
        return s;
    }

    struct getConnectionAppArmorSecurityContext
    {
        static std::string name()
        {
            static std::string s = "GetConnectionAppArmorSecurityContext";
            return s;
        }

        static const std::chrono::milliseconds default_timeout() { return std::chrono::seconds{1}; }

        typedef Apparmor Interface;
    };
};
}

#endif
