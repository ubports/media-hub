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
#ifndef CORE_UBUNTU_MEDIA_TRACK_LIST_H_
#define CORE_UBUNTU_MEDIA_TRACK_LIST_H_

#include <core/media/track.h>

#include <core/property.h>
#include <core/signal.h>

#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace core
{
namespace ubuntu
{
namespace media
{
class Player;

class TrackList : public std::enable_shared_from_this<TrackList>
{
  public:
    typedef std::vector<Track::Id> Container;
    typedef std::vector<Track::UriType> ContainerURI;
    typedef std::tuple<std::vector<Track::Id>, Track::Id> ContainerTrackIdTuple;
    typedef std::tuple<Track::Id, Track::Id> TrackIdTuple;
    typedef Container::iterator Iterator;
    typedef Container::const_iterator ConstIterator;

    struct Errors
    {
        Errors() = delete;

        struct InsufficientPermissionsToAddTrack : public std::runtime_error
        {
            InsufficientPermissionsToAddTrack();
        };

        struct TrackNotFound : public std::runtime_error
        {
            TrackNotFound();
        };

        struct FailedToMoveTrack : public std::runtime_error
        {
            FailedToMoveTrack();
        };

        struct FailedToFindMoveTrackSource : public std::runtime_error
        {
            FailedToFindMoveTrackSource(const std::string& err);
        };

        struct FailedToFindMoveTrackDest : public std::runtime_error
        {
            FailedToFindMoveTrackDest(const std::string& err);
        };
    };

    static const Track::Id& after_empty_track();

    TrackList(const TrackList&) = delete;
    ~TrackList();

    TrackList& operator=(const TrackList&) = delete;
    bool operator==(const TrackList&) const = delete;

    /** If set to false, calling add_track_with_uri_at or remove_track will have no effect. */
    virtual const core::Property<bool>& can_edit_tracks() const = 0;

    /** An array which contains the identifier of each track in the tracklist, in order. */
    virtual const core::Property<Container>& tracks() const = 0;

    /** Gets all the metadata available for a given Track. */
    virtual Track::MetaData query_meta_data_for_track(const Track::Id& id) = 0;

    /** Gets the URI for a given Track. */
    virtual Track::UriType query_uri_for_track(const Track::Id& id) = 0;

    /** Adds a URI into the TrackList. */
    virtual void add_track_with_uri_at(const Track::UriType& uri, const Track::Id& position, bool make_current) = 0;

    /** Adds a list of URIs into the TrackList. */
    virtual void add_tracks_with_uri_at(const ContainerURI& uris, const Track::Id& position) = 0;

    /** Moves track 'id' from its old position in the TrackList to new position. */
    virtual bool move_track(const Track::Id& id, const Track::Id& to) = 0;

    /** Removes a Track from the TrackList. */
    virtual void remove_track(const Track::Id& id) = 0;

    /** Skip to the specified TrackId. Calls stop() and play() on the player if toggle_player_state is true. */
    virtual void go_to(const Track::Id& track, bool toggle_player_state) = 0;

    /** Returns true if there is a next track in the TrackList after the current one playing */
    bool has_next() const;

    /** Returns true if there is a previous track in the TrackList before the current one playing */
    bool has_previous() const;

    /** Skip to the next Track in the TrackList if there is one. */
    virtual Track::Id next() = 0;

    /** Skip to the previous Track in the TrackList if there is one. */
    virtual Track::Id previous() = 0;

    /** Reorders the tracks such that they are in a random order. */
    virtual void shuffle_tracks() = 0;

    /** Restores the original order of tracks before shuffle mode was turned on. */
    virtual void unshuffle_tracks() = 0;

    /** Clears and resets the TrackList to the same as a newly constructed instance. */
    virtual void reset() = 0;

    /** Indicates that the entire tracklist has been replaced. */
    virtual const core::Signal<ContainerTrackIdTuple>& on_track_list_replaced() const = 0;

    /** Indicates that a track has been added to the track list. */
    virtual const core::Signal<Track::Id>& on_track_added() const = 0;

    /** Indicates that one or more tracks have been added to the track list. */
    virtual const core::Signal<ContainerURI>& on_tracks_added() const = 0;

    /** Indicates that a track has been moved within the track list. First template param
     *  holds the id of the track being moved. Second param holds the id of the track of the
     *  position to move the track to in the TrackList. */
    virtual const core::Signal<TrackIdTuple>& on_track_moved() const = 0;

    /** Indicates that a track has been removed from the track list. */
    virtual const core::Signal<Track::Id>& on_track_removed() const = 0;

    /** Indicates that the track list has been reset and there are no tracks now */
    virtual const core::Signal<void>& on_track_list_reset() const = 0;

    /** Indicates that the track list advanced from one track to another. */
    virtual const core::Signal<Track::Id>& on_track_changed() const = 0;

    /** Used to notify the Player of when the client requested that the Player should immediately play a new track. */
    virtual const core::Signal<std::pair<Track::Id, bool>>& on_go_to_track() const = 0;

    /** Used to notify the Player of when the end of the tracklist has been reached. */
    virtual const core::Signal<void>& on_end_of_tracklist() const = 0;

protected:
    TrackList();
};

}
}
}

#endif // CORE_UBUNTU_MEDIA_TRACK_LIST_H_
