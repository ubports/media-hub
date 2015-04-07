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
              skeleton.signals.track_added
          }
    {
        std::cout << "Creating new TrackListSkeleton::Private" << std::endl;
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
    media::Player::LoopStatus loop_status;

    core::Signal<void> on_track_list_replaced;
    core::Signal<Track::Id> on_track_removed;
    core::Signal<Track::Id> on_track_changed;

    mpris::TrackList::Skeleton skeleton;

    struct Signals
    {
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackAdded, mpris::TrackList::Signals::TrackAdded::ArgumentType> DBusTrackAddedSignal;

        Signals(const std::shared_ptr<DBusTrackAddedSignal>& remote_track_added)
        {
            on_track_added.connect([remote_track_added](const media::Track::Id &id)
            {
                std::cout << "Emitting remote_track_added()" << std::endl;
                remote_track_added->emit(id);
            });
        }

        core::Signal<Track::Id> on_track_added;
    } signals;
};

media::TrackListSkeleton::TrackListSkeleton(
        const dbus::types::ObjectPath& op)
    : dbus::Skeleton<media::TrackList>(the_session_bus()),
      d(new Private(this, access_service()->add_object_for_path(op)))
{
    std::cout << "Creating new TrackListSkeleton" << std::endl;
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
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    const auto next_track = std::next(d->current_track);
    std::cout << "has_next track? " << (next_track != d->tracks->get().end() ? "yes" : "no") << std::endl;
    return next_track != d->tracks->get().end();
}

const media::Track::Id& media::TrackListSkeleton::next()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (d->tracks->get().empty())
        return *(d->current_track);

    std::cout << "next() LoopStatus: " << d->loop_status << std::endl;

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
        d->current_track = d->tracks->get().begin();
        return *(d->current_track);
    }
    else if (has_next())
    {
        // Keep returning the next track until the last track is reached
        std::cout << "Current track id before next(): " << *d->current_track << std::endl;
        d->current_track = std::next(d->current_track);
        std::cout << "Current track id after next(): " << *d->current_track << std::endl;
        std::cout << *this << std::endl;
    }

    return *(d->current_track);
}

const media::Track::Id& media::TrackListSkeleton::current()
{
    if (d->tracks->get().size() && (d->current_track == d->empty_iterator))
    {
        std::cout << "current_track id == the end()" << std::endl;
        d->current_track = d->tracks->get().begin();
    }
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
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (shuffle)
        shuffle_tracks();
    else
        unshuffle_tracks();

}

const core::Property<media::TrackList::Container>& media::TrackListSkeleton::tracks() const
{
    return *d->tracks;
}

const core::Signal<void>& media::TrackListSkeleton::on_track_list_replaced() const
{
    // Print the TrackList instance
    std::cout << *this << std::endl;
    return d->on_track_list_replaced;
}

const core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added() const
{
    return d->signals.on_track_added;
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
    // Print the TrackList instance
    std::cout << *this << std::endl;
    return d->on_track_list_replaced;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_added()
{
    return d->signals.on_track_added;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_removed()
{
    return d->on_track_removed;
}

core::Signal<media::Track::Id>& media::TrackListSkeleton::on_track_changed()
{
    return d->on_track_changed;
}

// operator<< pretty prints the given TrackList to the given output stream.
std::ostream& media::operator<<(std::ostream& out, const media::TrackList& tracklist)
{
    auto non_const_tl = const_cast<media::TrackList*>(&tracklist);
    out << "TrackList\n---------------" << std::endl;
    for (const media::Track::Id &id : tracklist.tracks().get())
    {
        // '*' denotes the current track
        out << "\t" << ((dynamic_cast<media::TrackListSkeleton*>(non_const_tl)->current() == id) ? "*" : "");
        out << "Track Id: " << id << std::endl;
        out << "\t\turi: " << dynamic_cast<media::TrackListImplementation*>(non_const_tl)->query_uri_for_track(id) << std::endl;
        //out << "\t\thas_next: " << tracklist.has_next(
    }

    out << "---------------\nEnd TrackList" << std::endl;
    return out;
}
