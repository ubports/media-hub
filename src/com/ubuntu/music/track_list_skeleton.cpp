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

#include "track_list_skeleton.h"

#include <com/ubuntu/music/player.h>
#include <com/ubuntu/music/property.h>
#include <com/ubuntu/music/signal.h>
#include <com/ubuntu/music/track_list.h>

#include "property_stub.h"
#include "track_id.h"
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

struct music::TrackListSkeleton::Private
{
    Private(music::TrackListSkeleton* impl,
            dbus::Object::Ptr object)
        : impl(impl),
          object(object),
          can_edit_tracks(object->get_property<mpris::TrackList::Properties::CanEditTracks>()),
          tracks(object->get_property<mpris::TrackList::Properties::Tracks>())
    {
    }

    void handle_add_track_with_uri_at(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        Track::UriType uri; dbus::types::ObjectPath after; bool make_current;
        in->reader() >> uri >> after >> make_current;

        impl->add_track_with_uri_at(uri, music::Track::Id{after}, make_current);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply->get());
    }

    void handle_remove_track(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        dbus::types::ObjectPath track;
        in->reader() >> track;

        impl->remove_track(music::Track::Id{track});

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply->get());
    }

    void handle_go_to(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        dbus::types::ObjectPath track;
        in->reader() >> track;

        impl->go_to(music::Track::Id{track});

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply->get());
    }

    music::TrackListSkeleton* impl;
    dbus::Object::Ptr object;

    PropertyStub<bool, mpris::TrackList::Properties::CanEditTracks> can_edit_tracks;
    PropertyStub<TrackList::Container, mpris::TrackList::Properties::Tracks> tracks;

    Signal<void> on_track_list_replaced;
    Signal<std::shared_ptr<Track>> on_track_added;
    Signal<std::shared_ptr<Track>> on_track_removed;
    Signal<std::shared_ptr<Track>> on_track_changed;
};

music::TrackListSkeleton::TrackListSkeleton(
        const dbus::types::ObjectPath& op)
    : dbus::Skeleton<music::TrackList>(the_session_bus()),
      d(new Private(this, access_service()->add_object_for_path(op)))
{
    d->object->install_method_handler<mpris::TrackList::AddTrack>(
        std::bind(&Private::handle_add_track_with_uri_at,
                  std::ref(d),
                  std::placeholders::_1));

    d->object->install_method_handler<mpris::TrackList::RemoveTrack>(
        std::bind(&Private::handle_remove_track,
                  std::ref(d),
                  std::placeholders::_1));

    d->object->install_method_handler<mpris::TrackList::GoTo>(
        std::bind(&Private::handle_go_to,
                  std::ref(d),
                  std::placeholders::_1));
}

music::TrackListSkeleton::~TrackListSkeleton()
{
}

const music::Property<bool>& music::TrackListSkeleton::can_edit_tracks() const
{
    return d->can_edit_tracks;
}

music::Property<bool>& music::TrackListSkeleton::can_edit_tracks()
{
    return d->can_edit_tracks;
}

music::Property<music::TrackList::Container>& music::TrackListSkeleton::tracks()
{
    return d->tracks;
}

const music::Property<music::TrackList::Container>& music::TrackListSkeleton::tracks() const
{
    return d->tracks;
}

const music::Signal<void>& music::TrackListSkeleton::on_track_list_replaced() const
{
    return d->on_track_list_replaced;
}

const music::Signal<std::shared_ptr<music::Track>>& music::TrackListSkeleton::on_track_added() const
{
    return d->on_track_added;
}

const music::Signal<std::shared_ptr<music::Track>>& music::TrackListSkeleton::on_track_removed() const
{
    return d->on_track_removed;
}

const music::Signal<std::shared_ptr<music::Track>>& music::TrackListSkeleton::on_track_changed() const
{
    return d->on_track_changed;
}
