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

#include <com/ubuntu/music/track_list.h>

#include "track_id.h"

namespace music = com::ubuntu::music;

const music::Track::Id& music::TrackList::after_empty_track()
{
    static const music::Track::Id id{org::freedesktop::dbus::types::ObjectPath{"/org/mpris/MediaPlayer2/TrackList/NoTrack"}};
    return id;
}

music::TrackList::TrackList()
{
}

music::TrackList::~TrackList()
{
}
