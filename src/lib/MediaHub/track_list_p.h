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

#ifndef LOMIRI_MEDIAHUB_TRACK_LIST_P_H
#define LOMIRI_MEDIAHUB_TRACK_LIST_P_H

#include "track_list.h"

class DBusTrackList;

class QDBusConnection;
class QDBusPendingCall;

namespace lomiri {
namespace MediaHub {

class TrackListPrivate {
    Q_DECLARE_PUBLIC(TrackList)

public:
    TrackListPrivate(TrackList *q);

    void createProxy(const QDBusConnection &conn, const QString &objectPath);

    bool ensureProxy() const;
    // TODO: change to return QDBusObjectPath
    QString remotePos(int index) const;
    void watchErrors(const QDBusPendingCall &call);

    void initialize(const QVariantMap &properties);

    void addTrackWithUriAt(const QUrl &uri, int position, bool makeCurrent);
    void addTracksWithUriAt(const QVector<QUrl> &uris, int position);
    void moveTrack(int index, int to);
    void removeTrack(int index);
    void goTo(int index);
    void reset();

    void onTrackAdded(const QString &id);
    void onTracksAdded(const QStringList &ids);
    void onTrackMoved(const QString &id, const QString &to);
    void onTrackRemoved(const QString &id);
    void onTrackListReset();
    void onTrackChanged(const QString &id);

private:
    QVector<Track> m_tracks;
    QVector<QString> m_trackIds;
    bool m_canEditTracks;
    int m_currentTrack;

    // TODO: remove once service is spec-compliant
    int m_indexForNextAddition;
    QVector<QUrl> m_urisForNextAddition;


    QScopedPointer<DBusTrackList> m_proxy;
    TrackList *q_ptr;
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_TRACK_LIST_P_H
