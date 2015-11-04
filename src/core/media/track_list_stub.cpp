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

#include <core/media/player.h>
#include <core/media/track_list.h>

#include "property_stub.h"
#include "track_list_traits.h"
#include "the_session_bus.h"

#include "mpris/track_list.h"

#include <core/dbus/property.h>
#include <core/dbus/types/object_path.h>
#include <core/dbus/types/variant.h>
#include <core/dbus/types/stl/map.h>
#include <core/dbus/types/stl/vector.h>

#include <limits>

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

struct media::TrackListStub::Private
{
    Private(
            TrackListStub* impl,
            const std::shared_ptr<media::Player>& parent,
            const dbus::Object::Ptr& object)
        : impl(impl),
          parent(parent),
          object(object),
          can_edit_tracks(object->get_property<mpris::TrackList::Properties::CanEditTracks>()),
          tracks(object->get_property<mpris::TrackList::Properties::Tracks>()),
          signals
          {
              object->get_signal<mpris::TrackList::Signals::TrackAdded>(),
              object->get_signal<mpris::TrackList::Signals::TracksAdded>(),
              object->get_signal<mpris::TrackList::Signals::TrackMoved>(),
              object->get_signal<mpris::TrackList::Signals::TrackRemoved>(),
              object->get_signal<mpris::TrackList::Signals::TrackListReplaced>(),
              object->get_signal<mpris::TrackList::Signals::TrackChanged>()
          }
    {
    }

    TrackListStub* impl;
    std::shared_ptr<media::Player> parent;
    dbus::Object::Ptr object;

    std::shared_ptr<core::dbus::Property<mpris::TrackList::Properties::CanEditTracks>> can_edit_tracks;
    std::shared_ptr<core::dbus::Property<mpris::TrackList::Properties::Tracks>> tracks;


    struct Signals
    {
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackAdded, mpris::TrackList::Signals::TrackAdded::ArgumentType> DBusTrackAddedSignal;
        typedef core::dbus::Signal<mpris::TrackList::Signals::TracksAdded, mpris::TrackList::Signals::TracksAdded::ArgumentType> DBusTracksAddedSignal;
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackMoved, mpris::TrackList::Signals::TrackMoved::ArgumentType> DBusTrackMovedSignal;
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackRemoved, mpris::TrackList::Signals::TrackRemoved::ArgumentType> DBusTrackRemovedSignal;
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackListReplaced, mpris::TrackList::Signals::TrackListReplaced::ArgumentType> DBusTrackListReplacedSignal;
        typedef core::dbus::Signal<mpris::TrackList::Signals::TrackChanged, mpris::TrackList::Signals::TrackChanged::ArgumentType> DBusTrackChangedSignal;

        Signals(const std::shared_ptr<DBusTrackAddedSignal>& track_added,
                const std::shared_ptr<DBusTracksAddedSignal>& tracks_added,
                const std::shared_ptr<DBusTrackMovedSignal>& track_moved,
                const std::shared_ptr<DBusTrackRemovedSignal>& track_removed,
                const std::shared_ptr<DBusTrackListReplacedSignal>& track_list_replaced,
                const std::shared_ptr<DBusTrackChangedSignal>& track_changed)
            : on_track_added(),
              on_tracks_added(),
              on_track_moved(),
              on_track_removed(),
              on_track_list_replaced(),
              on_track_changed(),
              dbus
              {
                  track_added,
                  tracks_added,
                  track_moved,
                  track_removed,
                  track_list_replaced,
                  track_changed,
              }
        {
            dbus.on_track_added->connect([this](const Track::Id& id)
            {
                std::cout << "OnTrackAdded signal arrived via the bus." << std::endl;
                on_track_added(id);
            });

            dbus.on_tracks_added->connect([this](const media::TrackList::ContainerURI& tracks)
            {
                std::cout << "OnTracksAdded signal arrived via the bus." << std::endl;
                on_tracks_added(tracks);
            });

            dbus.on_track_moved->connect([this](const Track::Id& id)
            {
                std::cout << "OnTrackMoved signal arrived via the bus." << std::endl;
                on_track_moved(id);
            });

            dbus.on_track_removed->connect([this](const Track::Id& id)
            {
                std::cout << "OnTrackRemoved signal arrived via the bus." << std::endl;
                on_track_removed(id);
            });

            dbus.on_track_list_replaced->connect([this](const media::TrackList::ContainerTrackIdTuple& list)
            {
                std::cout << "OnTrackListRemoved signal arrived via the bus." << std::endl;
                on_track_list_replaced(list);
            });

            dbus.on_track_changed->connect([this](const Track::Id& id)
            {
                std::cout << "OnTrackChanged signal arrived via the bus." << std::endl;
                on_track_changed(id);
            });
        }

        core::Signal<Track::Id> on_track_added;
        core::Signal<media::TrackList::ContainerURI> on_tracks_added;
        core::Signal<Track::Id> on_track_moved;
        core::Signal<Track::Id> on_track_removed;
        core::Signal<media::TrackList::ContainerTrackIdTuple> on_track_list_replaced;
        core::Signal<Track::Id> on_track_changed;
        core::Signal<std::pair<Track::Id, bool>> on_go_to_track;
        core::Signal<void> on_end_of_tracklist;

        struct DBus
        {
            std::shared_ptr<DBusTrackAddedSignal> on_track_added;
            std::shared_ptr<DBusTracksAddedSignal> on_tracks_added;
            std::shared_ptr<DBusTrackMovedSignal> on_track_moved;
            std::shared_ptr<DBusTrackRemovedSignal> on_track_removed;
            std::shared_ptr<DBusTrackListReplacedSignal> on_track_list_replaced;
            std::shared_ptr<DBusTrackChangedSignal> on_track_changed;
        } dbus;
    } signals;
};

media::TrackListStub::TrackListStub(
        const std::shared_ptr<media::Player>& parent,
        const core::dbus::Object::Ptr& object)
    : d(new Private(this, parent, object))
{
}

media::TrackListStub::~TrackListStub()
{
}

const core::Property<bool>& media::TrackListStub::can_edit_tracks() const
{
    return *d->can_edit_tracks;
}

const core::Property<media::TrackList::Container>& media::TrackListStub::tracks() const
{
    return *d->tracks;
}

media::Track::MetaData media::TrackListStub::query_meta_data_for_track(const media::Track::Id& id)
{
    auto op = d->object->invoke_method_synchronously<
                mpris::TrackList::GetTracksMetadata,
                std::map<std::string, std::string>>(id);

    if (op.is_error())
        throw std::runtime_error("Problem querying meta data for track: " + op.error());

    media::Track::MetaData md;
    for(auto pair : op.value())
    {
        md.set(pair.first, pair.second);
    }
    return md;
}

media::Track::UriType media::TrackListStub::query_uri_for_track(const media::Track::Id& id)
{
    auto op = d->object->invoke_method_synchronously<
                mpris::TrackList::GetTracksUri,
                std::string>(id);

    if (op.is_error())
        throw std::runtime_error("Problem querying track for uri: " + op.error());

    return op.value();
}

void media::TrackListStub::add_track_with_uri_at(
        const media::Track::UriType& uri,
        const media::Track::Id& position,
        bool make_current)
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::AddTrack, void>(
                uri,
                position,
                make_current);

    if (op.is_error())
    {
        if (op.error().name() ==
                mpris::TrackList::Error::InsufficientPermissionsToAddTrack::name)
            throw media::TrackList::Errors::InsufficientPermissionsToAddTrack{};
        else
            throw std::runtime_error{op.error().print()};
    }
}

void media::TrackListStub::add_tracks_with_uri_at(const ContainerURI& uris, const Track::Id& position)
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::AddTracks, void>(
                uris,
                position);

    if (op.is_error())
    {
        if (op.error().name() ==
                mpris::TrackList::Error::InsufficientPermissionsToAddTrack::name)
            throw media::TrackList::Errors::InsufficientPermissionsToAddTrack{};
        else
            throw std::runtime_error{op.error().print()};
    }
}

bool media::TrackListStub::move_track(const media::Track::Id& id, const media::Track::Id& to)
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::MoveTrack, void>(id, to);

    if (op.is_error())
    {
        if (op.error().name() ==
                mpris::TrackList::Error::FailedToMoveTrack::name)
            throw media::TrackList::Errors::FailedToMoveTrack{};
        else if (op.error().name() ==
                mpris::TrackList::Error::FailedToFindMoveTrackSource::name)
            throw media::TrackList::Errors::FailedToFindMoveTrackSource{op.error().print()};
        else if (op.error().name() ==
                mpris::TrackList::Error::FailedToFindMoveTrackDest::name)
            throw media::TrackList::Errors::FailedToFindMoveTrackDest{op.error().print()};
        else
            throw std::runtime_error{op.error().print()};

        return false;
    }

    return true;
}

void media::TrackListStub::remove_track(const media::Track::Id& track)
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::RemoveTrack, void>(
                track);

    if (op.is_error())
        throw std::runtime_error("Problem removing track: " + op.error());
}

void media::TrackListStub::go_to(const media::Track::Id& track, bool toggle_player_state)
{
    (void) toggle_player_state;
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::GoTo, void>(
                track);

    if (op.is_error())
        throw std::runtime_error("Problem adding track: " + op.error());
}

media::Track::Id media::TrackListStub::next()
{
    // TODO: Add this to the dbus interface on the server and implement a proper dbus method call
    return media::Track::Id{"/empty/track/id"};
}

media::Track::Id media::TrackListStub::previous()
{
    // TODO: Add this to the dbus interface on the server and implement a proper dbus method call
    return media::Track::Id{"/empty/track/id"};
}

void media::TrackListStub::shuffle_tracks()
{
    std::cerr << "shuffle_tracks() does nothing from the client side" << std::endl;
}

void media::TrackListStub::unshuffle_tracks()
{
    std::cerr << "unshuffle_tracks() does nothing from the client side" << std::endl;
}

void media::TrackListStub::reset()
{
    auto op = d->object->invoke_method_synchronously<mpris::TrackList::Reset, void>();

    if (op.is_error())
        throw std::runtime_error("Problem resetting tracklist: " + op.error());
}

const core::Signal<media::TrackList::ContainerTrackIdTuple>& media::TrackListStub::on_track_list_replaced() const
{
    return d->signals.on_track_list_replaced;
}

const core::Signal<media::Track::Id>& media::TrackListStub::on_track_added() const
{
    return d->signals.on_track_added;
}

const core::Signal<media::TrackList::ContainerURI>& media::TrackListStub::on_tracks_added() const
{
    return d->signals.on_tracks_added;
}

const core::Signal<media::Track::Id>& media::TrackListStub::on_track_moved() const
{
    return d->signals.on_track_moved;
}

const core::Signal<media::Track::Id>& media::TrackListStub::on_track_removed() const
{
    return d->signals.on_track_removed;
}

const core::Signal<media::Track::Id>& media::TrackListStub::on_track_changed() const
{
    return d->signals.on_track_changed;
}

const core::Signal<std::pair<media::Track::Id, bool>>& media::TrackListStub::on_go_to_track() const
{
    return d->signals.on_go_to_track;
}

const core::Signal<void>& media::TrackListStub::on_end_of_tracklist() const
{
    return d->signals.on_end_of_tracklist;
}
