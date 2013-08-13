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

#include "com/ubuntu/music/connection.h"

#include "connection_private.h"

#include <functional>
#include <memory>

namespace music = com::ubuntu::music;

music::Connection::Connection(const std::shared_ptr<music::Connection::Private>& p) : d(p)
{
}

music::Connection::Connection(const music::Connection& rhs) : d(rhs.d)
{
}
    
music::Connection::~Connection()
{
}

music::Connection& music::Connection::operator=(const music::Connection& rhs)
{
    d = rhs.d;
    return *this;
}

bool music::Connection::operator==(const music::Connection& rhs) const
{
    return d == rhs.d;
}

bool music::Connection::is_connected() const
{
    return d->connection.connected();
}

void music::Connection::disconnect()
{
    d->connection.disconnect();
}
