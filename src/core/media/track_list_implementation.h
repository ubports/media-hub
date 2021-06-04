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

#ifndef CORE_UBUNTU_MEDIA_TRACK_LIST_IMPLEMENTATION_H_
#define CORE_UBUNTU_MEDIA_TRACK_LIST_IMPLEMENTATION_H_

#include "engine.h"
#include "track.h"

#include <QObject>
#include <QScopedPointer>
#include <QVector>

namespace core
{
namespace ubuntu
{
namespace media
{

namespace TrackList
{
    struct Errors
    {
        Errors() = delete;

        struct ExceptionBase: public std::runtime_error {
            ExceptionBase(const QString &msg = {}):
                std::runtime_error(msg.toStdString()) {}
        };

        struct InsufficientPermissionsToAddTrack: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };

        struct FailedToMoveTrack: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };

        struct FailedToFindMoveTrackSource: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };

        struct FailedToFindMoveTrackDest: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };

        struct TrackNotFound: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };
    };

    typedef QVector<Track::Id> Container;
    typedef Container::ConstIterator ConstIterator;
} // namespace TrackList

class TrackListImplementationPrivate;
class TrackListImplementation: public QObject
{
    Q_OBJECT

public:
    TrackListImplementation(
            const QSharedPointer<Engine::MetaDataExtractor> &extractor,
            QObject *parent = nullptr);
    ~TrackListImplementation();

    void setLoopStatus(Player::LoopStatus status);
    Player::LoopStatus loopStatus() const;

    void setShuffle(bool shuffle);
    bool shuffle() const;

    void setCurrentPosition(uint64_t position);

    bool canEditTracks() const;

    QUrl query_uri_for_track(const Track::Id& id);
    Track::MetaData query_meta_data_for_track(const Track::Id& id);

    static const Track::Id &afterEmptyTrack();
    void add_track_with_uri_at(const QUrl &uri, const Track::Id& position, bool make_current);
    void add_tracks_with_uri_at(const QVector<QUrl> &uris, const Track::Id& position);
    bool move_track(const Track::Id& id, const Track::Id& to);
    void remove_track(const Track::Id& id);

    void go_to(const Track::Id& track);
    const TrackList::Container &shuffled_tracks() const;
    void reset();
    const TrackList::Container &tracks() const;

    bool hasNext() const;
    bool hasPrevious() const;
    Track::Id next();
    Track::Id previous();
    const Track::Id& current() const;

Q_SIGNALS:
    void endOfTrackList();
    void onGoToTrack(const media::Track::Id &id);
    void trackAdded(const media::Track::Id &id);
    void tracksAdded(const QVector<QUrl> &tracks);
    void trackRemoved(const media::Track::Id &id);
    void trackMoved(const media::Track::Id &id,
                    const media::Track::Id &to);
    void trackListReset();
    void trackChanged(const media::Track::Id &id);
    void trackListReplaced(const QVector<media::Track::Id> &tracks,
                           const Track::Id &currentTrack);

private:
    Q_DECLARE_PRIVATE(TrackListImplementation)
    QScopedPointer<TrackListImplementationPrivate> d_ptr;
};

}
}
}

#endif // CORE_UBUNTU_MEDIA_TRACK_LIST_IMPLEMENTATION_H_
