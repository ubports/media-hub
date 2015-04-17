/*
 * Copyright © 2015 Canonical Ltd.
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
#include "track_list_implementation.h"

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

#include <iostream>
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
          empty_iterator(tracks->get().begin()),
          loop_status(media::Player::LoopStatus::none),
          skeleton{mpris::TrackList::Skeleton::Configuration{object, mpris::TrackList::Skeleton::Configuration::Defaults{}}},
          signals
          {
              skeleton.signals.track_added,
              skeleton.signals.track_removed,
              skeleton.signals.tracklist_replaced
          }
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

        current_track = std::find(tracks->get().begin(), tracks->get().end(), track);
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
    media::Player::LoopStatus loop_status;

    mpris::TrackList::Skeleton skeleton;

    struct Signals
    {
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackAdded, mpris::TrackList::Signals::TrackAdded::ArgumentType> DBusTrackAddedSignal;
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackRemoved, mpris::TrackList::Signals::TrackRemoved::ArgumentType> DBusTrackRemovedSignal;
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackListReplaced, mpris::TrackList::Signals::TrackListReplaced::ArgumentType> DBusTrackListReplacedSignal;

        Signals(const std::shared_ptr<DBusTrackAddedSignal>& remote_track_added,
                const std::shared_ptr<DBusTrackRemovedSignal>& remote_track_removed,
                const std::shared_ptr<DBusTrackListReplacedSignal>& remote_track_list_replaced)
        {
            // Connect all of the MPRIS interface signals to be emitted over dbus
            on_track_added.connect([remote_track_added](const media::Track::Id &id)
            {
                remote_track_added->emit(id);
            });

            on_track_removed.connect([remote_track_removed](const media::Track::Id &id)
            {
                remote_track_removed->emit(id);
            });

            on_track_list_replaced.connect([remote_track_list_replaced](const media::TrackList::ContainerTrackIdTuple &tltuple)
            {
                remote_track_list_replaced->emit(tltuple);
            });
        }

        core::Signal<Track::Id> on_track_added;
        core::Signal<Track::Id> on_track_removed;
        core::Signal<TrackList::ContainerTrackIdTuple> on_track_list_replaced;
        core::Signal<Track::Id> on_track_changed;
        core::Signal<Track::Id> on_go_to_track;
    } signals;
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
    const auto next_track = std::next(d->current_track);
    std::cout << "has_next track? " << (next_track != tracks().get().end() ? "yes" : "no") << std::endl;
    return next_track != tracks().get().end();
}

const media::Track::Id& media::TrackListSkeleton::next()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (tracks().get().empty())
        return *(d->current_track);

    // Loop on the current track forever
    if (d->loop_status == media::Player::LoopStatus::track)
    {
        std::cout << "Looping on the current track..." << std::endl;
        return *(d->current_track);
    }
    // Loop over the whole playlist and repeat
    else if (d->loop_status == media::Player::LoopStatus::playlist && !has_next())
    {
        std::cout << "Looping on the entire TrackList..." << std::endl;
        d->current_track = tracks().get().begin();
        return *(d->current_track);
    }
    else if (has_next())
    {
        // Keep returning the next track until the last track is reached
        d->current_track = std::next(d->current_track);
        std::cout << *this << std::endl;
    }

    return *(d->current_track);
}

const media::Track::Id& media::TrackListSkeleton::current()
{
    // Prevent the TrackList from sitting at the end which will cause
    // a segfault when calling current()
    if (tracks().get().size() && (d->current_track == d->empty_iterator))
        d->current_track = tracks().get().begin();
    else if (tracks().get().empty())
        std::cerr << "TrackList is empty therefore there is no valid current track" << std::endl;

    return *(d->current_track);
}

const core::Property<bool>& media::TrackListSkeleton::can_edit_tracks() const
{
    return *d->skeleton.properties.can_edit_tracks;
}

core::Property<bool>& media::TrackListSkeleton::can_edit_tracks()
{
    return *d->skeleton.properties.can_edit_tracks;
}

core::Property<media::TrackList::Container>& media::TrackListSkeleton::tracks()
{
    return *d->skeleton.properties.tracks;
}

void media::TrackListSkeleton::on_loop_status_changed(const media::Player::LoopStatus& loop_status)
{
    d->loop_status = loop_status;
}

media::Player::LoopStatus media::TrackListSkeleton::loop_status() const
{
    return d->loop_status;
}

void media::TrackListSkeleton::on_shuffle_changed(bool shuffle)
{
    if (shuffle)
        shuffle_tracks();
    else
        unshuffle_tracks();
}

const core::Property<media::TrackList::Container>& media::TrackListSkeleton::tracks() const
{
    return *d->skeleton.properties.tracks;
}

const core::Signal<media::TrackList::ContainerTrackIdTuple>& media::TrackListSkeleton::on_track_list_replaced() const
{
    // Print the TrackList instance
    std::cout << *this << std::endl;
    return d->signals.on_track_list_replaced;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added() const
{
    return d->signals.on_track_added;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_removed() const
{
    return d->signals.on_track_removed;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_changed() const
{
    return d->signals.on_track_changed;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_go_to_track() const
{
    return d->signals.on_go_to_track;
}

core::Signal<media::TrackList::ContainerTrackIdTuple>& media::TrackListSkeleton::on_track_list_replaced()
{
    return d->signals.on_track_list_replaced;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added()
{
    return d->signals.on_track_added;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_removed()
{
    return d->signals.on_track_removed;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_changed()
{
    return d->signals.on_track_changed;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_go_to_track()
{
    return d->signals.on_go_to_track;
}

// operator<< pretty prints the given TrackList to the given output stream.
inline std::ostream& media::operator<<(std::ostream& out, const media::TrackList& tracklist)
{
    auto non_const_tl = const_cast<media::TrackList*>(&tracklist);
    out << "TrackList\n---------------" << std::endl;
    for (const media::Track::Id &id : tracklist.tracks().get())
    {
        // '*' denotes the current track
        out << "\t" << ((dynamic_cast<media::TrackListSkeleton*>(non_const_tl)->current() == id) ? "*" : "");
        out << "Track Id: " << id << std::endl;
        out << "\t\turi: " << dynamic_cast<media::TrackListImplementation*>(non_const_tl)->query_uri_for_track(id) << std::endl;
    }

    out << "---------------\nEnd TrackList" << std::endl;
    return out;
}

