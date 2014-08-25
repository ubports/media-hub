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
#ifndef CORE_UBUNTU_MEDIA_PLAYER_CLIENT_CONFIGURATION_H_
#define CORE_UBUNTU_MEDIA_PLAYER_CLIENT_CONFIGURATION_H_

#include <core/media/player.h>

#include <core/dbus/bus.h>
#include <core/dbus/object.h>

struct core::ubuntu::media::Player::Configuration
{
    std::shared_ptr<core::dbus::Bus> bus;
    std::shared_ptr<core::dbus::Object> session;
};

#endif // CORE_UBUNTU_MEDIA_PLAYER_CLIENT_CONFIGURATION_H_
