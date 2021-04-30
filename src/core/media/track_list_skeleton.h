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
#ifndef CORE_UBUNTU_MEDIA_TRACK_LIST_SKELETON_H_
#define CORE_UBUNTU_MEDIA_TRACK_LIST_SKELETON_H_

#include "apparmor/ubuntu.h"

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <QMap>

class QDBusConnection;

namespace core
{
namespace ubuntu
{
namespace media
{

class TrackListImplementation;

class TrackListSkeletonPrivate;
class TrackListSkeleton: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.TrackList")
    // FIXME: this should be QDBusObjectPath, and not QString!
    Q_PROPERTY(QStringList Tracks READ tracks)
    Q_PROPERTY(bool CanEditTracks READ canEditTracks)

public:
    TrackListSkeleton(const QDBusConnection &bus,
        const core::ubuntu::media::apparmor::ubuntu::RequestContextResolver::Ptr& request_context_resolver,
        const core::ubuntu::media::apparmor::ubuntu::RequestAuthenticator::Ptr& request_authenticator,
        TrackListImplementation *impl,
        QObject *parent = nullptr);
    ~TrackListSkeleton();

    QStringList tracks() const;
    bool canEditTracks() const;

public Q_SLOTS:
    // FIXME: these should all be QDBusObjectPath, and not QStrings!
    // FIXME: this should take a list, not just a single track;
    // and it should return an aa{sv}, not a{ss}
    QMap<QString,QString> GetTracksMetadata(const QString &id);
    void AddTrack(const QString &uri, const QString &after, bool makeCurrent);
    void RemoveTrack(const QString &id);
    void GoTo(const QString &id);

    // Not in MPRIS:
    QString GetTracksUri(const QString &id);
    void AddTracks(const QStringList &uris, const QString &after);
    void MoveTrack(const QString &id, const QString &to);
    void Reset();

Q_SIGNALS:
    // FIXME: these should all be QDBusObjectPath, and not QStrings!
    Q_SCRIPTABLE void TrackListReplaced(const QStringList &tracks,
                                        const QString &currentTrack);
    // FIXME: For this reason the signature of this signal does not
    // match the MPRIS specification: the metadata is missing.
    Q_SCRIPTABLE void TrackAdded(const QString &id);
    Q_SCRIPTABLE void TrackRemoved(const QString &id);
    // FIXME: the order of parameters is incorrect!
    Q_SCRIPTABLE void TrackMetadataChanged(const QVariantMap &metadata,
                                           const QDBusObjectPath &path);

    // Not in MPRIS:
    Q_SCRIPTABLE void TracksAdded(const QStringList &trackURIs);
    Q_SCRIPTABLE void TrackMoved(const QString &id, const QString &to);
    Q_SCRIPTABLE void TrackChanged(const QString &id);
    Q_SCRIPTABLE void TrackListReset();

private:
    Q_DECLARE_PRIVATE(TrackListSkeleton)
    QScopedPointer<TrackListSkeletonPrivate> d_ptr;
};

}
}
}
#endif // CORE_UBUNTU_MEDIA_PROPERTY_H_
