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
#include <core/media/track_list.h>

#include "codec.h"
#include "property_stub.h"
#include "track_list_traits.h"
#include "the_session_bus.h"

#include "mpris/track_list.h"

#include <core/dbus/object.h>
#include <core/dbus/property.h>
#include <core/dbus/types/object_path.h>
#include <core/dbus/types/variant.h>
#include <core/dbus/types/stl/map.h>
#include <core/dbus/types/stl/vector.h>

#include <limits>

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

struct media::TrackListSkeleton::Private
{
    Private(media::TrackListSkeleton* impl,
            dbus::Object::Ptr object)
        : impl(impl),
          object(object),
          can_edit_tracks(object->get_property<mpris::TrackList::Properties::CanEditTracks>()),
          tracks(object->get_property<mpris::TrackList::Properties::Tracks>()),
          current_track(tracks->get().begin()),
          empty_iterator(tracks->get().begin())
    {
    }

    void handle_get_tracks_metadata(const core::dbus::Message::Ptr& msg)
    {
        media::Track::Id track;
        msg->reader() >> track;

        auto meta_data = impl->query_meta_data_for_track(track);

        auto reply = dbus::Message::make_method_return(msg);
        reply->writer() << *meta_data;
        impl->access_bus()->send(reply);
    }

    void handle_add_track_with_uri_at(const core::dbus::Message::Ptr& msg)
    {
        Track::UriType uri; media::Track::Id after; bool make_current;
        msg->reader() >> uri >> after >> make_current;

        impl->add_track_with_uri_at(uri, after, make_current);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    void handle_remove_track(const core::dbus::Message::Ptr& msg)
    {
        media::Track::Id track;
        msg->reader() >> track;

        impl->remove_track(track);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    void handle_go_to(const core::dbus::Message::Ptr& msg)
    {
        media::Track::Id track;
        msg->reader() >> track;

        impl->go_to(track);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    media::TrackListSkeleton* impl;
    dbus::Object::Ptr object;

    std::shared_ptr<core::dbus::Property<mpris::TrackList::Properties::CanEditTracks>> can_edit_tracks;
    std::shared_ptr<core::dbus::Property<mpris::TrackList::Properties::Tracks>> tracks;
    TrackList::ConstIterator current_track;
    TrackList::ConstIterator empty_iterator;

    core::Signal<void> on_track_list_replaced;
    core::Signal<Track::Id> on_track_added;
    core::Signal<Track::Id> on_track_removed;
    core::Signal<Track::Id> on_track_changed;
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
    return d->current_track != d->tracks->get().end();
}

const media::Track::Id& media::TrackListSkeleton::next()
{
    if (d->tracks->get().empty())
        return *(d->current_track);

    if (d->tracks->get().size() && (d->current_track == d->empty_iterator))
    {        
        d->current_track = d->tracks->get().begin();
        return *(d->current_track = std::next(d->current_track));
    }

    d->current_track = std::next(d->current_track);
    return *(d->current_track);
}

const core::Property<bool>& media::TrackListSkeleton::can_edit_tracks() const
{
    return *d->can_edit_tracks;
}

core::Property<bool>& media::TrackListSkeleton::can_edit_tracks()
{
    return *d->can_edit_tracks;
}

core::Property<media::TrackList::Container>& media::TrackListSkeleton::tracks()
{
    return *d->tracks;
}

const core::Property<media::TrackList::Container>& media::TrackListSkeleton::tracks() const
{
    return *d->tracks;
}

const core::Signal<void>& media::TrackListSkeleton::on_track_list_replaced() const
{
    return d->on_track_list_replaced;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added() const
{
    return d->on_track_added;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_removed() const
{
    return d->on_track_removed;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_changed() const
{
    return d->on_track_changed;
}

core::Signal<void>& media::TrackListSkeleton::on_track_list_replaced()
{
    return d->on_track_list_replaced;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added()
{
    return d->on_track_added;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_removed()
{
    return d->on_track_removed;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_changed()
{
    return d->on_track_changed;
}
