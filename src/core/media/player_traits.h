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

#ifndef PLAYER_TRAITS_H_
#define PLAYER_TRAITS_H_

#include <org/freedesktop/dbus/traits/service.h>

namespace org
{
namespace freedesktop
{
namespace dbus
{
namespace traits
{
template<>
struct Service<core::ubuntu::media::Player>
{
    static const std::string& interface_name()
    {
        static const std::string s
        {
            "core.ubuntu.media.Service.Player"
        };
        return s;
    }
};
}
}
}
}


#endif // PLAYER_TRAITS_H_
