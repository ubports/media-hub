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

#include <core/media/player.h>
#include <core/media/property.h>
#include <core/media/signal.h>
#include <core/media/track_list.h>

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
namespace media = core::ubuntu::media;

struct media::TrackListSkeleton::Private
{
    Private(media::TrackListSkeleton* impl,
            dbus::Object::Ptr object)
        : impl(impl),
          object(object),
          can_edit_tracks(object->get_property<mpris::TrackList::Properties::CanEditTracks>()),
          tracks(object->get_property<mpris::TrackList::Properties::Tracks>()),
          current_track(tracks.get().begin())
    {
    }

    void handle_get_tracks_metadata(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        media::Track::Id track;
        in->reader() >> track;

        auto meta_data = impl->query_meta_data_for_track(track);

        auto reply = dbus::Message::make_method_return(msg);
        reply->writer() << *meta_data;
        impl->access_bus()->send(reply->get());
    }

    void handle_add_track_with_uri_at(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        Track::UriType uri; media::Track::Id after; bool make_current;
        in->reader() >> uri >> after >> make_current;

        impl->add_track_with_uri_at(uri, after, make_current);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply->get());
    }

    void handle_remove_track(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        media::Track::Id track;
        in->reader() >> track;

        impl->remove_track(track);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply->get());
    }

    void handle_go_to(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        media::Track::Id track;
        in->reader() >> track;

        impl->go_to(track);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply->get());
    }

    media::TrackListSkeleton* impl;
    dbus::Object::Ptr object;

    PropertyStub<bool, mpris::TrackList::Properties::CanEditTracks> can_edit_tracks;
    PropertyStub<TrackList::Container, mpris::TrackList::Properties::Tracks> tracks;
    TrackList::ConstIterator current_track;

    Signal<void> on_track_list_replaced;
    Signal<Track::Id> on_track_added;
    Signal<Track::Id> on_track_removed;
    Signal<Track::Id> on_track_changed;
};

media::TrackListSkeleton::TrackListSkeleton(
        const dbus::types::ObjectPath& op)
    : dbus::Skeleton<media::TrackList>(the_session_bus()),
      d(new Private(this, access_service()->add_object_for_path(op)))
{
    d->object->install_method_handler<mpris::TrackList::GetTracksMetadata>(
        std::bind(&Private::handle_get_tracks_metadata,
                  std::ref(d),
                  std::placeholders::_1));

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

media::TrackListSkeleton::~TrackListSkeleton()
{
}

bool media::TrackListSkeleton::has_next() const
{
    return std::next(d->current_track) != d->tracks.get().end();
}

const media::Track::Id& media::TrackListSkeleton::next()
{
    return *(d->current_track = std::next(d->current_track));
}

const media::Property<bool>& media::TrackListSkeleton::can_edit_tracks() const
{
    return d->can_edit_tracks;
}

media::Property<bool>& media::TrackListSkeleton::can_edit_tracks()
{
    return d->can_edit_tracks;
}

media::Property<media::TrackList::Container>& media::TrackListSkeleton::tracks()
{
    return d->tracks;
}

const media::Property<media::TrackList::Container>& media::TrackListSkeleton::tracks() const
{
    return d->tracks;
}

const media::Signal<void>& media::TrackListSkeleton::on_track_list_replaced() const
{
    return d->on_track_list_replaced;
}

const media::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added() const
{
    return d->on_track_added;
}

const media::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_removed() const
{
    return d->on_track_removed;
}

const media::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_changed() const
{
    return d->on_track_changed;
}

media::Signal<void>& media::TrackListSkeleton::on_track_list_replaced()
{
    return d->on_track_list_replaced;
}

media::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added()
{
    return d->on_track_added;
}

media::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_removed()
{
    return d->on_track_removed;
}

media::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_changed()
{
    return d->on_track_changed;
}
