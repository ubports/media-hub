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

#include "com/ubuntu/music/track_list.h"

#include "com/ubuntu/music/track.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace music = com::ubuntu::music;

struct music::TrackList::Private
{
    std::vector<music::Track> tracks;
    bool is_editable = true;

    bool operator==(const Private& rhs) const
    {
        return tracks == rhs.tracks && is_editable == rhs.is_editable;
    }

};

music::TrackList::TrackList() : d(new Private())
{
}

music::TrackList::TrackList(const music::TrackList& rhs) : d(new Private(*rhs.d))
{
}

music::TrackList::~TrackList()
{
}

music::TrackList& music::TrackList::operator=(const music::TrackList& rhs)
{
    *d = *rhs.d;
    return *this;
}

bool music::TrackList::operator==(const music::TrackList& rhs) const
{
    return *d == *rhs.d;
}

bool music::TrackList::is_editable()
{
    return d->is_editable;
}

std::size_t music::TrackList::size() const
{
    return d->tracks.size();
}

music::TrackList::Iterator music::TrackList::begin()
{
    return d->tracks.begin();
}

music::TrackList::ConstIterator music::TrackList::begin() const
{
    return d->tracks.begin();
}

music::TrackList::ConstIterator music::TrackList::end() const
{
    return d->tracks.end();
}

music::TrackList::Iterator music::TrackList::end()
{
    return d->tracks.end();
}

music::TrackList::ConstIterator music::TrackList::find_for_uri(const Track::UriType& uri) const
{
    return std::find_if(d->tracks.begin(), d->tracks.end(), [uri](const Track& track) { return track.uri() == uri; });
}

music::TrackList::Iterator music::TrackList::find_for_uri(const Track::UriType& uri)
{
    return std::find_if(d->tracks.begin(), d->tracks.end(), [uri](const Track& track) { return track.uri() == uri; });
}

void music::TrackList::prepend_track_with_uri(
    const music::Track::UriType& uri,
    bool make_current)
{
    add_track_with_uri_at(
        uri,
        begin(),
        make_current);
}

void music::TrackList::append_track_with_uri(
    const music::Track::UriType& uri,
    bool make_current)
{
    add_track_with_uri_at(
        uri,
        end(),
        make_current);
}

void music::TrackList::add_track_with_uri_at(
    const music::Track::UriType& uri,
    music::TrackList::Iterator at,
    bool make_current)
{    
    d->tracks.insert(at, Track{uri, Track::MetaData{}});
    (void) make_current;
    // ToDo: Talk to service.
}

void music::TrackList::remove_track_at(TrackList::Iterator at)
{
    d->tracks.erase(at);
    // TODO: Talk to actual service here.
}

void music::TrackList::for_each(const std::function<void(const music::Track&)> functor) const
{
    for (auto track : d->tracks)
        functor(track);
}

void music::TrackList::go_to(const music::Track& track)
{
    (void) track;
}

music::Connection music::TrackList::on_track_list_replaced(const std::function<void()>& slot)
{
    (void) slot;
    return Connection(nullptr);
}

music::Connection music::TrackList::on_track_added(const std::function<void(const music::Track& t)>& slot)
{
    (void) slot;
    return Connection(nullptr);
}

music::Connection music::TrackList::on_track_removed(const std::function<void(const music::Track& t)>& slot)
{
    (void) slot;
    return Connection(nullptr);
}

music::Connection music::TrackList::on_current_track_changed(const std::function<void(const music::Track&)>& slot)
{
    (void) slot;
    return Connection(nullptr);
}
