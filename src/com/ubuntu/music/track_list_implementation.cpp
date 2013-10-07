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

#include "track_list_implementation.h"

#include "track_id.h"

#include <com/ubuntu/music/property.h>

namespace dbus = org::freedesktop::dbus;
namespace music = com::ubuntu::music;

namespace
{
struct DummyTrack : public music::Track
{
    DummyTrack(
            const music::Track::Id& id,
            const music::Track::UriType& uri,
            const music::Track::MetaData& meta_data)
        : music::Track(id), d{id, uri, music::Property<music::Track::MetaData>(meta_data)}
    {
    }

    virtual const UriType& uri() const
    {
        return d.uri;
    }

    virtual const music::Property<MetaData>& meta_data() const
    {
        return d.meta_data;
    }

    struct
    {
        music::Track::Id id;
        music::Track::UriType uri;
        music::Property<music::Track::MetaData> meta_data;
    } d;
};
}

struct music::TrackListImplementation::Private
{
    dbus::types::ObjectPath path;
};

music::TrackListImplementation::TrackListImplementation(
        const dbus::types::ObjectPath& op)
    : music::TrackListSkeleton(op),
      d(new Private{op})
{
    can_edit_tracks().set(true);
}

music::TrackListImplementation::~TrackListImplementation()
{
}

void music::TrackListImplementation::add_track_with_uri_at(
        const music::Track::UriType& uri,
        const music::Track::Id& position,
        bool make_current)
{
    static size_t track_counter = 0;
    tracks().update([this, uri, position, make_current](TrackList::Container& container)
    {
        std::stringstream ss; ss << d->path.as_string() << "/" << track_counter++;
        Track::Id id{dbus::types::ObjectPath{ss.str()}};
        container.push_back(std::make_shared<DummyTrack>(id, uri, music::Track::MetaData{}));
        return true;
    });
}

void music::TrackListImplementation::remove_track(const music::Track::Id& id)
{
    (void) id;
}

void music::TrackListImplementation::go_to(const music::Track::Id& track)
{
    (void) track;
}
