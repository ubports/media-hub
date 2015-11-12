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

#include <core/media/track_list.h>

namespace media = core::ubuntu::media;

media::TrackList::Errors::InsufficientPermissionsToAddTrack::InsufficientPermissionsToAddTrack()
    : std::runtime_error{"Insufficient client permissions for adding track to TrackList"}
{
}

<<<<<<< TREE
media::TrackList::Errors::FailedToMoveTrack::FailedToMoveTrack()
    : std::runtime_error{"Failed to move track within TrackList"}
{
}

media::TrackList::Errors::FailedToFindMoveTrackSource::FailedToFindMoveTrackSource
        (const std::string &e)
    : std::runtime_error{e}
{
}

media::TrackList::Errors::FailedToFindMoveTrackDest::FailedToFindMoveTrackDest
        (const std::string &e)
    : std::runtime_error{e}
{
}

media::TrackList::Errors::TrackNotFound::TrackNotFound()
    : std::runtime_error{"Track not found in TrackList"}
{
}

const media::Track::Id& media::TrackList::after_empty_track()
{
    static const media::Track::Id id{"/org/mpris/MediaPlayer2/TrackList/NoTrack"};
    return id;
}

media::TrackList::TrackList()
{
}

media::TrackList::~TrackList()
{
}

bool media::TrackList::has_next() const
{
    return false;
}

bool media::TrackList::has_previous() const
{
    return false;
}
