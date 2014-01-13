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

#include "core/media/connection.h"

#include <functional>
#include <memory>
#include <mutex>

namespace music = com::ubuntu::music;

struct music::Connection::Private
{
    Private(const music::Connection::Disconnector& disconnector)
            : disconnector(disconnector)
    {
    }

    ~Private()
    {
    }

    void disconnect()
    {
        static const music::Connection::Disconnector empty_disconnector{};

        std::lock_guard<std::mutex> lg(guard);

        if (!disconnector)
            return;

        disconnector();
        disconnector = empty_disconnector;
    }

    std::mutex guard;
    music::Connection::Disconnector disconnector;
};

music::Connection::Connection(const music::Connection::Disconnector& disconnector) : d(new Private(disconnector))
{
}
    
music::Connection::~Connection()
{
}

bool music::Connection::is_connected() const
{
    return (d->disconnector ? true : false);
}

void music::Connection::disconnect()
{
    d->disconnect();
}
