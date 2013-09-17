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

#include "com/ubuntu/music/player.h"
#include "com/ubuntu/music/service.h"

namespace music = com::ubuntu::music;

const std::shared_ptr<music::Service> music::Service::Client::instance()
{
    static std::shared_ptr<music::Service> instance{new music::Service()};
    return instance;
}

music::Service::Service()
{
}

music::Service::~Service()
{
}

std::shared_ptr<music::Player> music::Service::create_session()
{
    static std::shared_ptr<music::Player> session{new music::Player(shared_from_this())};
    return session;
}
