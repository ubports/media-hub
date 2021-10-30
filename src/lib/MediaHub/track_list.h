/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
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
 */

#ifndef LOMIRI_MEDIAHUB_TRACK_LIST_H
#define LOMIRI_MEDIAHUB_TRACK_LIST_H

#include "global.h"
#include "track.h"

#include <QObject>
#include <QScopedPointer>
#include <QVector>

namespace lomiri {
namespace MediaHub {

class PlayerPrivate;

class TrackListPrivate;
class MH_EXPORT TrackList: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TrackList)

public:
    TrackList(QObject *parent = nullptr);
    ~TrackList();

    /** If set to false, calling add_track_with_uri_at or remove_track will have no effect. */
    Q_PROPERTY(bool canEditTracks READ canEditTracks
               NOTIFY canEditTracksChanged)
    Q_PROPERTY(int currentTrack READ currentTrack WRITE goTo
               NOTIFY currentTrackChanged)

    const QVector<Track> &tracks() const;

    bool canEditTracks() const;
    int currentTrack() const;

    /*
     * Unused:
     * - query_meta_data_for_track()
     * - query_uri_for_track() (can use `tracks()[index].url()`)
     * - has_next()
     * - has_previous()
     * - next()
     * - previous()
     * - on_track_list_replaced() // Never emitted by the service
     *
     * FIXME: remove this comment?
     * We don't want to allow other processes to modify our own playlist, and
     * QtMultimedia does not provide a way to track another process's playlist.
     * Therefore it looks like we can get away without all them.
     */

    /** Adds a URI into the TrackList. */
    void addTrackWithUriAt(const QUrl &uri, int position, bool makeCurrent);

    /** Adds a list of URIs into the TrackList. */
    void addTracksWithUriAt(const QVector<QUrl> &uris, int position);

    /** Moves track at 'index' from its old position in the TrackList to new position. */
    void moveTrack(int index, int to);

    /** Removes a Track from the TrackList. */
    void removeTrack(int index);

    /** Skip to the specified Track. */
    void goTo(int index);

    /** Clears and resets the TrackList to the same as a newly constructed instance. */
    void reset();

Q_SIGNALS:
    void canEditTracksChanged();
    void currentTrackChanged(); // D-Bus: TrackChanged

    void tracksAdded(int start, int end);
    void trackRemoved(int index);
    void trackMoved(int index, int to);
    void trackListReset();

private:
    friend class PlayerPrivate;
    Q_DECLARE_PRIVATE(TrackList)
    QScopedPointer<TrackListPrivate> d_ptr;
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_TRACK_LIST_H
