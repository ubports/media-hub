/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include <algorithm>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <tuple>
#include <unistd.h>

#include <dbus/dbus.h>

#include "track_list_implementation.h"

#include "engine.h"

#include "core/media/logger/logger.h"

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

struct media::TrackListImplementation::Private
{
    typedef std::map<Track::Id, std::tuple<Track::UriType, Track::MetaData>> MetaDataCache;

    dbus::Object::Ptr object;
    size_t track_counter;
    MetaDataCache meta_data_cache;
    std::shared_ptr<media::Engine::MetaDataExtractor> extractor;
    // Used for caching the original tracklist order to be used to restore the order
    // to the live TrackList after shuffle is turned off
    media::TrackList::Container shuffled_tracks;
    bool shuffle;

    void updateCachedTrackMetadata(const media::Track::Id& id, const media::Track::UriType& uri)
    {
        if (meta_data_cache.count(id) == 0)
        {
            // FIXME: This code seems to conflict badly when called multiple times in a row: causes segfaults
#if 0
            try {
                meta_data_cache[id] = std::make_tuple(
                            uri,
                            extractor->meta_data_for_track_with_uri(uri));
            } catch (const std::runtime_error &e) {
                std::cerr << "Failed to retrieve metadata for track '" << uri << "' (" << e.what() << ")" << std::endl;
            }
#else
            meta_data_cache[id] = std::make_tuple(
                    uri,
                    core::ubuntu::media::Track::MetaData{});
#endif
        } else
        {
            std::get<0>(meta_data_cache[id]) = uri;
        }
    }

    media::TrackList::Container::iterator get_shuffled_insert_it()
    {
        media::TrackList::Container::iterator random_it = shuffled_tracks.begin();
        if (random_it == shuffled_tracks.end())
            return random_it;

        // This is slightly biased, but not much, as RAND_MAX >= 32767, which is
        // much more than the average number of tracks.
        // Note that for N tracks we have N + 1 possible insertion positions.
        std::advance(random_it, rand() % (shuffled_tracks.size() + 1));
        return random_it;
    }
};

media::TrackListImplementation::TrackListImplementation(
        const dbus::Bus::Ptr& bus,
        const dbus::Object::Ptr& object,
        const std::shared_ptr<media::Engine::MetaDataExtractor>& extractor,
        const media::apparmor::ubuntu::RequestContextResolver::Ptr& request_context_resolver,
        const media::apparmor::ubuntu::RequestAuthenticator::Ptr& request_authenticator)
    : media::TrackListSkeleton(bus, object, request_context_resolver, request_authenticator),
      d(new Private{object, 0, Private::MetaDataCache{},
                    extractor, media::TrackList::Container{}, false})
{
    can_edit_tracks().set(true);
}

media::TrackListImplementation::~TrackListImplementation()
{
}

media::Track::UriType media::TrackListImplementation::query_uri_for_track(const media::Track::Id& id)
{
    const auto it = d->meta_data_cache.find(id);

    if (it == d->meta_data_cache.end())
        return Track::UriType{};

    return std::get<0>(it->second);
}

media::Track::MetaData media::TrackListImplementation::query_meta_data_for_track(const media::Track::Id& id)
{
    const auto it = d->meta_data_cache.find(id);

    if (it == d->meta_data_cache.end())
        return Track::MetaData{};

    return std::get<1>(it->second);
}

void media::TrackListImplementation::add_track_with_uri_at(
        const media::Track::UriType& uri,
        const media::Track::Id& position,
        bool make_current)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    std::stringstream ss;
    ss << d->object->path().as_string() << "/" << d->track_counter++;
    Track::Id id{ss.str()};

    MH_DEBUG("Adding Track::Id: %s", id.c_str());
    MH_DEBUG("\tURI: %s", uri.c_str());

    const auto current = get_current_track();

    auto result = tracks().update([this, id, position, make_current](TrackList::Container& container)
    {
        auto it = std::find(container.begin(), container.end(), position);
        container.insert(it, id);

        return true;
    });

    if (result)
    {
        d->updateCachedTrackMetadata(id, uri);

        if (d->shuffle)
            d->shuffled_tracks.insert(d->get_shuffled_insert_it(), id);

        if (make_current)
        {
            set_current_track(id);
            go_to(id);
        } else {
            set_current_track(current);
        }

        MH_DEBUG("Signaling that we just added track id: %s", id.c_str());
        // Signal to the client that a track was added to the TrackList
        on_track_added()(id);

        // Signal to the client that the current track has changed for the first
        // track added to the TrackList
        if (tracks().get().size() == 1)
            on_track_changed()(id);
    }
}

void media::TrackListImplementation::add_tracks_with_uri_at(const ContainerURI& uris, const Track::Id& position)
{
    MH_TRACE("");

    const auto current = get_current_track();

    Track::Id current_id;
    ContainerURI tmp;
    for (const auto uri : uris)
    {
        // TODO: Refactor this code to use a smaller common function shared with add_track_with_uri_at()
        std::stringstream ss;
        ss << d->object->path().as_string() << "/" << d->track_counter++;
        Track::Id id{ss.str()};
        MH_DEBUG("Adding Track::Id: %s", id.c_str());
        MH_DEBUG("\tURI: %s", uri.c_str());

        tmp.push_back(id);

        Track::Id insert_position = position;

        auto it = std::find(tracks().get().begin(), tracks().get().end(), insert_position);
        const auto result = tracks().update([this, id, position, it, &insert_position](TrackList::Container& container)
        {
            container.insert(it, id);
            // Make sure the next insert position is after the current insert position
            // Update the Track::Id after which to insert the next one from uris
            insert_position = id;

            return true;
        });

        if (result)
        {
            d->updateCachedTrackMetadata(id, uri);

            if (d->shuffle)
                d->shuffled_tracks.insert(d->get_shuffled_insert_it(), id);

            // Signal to the client that the current track has changed for the first track added to the TrackList
            if (tracks().get().size() == 1)
                current_id = id;
        }
    }

    set_current_track(current);

    MH_DEBUG("Signaling that we just added %d tracks to the TrackList", tmp.size());
    on_tracks_added()(tmp);

    if (!current_id.empty())
        on_track_changed()(current_id);
}

bool media::TrackListImplementation::move_track(const media::Track::Id& id,
                                                const media::Track::Id& to)
{
    MH_TRACE("");

    std::cout << "-----------------------------------------------------" << std::endl;
    if (id.empty() or to.empty())
    {
        MH_ERROR("Can't move track since 'id' or 'to' are empty");
        return false;
    }

    if (id == to)
    {
        MH_ERROR("Can't move track to it's same position");
        return false;
    }

    if (tracks().get().size() == 1)
    {
        MH_ERROR("Can't move track since TrackList contains only one track");
        return false;
    }

    bool ret = false;
    const media::Track::Id current_id = *current_iterator();
    MH_DEBUG("current_track id: %s", current_id);
    // Get an iterator that points to the track that is the insertion point
    auto insert_point_it = std::find(tracks().get().begin(), tracks().get().end(), to);
    if (insert_point_it != tracks().get().end())
    {
        const auto result = tracks().update([this, id, to, current_id, &insert_point_it]
                (TrackList::Container& container)
        {
            // Get an iterator that points to the track to move within the TrackList
            auto to_move_it = std::find(tracks().get().begin(), tracks().get().end(), id);
            if (to_move_it != tracks().get().end())
            {
                container.erase(to_move_it);
            }
            else
            {
                throw media::TrackList::Errors::FailedToFindMoveTrackDest
                        ("Failed to find destination track " + to);
            }

            // Insert id at the location just before insert_point_it
            container.insert(insert_point_it, id);

            const auto new_current_track_it = std::find(tracks().get().begin(), tracks().get().end(), current_id);
            if (new_current_track_it != tracks().get().end())
            {
                const bool r = update_current_iterator(new_current_track_it);
                if (!r)
                {
                    throw media::TrackList::Errors::FailedToMoveTrack();
                }
                std::stringstream ss;
                ss << *current_iterator();
                MH_DEBUG("*** Updated current_iterator, id: %s", ss.str().c_str());
            }
            else
            {
                MH_ERROR("Can't update current_iterator - failed to find track after move");
                throw media::TrackList::Errors::FailedToMoveTrack();
            }

            return true;
        });

        if (result)
        {
            MH_DEBUG("TrackList after move");
            for(const auto track : tracks().get())
            {
                std::stringstream ss;
                ss << track;
                MH_DEBUG("%s", ss.str().c_str());
            }
            const media::TrackList::TrackIdTuple ids = std::make_tuple(id, to);
            // Signal to the client that track 'id' was moved within the TrackList
            on_track_moved()(ids);
            ret = true;
        }
    }
    else
    {
        throw media::TrackList::Errors::FailedToFindMoveTrackSource
                ("Failed to find source track " + id);
    }

    MH_DEBUG("-----------------------------------------------------");

    return ret;
}

void media::TrackListImplementation::remove_track(const media::Track::Id& id)
{
    const auto result = tracks().update([id](TrackList::Container& container)
    {
        container.erase(std::find(container.begin(), container.end(), id));
        return true;
    });

    reset_current_iterator_if_needed();

    if (result)
    {
        d->meta_data_cache.erase(id);

        if (d->shuffle)
            d->shuffled_tracks.erase(find(d->shuffled_tracks.begin(),
                                          d->shuffled_tracks.end(), id));

        on_track_removed()(id);

        // Make sure playback stops if all tracks were removed
        if (tracks().get().empty())
            on_end_of_tracklist()();
    }
}

void media::TrackListImplementation::go_to(const media::Track::Id& track)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    // Signal the Player instance to go to a specific track for playback
    on_go_to_track()(track);
    on_track_changed()(track);
}

void media::TrackListImplementation::set_shuffle(bool shuffle)
{
    d->shuffle = shuffle;

    if (shuffle) {
        d->shuffled_tracks = tracks().get();
        random_shuffle(d->shuffled_tracks.begin(), d->shuffled_tracks.end());
    }
}

bool media::TrackListImplementation::shuffle()
{
    return d->shuffle;
}

const media::TrackList::Container& media::TrackListImplementation::shuffled_tracks()
{
    return d->shuffled_tracks;
}

void media::TrackListImplementation::reset()
{
    MH_TRACE("");

    // Make sure playback stops
    on_end_of_tracklist()();
    // And make sure there is no "current" track
    media::TrackListSkeleton::reset();

    tracks().update([this](TrackList::Container& container)
    {
        container.clear();
        on_track_list_reset()();

        d->track_counter = 0;
        d->shuffled_tracks.clear();

        return true;
    });
}
