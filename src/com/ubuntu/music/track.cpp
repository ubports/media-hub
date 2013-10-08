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

#include <com/ubuntu/music/track.h>
#include <com/ubuntu/music/property.h>

#include <map>

namespace music = com::ubuntu::music;

struct music::Track::Private
{
    music::Track::Id id;
};

music::Track::Track(const music::Track::Id& id) : d(new Private{id})
{
}

music::Track::~Track()
{
}

const music::Track::Id& music::Track::id() const
{
    return d->id;
}
