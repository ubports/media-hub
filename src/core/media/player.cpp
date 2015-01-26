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

#include <core/media/player.h>

#include "player_configuration.h"

namespace media = core::ubuntu::media;

media::Player::Errors::OutOfProcessBufferStreamingNotSupported::OutOfProcessBufferStreamingNotSupported()
    : std::runtime_error{"Implementation does not support out-of-process buffer streaming"}
{
}

const media::Player::Configuration& media::Player::Client::default_configuration()
{
    static const media::Player::Configuration config
    {
        0,
        nullptr,
        nullptr
    };
    return config;
}

media::Player::Player()
{
}

media::Player::~Player()
{
}

