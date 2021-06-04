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

#include "track_list_implementation.h"

#include "engine.h"
#include "logging.h"

#include <QMap>
#include <QPair>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QUrl>

#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <tuple>
#include <unistd.h>

namespace media = core::ubuntu::media;

using namespace media;

namespace core {
namespace ubuntu {
namespace media {

class TrackListImplementationPrivate
{
    Q_DECLARE_PUBLIC(TrackListImplementation)

public:
    typedef QMap<Track::Id, QPair<QUrl, Track::MetaData>> MetaDataCache;

    TrackListImplementationPrivate(
        const QSharedPointer<media::Engine::MetaDataExtractor> &extractor,
        TrackListImplementation *q);

    TrackList::ConstIterator empty_iterator() const { return m_tracks.begin(); }
    bool is_first_track(const TrackList::ConstIterator &it) const {
        return it == m_tracks.begin();
    }
    bool is_last_track(const TrackList::ConstIterator &it) const {
        return it == m_tracks.end();
    }

    TrackList::ConstIterator current_iterator() const;
    void set_current_track(const Track::Id &id);
    Track::Id get_current_track() const;
    TrackList::ConstIterator get_current_shuffled() const;

    void add_track_with_uri_at(const QUrl &uri,
                               const Track::Id &position,
                               bool make_current);
    void add_tracks_with_uri_at(const QVector<QUrl> &uris,
                                const Track::Id &position);
    bool move_track(const Track::Id &id, const Track::Id &to);
    void remove_track(const Track::Id &id);
    void do_remove_track(const Track::Id &id);
    void go_to(const Track::Id &track);
    void updateCachedTrackMetadata(const Track::Id &id, const QUrl &uri)
    {
        auto i = meta_data_cache.find(id);
        if (i == meta_data_cache.end()) {
            meta_data_cache[id] = qMakePair(uri, Track::MetaData{});
        } else {
            i.value().first = uri;
        }
    }

    media::TrackList::Container::iterator get_shuffled_insert_it()
    {
        auto random_it = shuffled_tracks.begin();
        if (random_it == shuffled_tracks.end())
            return random_it;

        uint32_t random = QRandomGenerator::global()->generate();
        // This is slightly biased, but not much, as RAND_MAX >= 32767, which is
        // much more than the average number of tracks.
        // Note that for N tracks we have N + 1 possible insertion positions.
        std::advance(random_it, random % (shuffled_tracks.count() + 1));
        return random_it;
    }

    size_t track_counter;
    MetaDataCache meta_data_cache;
    QSharedPointer<Engine::MetaDataExtractor> extractor;
    // Used for caching the original tracklist order to be used to restore the order
    // to the live TrackList after shuffle is turned off
    TrackList::Container shuffled_tracks;
    TrackList::Container m_tracks;
    bool shuffle;
    mutable media::Track::Id current_track;
    media::Player::LoopStatus loop_status;
    uint64_t current_position;
    TrackListImplementation *q_ptr;
};

}}} // namespace

TrackListImplementationPrivate::TrackListImplementationPrivate(
        const QSharedPointer<media::Engine::MetaDataExtractor> &extractor,
        TrackListImplementation *q):
    track_counter(0),
    extractor(extractor),
    shuffle(false),
    loop_status(media::Player::LoopStatus::none),
    current_position(0),
    q_ptr(q)
{
}

TrackList::ConstIterator
TrackListImplementationPrivate::current_iterator() const
{
    // Prevent the TrackList from sitting at the end which will cause
    // a segfault when calling current()
    if (!m_tracks.isEmpty() && current_track.isEmpty())
    {
        MH_DEBUG("Wrapping d->current_track back to begin()");
        current_track = m_tracks.first();
    }
    else if (m_tracks.isEmpty())
    {
        MH_ERROR("TrackList is empty therefore there is no valid current track");
    }

    return std::find(m_tracks.begin(), m_tracks.end(), current_track);
}

void TrackListImplementationPrivate::set_current_track(const Track::Id &id)
{
    if (m_tracks.contains(id))
        current_track = id;
}

Track::Id TrackListImplementationPrivate::get_current_track() const
{
    if (m_tracks.isEmpty())
        return Track::Id{};

    return current_track;
}

TrackList::ConstIterator
TrackListImplementationPrivate::get_current_shuffled() const
{
    auto current_id = get_current_track();
    return std::find(shuffled_tracks.begin(), shuffled_tracks.end(), current_id);
}

void TrackListImplementationPrivate::add_track_with_uri_at(
        const QUrl &uri,
        const media::Track::Id& position,
        bool make_current)
{
    Q_Q(TrackListImplementation);
    MH_TRACE("");

    Track::Id id = q->objectName() + '/' + QString::number(track_counter++);

    MH_DEBUG("Adding Track::Id: %s", qUtf8Printable(id));
    MH_DEBUG("\tURI: %s", qUtf8Printable(uri.toString()));

    const auto current = get_current_track();

    auto it = std::find(m_tracks.begin(), m_tracks.end(), position);
    m_tracks.insert(it, id);

    updateCachedTrackMetadata(id, uri);

    if (shuffle)
        shuffled_tracks.insert(get_shuffled_insert_it(), id);

    if (make_current) {
        set_current_track(id);
        go_to(id);
    } else {
        set_current_track(current);
    }

    MH_DEBUG("Signaling that we just added track id: %s", qUtf8Printable(id));
    // Signal to the client that a track was added to the TrackList
    Q_EMIT q->trackAdded(id);

    // Signal to the client that the current track has changed for the first
    // track added to the TrackList
    if (m_tracks.count() == 1) {
        Q_EMIT q->trackChanged(id);
    }
}

void TrackListImplementationPrivate::add_tracks_with_uri_at(
        const QVector<QUrl> &uris,
        const Track::Id &position)
{
    Q_Q(TrackListImplementation);

    const auto current = get_current_track();

    Track::Id current_id;
    QVector<QUrl> tmp;
    for (const auto uri : uris)
    {
        // TODO: Refactor this code to use a smaller common function shared with add_track_with_uri_at()
        Track::Id id = q->objectName() + '/' +
            QString::number(track_counter++);
        MH_DEBUG("Adding Track::Id: %s", qUtf8Printable(id));
        MH_DEBUG("\tURI: %s", qUtf8Printable(uri.toString()));

        tmp.push_back(id);

        Track::Id insert_position = position;

        auto it = std::find(m_tracks.begin(), m_tracks.end(), insert_position);
        m_tracks.insert(it, id);
        // Make sure the next insert position is after the current insert position
        // Update the Track::Id after which to insert the next one from uris
        insert_position = id;

        updateCachedTrackMetadata(id, uri);

        if (shuffle)
            shuffled_tracks.insert(get_shuffled_insert_it(), id);

        if (m_tracks.count() == 1)
            current_id = id;
    }

    set_current_track(current);

    MH_DEBUG("Signaling that we just added %d tracks to the TrackList", tmp.size());
    Q_EMIT q->tracksAdded(tmp);

    if (!current_id.isEmpty()) {
        Q_EMIT q->trackChanged(current_id);
    }
}

bool TrackListImplementationPrivate::move_track(const media::Track::Id &id,
                                                const media::Track::Id &to)
{
    Q_Q(TrackListImplementation);
    MH_DEBUG("-----------------------------------------------------");
    if (id.isEmpty() or to.isEmpty())
    {
        MH_ERROR("Can't move track since 'id' or 'to' are empty");
        return false;
    }

    if (id == to)
    {
        MH_ERROR("Can't move track to it's same position");
        return false;
    }

    if (m_tracks.count() == 1)
    {
        MH_ERROR("Can't move track since TrackList contains only one track");
        return false;
    }

    MH_DEBUG("current_track id: %s", qUtf8Printable(current_track));
    // Get an iterator that points to the track that is the insertion point
    auto insert_point_it = std::find(m_tracks.begin(), m_tracks.end(), to);
    if (insert_point_it == m_tracks.end()) {
        throw media::TrackList::Errors::FailedToFindMoveTrackSource
                ("Failed to find source track " + id);
    }

    // Get an iterator that points to the track to move within the TrackList
    auto to_move_it = std::find(m_tracks.begin(), m_tracks.end(), id);
    if (to_move_it != m_tracks.end())
    {
        m_tracks.erase(to_move_it);
    }
    else
    {
        throw media::TrackList::Errors::FailedToFindMoveTrackDest
                ("Failed to find destination track " + to);
    }

    // Insert id at the location just before insert_point_it
    m_tracks.insert(insert_point_it, id);

    const auto new_current_track_it = std::find(m_tracks.begin(), m_tracks.end(), current_track);
    if (!current_track.isEmpty() && new_current_track_it == m_tracks.end()) {
        MH_ERROR("Can't update current_iterator - failed to find track after move");
        throw media::TrackList::Errors::FailedToMoveTrack();
    }

    MH_DEBUG("TrackList after move");
    for(const auto track : m_tracks)
    {
        MH_DEBUG("%s", qUtf8Printable(track));
    }
    // Signal to the client that track 'id' was moved within the TrackList
    Q_EMIT q->trackMoved(id, to);

    MH_DEBUG("-----------------------------------------------------");

    return true;
}

void TrackListImplementationPrivate::remove_track(const Track::Id &id)
{
    Q_Q(TrackListImplementation);

    media::Track::Id track = id;

    auto id_it = std::find(m_tracks.begin(), m_tracks.end(), track);
    if (id_it == m_tracks.end()) {
        QString err_str = QString("Track ") + track + " not found in track list";
        MH_WARNING() << err_str;
        throw media::TrackList::Errors::TrackNotFound(err_str);
    }

    media::Track::Id next;
    bool deleting_current = false;

    if (id == current_track)
    {
        MH_DEBUG("Removing current track");
        deleting_current = true;

        auto next_it = std::next(id_it);

        if (next_it == m_tracks.end() &&
            loop_status == media::Player::LoopStatus::playlist)
        {
            // Removed the last track, current is the first track and make sure that
            // the player starts playing it
            next_it = m_tracks.begin();
        }

        if (next_it == m_tracks.end())
        {
            current_track.clear();
            // Nothing else to play, stop playback
            Q_EMIT q->endOfTrackList();
        }
        else
        {
            current_track = *next_it;
        }
    }

    do_remove_track(track);

    if (!current_track.isEmpty() and deleting_current)
        go_to(current_track);
}

void TrackListImplementationPrivate::do_remove_track(const Track::Id &id)
{
    Q_Q(TrackListImplementation);
    int removed = m_tracks.removeAll(id);

    if (removed > 0)
    {
        meta_data_cache.remove(id);

        if (shuffle)
            shuffled_tracks.removeAll(id);

        Q_EMIT q->trackRemoved(id);

        // Make sure playback stops if all tracks were removed
        if (m_tracks.isEmpty())
            Q_EMIT q->endOfTrackList();
    }
}

void TrackListImplementationPrivate::go_to(const Track::Id &track)
{
    Q_Q(TrackListImplementation);
    set_current_track(track);
    // Signal the Player instance to go to a specific track for playback
    Q_EMIT q->onGoToTrack(track);
    Q_EMIT q->trackChanged(track);
}

TrackListImplementation::TrackListImplementation(
        const QSharedPointer<media::Engine::MetaDataExtractor> &extractor,
        QObject *parent):
    QObject(parent),
    d_ptr(new TrackListImplementationPrivate(extractor, this))
{
}

TrackListImplementation::~TrackListImplementation()
{
}

void TrackListImplementation::setShuffle(bool shuffle)
{
    Q_D(TrackListImplementation);
    d->shuffle = shuffle;

    if (shuffle) {
        d->shuffled_tracks = d->m_tracks;
        std::random_shuffle(d->shuffled_tracks.begin(),
                            d->shuffled_tracks.end());
    }
}

bool TrackListImplementation::shuffle() const
{
    Q_D(const TrackListImplementation);
    return d->shuffle;
}

QUrl TrackListImplementation::query_uri_for_track(const Track::Id &id)
{
    Q_D(TrackListImplementation);
    const auto it = d->meta_data_cache.find(id);

    if (it == d->meta_data_cache.end())
        return QUrl();

    return it.value().first;
}

Track::MetaData TrackListImplementation::query_meta_data_for_track(const Track::Id &id)
{
    Q_D(TrackListImplementation);
    const auto it = d->meta_data_cache.find(id);

    if (it == d->meta_data_cache.end())
        return Track::MetaData{};

    return it.value().second;
}

void media::TrackListImplementation::add_track_with_uri_at(
        const QUrl &uri,
        const media::Track::Id& position,
        bool make_current)
{
    Q_D(TrackListImplementation);
    MH_TRACE("");
    d->add_track_with_uri_at(uri, position, make_current);
}

void TrackListImplementation::add_tracks_with_uri_at(const QVector<QUrl> &uris,
                                                     const Track::Id &position)
{
    Q_D(TrackListImplementation);
    MH_TRACE("");
    d->add_tracks_with_uri_at(uris, position);
}

bool TrackListImplementation::move_track(const Track::Id &id,
                                         const Track::Id &to)
{
    Q_D(TrackListImplementation);
    MH_TRACE("");
    return d->move_track(id, to);
}

void media::TrackListImplementation::remove_track(const media::Track::Id& id)
{
    Q_D(TrackListImplementation);
    d->remove_track(id);
}

void media::TrackListImplementation::go_to(const media::Track::Id& track)
{
    Q_D(TrackListImplementation);
    MH_TRACE("");
    d->go_to(track);
}

const TrackList::Container &TrackListImplementation::shuffled_tracks() const
{
    Q_D(const TrackListImplementation);
    return d->shuffled_tracks;
}

void media::TrackListImplementation::reset()
{
    Q_D(TrackListImplementation);
    MH_TRACE("");

    d->current_track.clear();

    // Make sure playback stops
    Q_EMIT endOfTrackList();
    // And make sure there is no "current" track
    d->m_tracks.clear();
    d->track_counter = 0;
    d->shuffled_tracks.clear();

    Q_EMIT trackListReset();
}

const TrackList::Container &TrackListImplementation::tracks() const
{
    Q_D(const TrackListImplementation);
    return d->m_tracks;
}

/*
 * NOTE We do not consider the loop status in this function due to the use of it
 * we do in TrackListImplementation::next() (the function is used to know whether we
 * need to wrap when looping is active).
 */
bool TrackListImplementation::hasNext() const
{
    Q_D(const TrackListImplementation);
    const auto n_tracks = d->m_tracks.count();

    if (n_tracks == 0)
        return false;

    // TODO Using current_iterator() makes media-hub crash later. Logic for
    // handling the iterators must be reviewed. As a minimum updates to the
    // track list should update current_track instead of the list being sneakly
    // changed in player_implementation.cpp.
    // To avoid the crash we consider that current_track will be eventually
    // initialized to the first track when current_iterator() gets called.
    if (d->current_track == d->empty_iterator())
    {
        if (n_tracks < 2)
            return false;
        else
            return true;
    }

    if (shuffle())
    {
        auto it = d->get_current_shuffled();
        return ++it != d->shuffled_tracks.end();
    }
    else
    {
        const auto next_track = std::next(d->current_iterator());
        return !d->is_last_track(next_track);
    }
}

/*
 * NOTE We do not consider the loop status in this function due to the use of it
 * we do in TrackListImplementation::previous() (the function is used to know whether we
 * need to wrap when looping is active).
 */
bool TrackListImplementation::hasPrevious() const
{
    Q_D(const TrackListImplementation);
    if (d->m_tracks.isEmpty() || d->current_track == d->empty_iterator())
        return false;

    MH_DEBUG() << "shuffle is" << shuffle();
    if (shuffle())
        return d->get_current_shuffled() != d->shuffled_tracks.begin();
    else
        return d->current_track != d->m_tracks.begin();
}

media::Track::Id media::TrackListImplementation::next()
{
    Q_D(TrackListImplementation);
    MH_TRACE("");
    if (d->m_tracks.isEmpty()) {
        // TODO Change ServiceSkeleton to return with error from DBus call
        MH_ERROR("No tracks, cannot go to next");
        return media::Track::Id{};
    }

    bool go_to_track = false;

    // End of the track reached so loop around to the beginning of the track
    if (d->loop_status == media::Player::LoopStatus::track)
    {
        MH_INFO("Looping on the current track since LoopStatus is set to track");
        go_to_track = true;
    }
    // End of the tracklist reached so loop around to the beginning of the tracklist
    else if (d->loop_status == media::Player::LoopStatus::playlist && not hasNext())
    {
        MH_INFO("Looping on the tracklist since LoopStatus is set to playlist");

        if (shuffle())
        {
            const auto id = *d->shuffled_tracks.begin();
            d->set_current_track(id);
        }
        else
        {
            d->current_track = d->m_tracks.first();
        }
        go_to_track = true;
    }
    else
    {
        if (shuffle())
        {
            auto it = d->get_current_shuffled();
            if (it == d->shuffled_tracks.end()) {
                d->set_current_track(d->shuffled_tracks.first());
                go_to_track = true;
            } else if (++it != d->shuffled_tracks.end()) {
                MH_INFO("Advancing to next track: %s", qUtf8Printable(*it));
                d->set_current_track(*it);
                go_to_track = true;
            }
        }
        else
        {
            const auto it = std::next(d->current_iterator());
            if (not d->is_last_track(it))
            {
                MH_INFO("Advancing to next track: %s", qUtf8Printable(*it));
                d->current_track = *it;
                go_to_track = true;
            }
        }

    }

    const media::Track::Id id = *(d->current_iterator());
    if (go_to_track)
    {
        MH_DEBUG("next track id is %s", qUtf8Printable(id));
        Q_EMIT trackChanged(id);
        // Signal the PlayerImplementation to play the next track
        Q_EMIT onGoToTrack(id);
    }
    else
    {
        // At the end of the tracklist and not set to loop
        MH_INFO("End of tracklist reached");
        Q_EMIT endOfTrackList();
    }

    return id;
}

media::Track::Id media::TrackListImplementation::previous()
{
    Q_D(TrackListImplementation);
    MH_TRACE("");
    if (d->m_tracks.isEmpty()) {
        // TODO Change ServiceSkeleton to return with error from DBus call
        MH_ERROR("No tracks, cannot go to previous");
        return media::Track::Id{};
    }

    bool go_to_track = false;
    // Position is measured in nanoseconds
    const uint64_t max_position = 5 * UINT64_C(1000000000);

    // If we're playing the current track for > max_position time then
    // repeat it from the beginning
    if (d->current_position > max_position)
    {
        MH_INFO("Repeating current track...");
        go_to_track = true;
    }
    // Loop on the current track forever
    else if (d->loop_status == media::Player::LoopStatus::track)
    {
        MH_INFO("Looping on the current track...");
        go_to_track = true;
    }
    // Loop over the whole playlist and repeat
    else if (d->loop_status == media::Player::LoopStatus::playlist && not hasPrevious())
    {
        MH_INFO("Looping on the entire TrackList...");

        if (shuffle())
        {
            const auto id = *std::prev(d->shuffled_tracks.end());
            d->set_current_track(id);
        }
        else
        {
            d->current_track = d->m_tracks.last();
        }

        go_to_track = true;
    }
    else
    {
        if (shuffle())
        {
            auto it = d->get_current_shuffled();
            if (it != d->shuffled_tracks.begin()) {
                d->set_current_track(*(--it));
                go_to_track = true;
            }
        }
        else if (not d->is_first_track(d->current_iterator()))
        {
            // Keep returning the previous track until the first track is reached
            d->current_track = *std::prev(d->current_iterator());
            go_to_track = true;
        }
    }

    const media::Track::Id id = *(d->current_iterator());
    if (go_to_track)
    {
        Q_EMIT trackChanged(id);
        Q_EMIT onGoToTrack(id);
    }
    else
    {
        // At the beginning of the tracklist and not set to loop
        MH_INFO("Beginning of tracklist reached");
        Q_EMIT endOfTrackList();
    }

    return id;
}

const Track::Id &TrackListImplementation::current() const
{
    Q_D(const TrackListImplementation);
    return *(d->current_iterator());
}

void TrackListImplementation::setLoopStatus(Player::LoopStatus loop_status)
{
    Q_D(TrackListImplementation);
    d->loop_status = loop_status;
}

Player::LoopStatus TrackListImplementation::loopStatus() const
{
    Q_D(const TrackListImplementation);
    return d->loop_status;
}

void TrackListImplementation::setCurrentPosition(uint64_t position)
{
    Q_D(TrackListImplementation);
    d->current_position = position;
}

bool TrackListImplementation::canEditTracks() const
{
    return true;
}

const Track::Id &TrackListImplementation::afterEmptyTrack()
{
    static const media::Track::Id id{"/org/mpris/MediaPlayer2/TrackList/NoTrack"};
    return id;
}
