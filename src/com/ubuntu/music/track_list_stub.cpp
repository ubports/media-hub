/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "track_list_stub.h"

#include <com/ubuntu/music/player.h>
#include <com/ubuntu/music/property.h>
#include <com/ubuntu/music/signal.h>
#include <com/ubuntu/music/track_list.h>

#include "property_stub.h"
#include "track_list_traits.h"
#include "the_session_bus.h"

#include "mpris/track_list.h"

#include <org/freedesktop/dbus/types/object_path.h>
#include <org/freedesktop/dbus/types/variant.h>
#include <org/freedesktop/dbus/types/stl/map.h>
#include <org/freedesktop/dbus/types/stl/vector.h>

#include <limits>

namespace dbus = org::freedesktop::dbus;
namespace music = com::ubuntu::music;

struct music::TrackListStub::Private
{
    Private(
            TrackListStub* impl,
            const std::shared_ptr<music::Player>& parent,
            const dbus::types::ObjectPath& op)
        : impl(impl),
          parent(parent),
          object(impl->access_service()->object_for_path(op)),
          can_edit_tracks(object->get_property<mpris::TrackList::Properties::CanEditTracks>()),
          tracks(object->get_property<mpris::TrackList::Properties::Tracks>())
    {
    }

    TrackListStub* impl;
    std::shared_ptr<music::Player> parent;
    dbus::Object::Ptr object;

    PropertyStub<bool, mpris::TrackList::Properties::CanEditTracks> can_edit_tracks;
    PropertyStub<TrackList::Container, mpris::TrackList::Properties::Tracks> tracks;

    Signal<void> on_track_list_replaced;
    Signal<Track::Id> on_track_added;
    Signal<Track::Id> on_track_removed;
    Signal<Track::Id> on_track_changed;
};

music::TrackListStub::TrackListStub(
        const std::shared_ptr<music::Player>& parent,
        const org::freedesktop::dbus::types::ObjectPath& op)
    : dbus::Stub<music::TrackList>(the_session_bus()),
      d(new Private(this, parent, op))
{
}

music::TrackListStub::~TrackListStub()
{
}

const music::Property<bool>& music::TrackListStub::can_edit_tracks() const
{
    return d->can_edit_tracks;
}

const music::Property<music::TrackList::Container>& music::TrackListStub::tracks() const
{
    return d->tracks;
}

music::Track::MetaData music::TrackListStub::query_meta_data_for_track(const music::Track::Id& id)
{
    auto op
            = d->object->invoke_method_synchronously<
                mpris::TrackList::GetTracksMetadata,
                std::map<std::string, std::string>>(id);

    if (op.is_error())
        throw std::runtime_error("Problem querying meta data for track: " + op.error());

    music::Track::MetaData md;
    for(auto pair : op.value())
    {
        md.set(pair.first, pair.second);
    }
    return md;
}

void music::TrackListStub::add_track_with_uri_at(
        const music::Track::UriType& uri,
        const music::Track::Id& id,
        bool make_current)
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::AddTrack, void>(
                uri,
                id,
                make_current);

    if (op.is_error())
        throw std::runtime_error("Problem adding track: " + op.error());
}

void music::TrackListStub::remove_track(const music::Track::Id& track)
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::RemoveTrack, void>(
                track);

    if (op.is_error())
        throw std::runtime_error("Problem removing track: " + op.error());
}

void music::TrackListStub::go_to(const music::Track::Id& track)
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::GoTo, void>(
                track);

    if (op.is_error())
        throw std::runtime_error("Problem adding track: " + op.error());
}

const music::Signal<void>& music::TrackListStub::on_track_list_replaced() const
{
    return d->on_track_list_replaced;
}

const music::Signal<music::Track::Id>& music::TrackListStub::on_track_added() const
{
    return d->on_track_added;
}

const music::Signal<music::Track::Id>& music::TrackListStub::on_track_removed() const
{
    return d->on_track_removed;
}

const music::Signal<music::Track::Id>& music::TrackListStub::on_track_changed() const
{
    return d->on_track_changed;
}
