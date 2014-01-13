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

namespace media = core::ubuntu::media;

struct media::Connection::Private
{
    Private(const media::Connection::Disconnector& disconnector)
            : disconnector(disconnector)
    {
    }

    ~Private()
    {
    }

    void disconnect()
    {
        static const media::Connection::Disconnector empty_disconnector{};

        std::lock_guard<std::mutex> lg(guard);

        if (!disconnector)
            return;

        disconnector();
        disconnector = empty_disconnector;
    }

    std::mutex guard;
    media::Connection::Disconnector disconnector;
};

media::Connection::Connection(const media::Connection::Disconnector& disconnector) : d(new Private(disconnector))
{
}

media::Connection::~Connection()
{
}

bool media::Connection::is_connected() const
{
    return (d->disconnector ? true : false);
}

void media::Connection::disconnect()
{
    d->disconnect();
}
