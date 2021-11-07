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

#include "track_list_p.h"

#include "dbus_constants.h"
#include "dbus_utils.h"

#include <QDBusAbstractInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>

using namespace lomiri::MediaHub;

class DBusTrackList: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    DBusTrackList(const QDBusConnection &conn, const QString &path,
                  TrackListPrivate *d);

private Q_SLOTS:
    void onTrackAdded(const QString &id) { d->onTrackAdded(id); }
    void onTracksAdded(const QStringList &ids) { d->onTracksAdded(ids); }
    void onTrackMoved(const QString &id, const QString &to) {
        d->onTrackMoved(id, to);
    }
    void onTrackRemoved(const QString &id) { d->onTrackRemoved(id); }
    void onTrackListReset() { d->onTrackListReset(); }
    void onTrackChanged(const QString &id) { d->onTrackChanged(id); }

private:
    TrackListPrivate *d;
};

DBusTrackList::DBusTrackList(const QDBusConnection &conn,
                             const QString &path,
                             TrackListPrivate *d):
    QDBusAbstractInterface(QStringLiteral(MEDIAHUB_SERVICE_NAME),
                           path,
                           MPRIS_TRACKLIST_INTERFACE,
                           conn, nullptr),
    d(d)
{
    QDBusConnection c(conn);

    c.connect(service(), path, interface(), QStringLiteral("TrackAdded"),
              this, SLOT(onTrackAdded(QString)));
    c.connect(service(), path, interface(), QStringLiteral("TracksAdded"),
              this, SLOT(onTracksAdded(QStringList)));
    c.connect(service(), path, interface(), QStringLiteral("TrackRemoved"),
              this, SLOT(onTrackRemoved(QString)));
    c.connect(service(), path, interface(), QStringLiteral("TrackMoved"),
              this, SLOT(onTrackMoved(QString,QString)));
    c.connect(service(), path, interface(), QStringLiteral("TrackListReset"),
              this, SLOT(onTrackListReset()));

    c.connect(service(), path, interface(), QStringLiteral("TrackChanged"),
              this, SLOT(onTrackChanged(QString)));

    // Blocking call to get the initial properties
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral(MEDIAHUB_SERVICE_NAME),
        path,
        QStringLiteral(FDO_PROPERTIES_INTERFACE),
        QStringLiteral("GetAll"));
    msg.setArguments({ QStringLiteral(MPRIS_TRACKLIST_INTERFACE) });
    QDBusMessage reply = c.call(msg);
    if (Q_UNLIKELY(reply.type()) == QDBusMessage::ErrorMessage) {
        qWarning() << "Cannot get tracklist properties:" <<
            reply.errorMessage();
    } else {
        d->initialize(reply.arguments().first().toMap());
    }
}

TrackListPrivate::TrackListPrivate(TrackList *q):
    m_canEditTracks(true),
    m_currentTrack(-1),
    q_ptr(q)
{
}

void TrackListPrivate::createProxy(const QDBusConnection &conn,
                                   const QString &objectPath)
{
    m_proxy.reset(new DBusTrackList(conn, objectPath, this));
}

bool TrackListPrivate::ensureProxy() const
{
    if (!m_proxy) {
        qWarning() << "Operating on disconnected playlist not supported!";
        return false;
    }
    return true;
}

QString TrackListPrivate::remotePos(int index) const
{
    return (index >= 0 && index < m_trackIds.count()) ?
        m_trackIds[index] :
        QStringLiteral("/org/mpris/MediaPlayer2/TrackList/NoTrack");
}

void TrackListPrivate::initialize(const QVariantMap &properties)
{
    m_canEditTracks =
        properties.value(QStringLiteral("CanEditTracks"), false).toBool();
    // The tracklist is always empty here, no need to retrieve it
}

void TrackListPrivate::addTrackWithUriAt(const QUrl &uri, int position,
                                         bool makeCurrent)
{
    if (!ensureProxy()) return;

    QDBusPendingCall call =
        m_proxy->asyncCall(QStringLiteral("AddTrack"),
                           uri.toString(), remotePos(position), makeCurrent);
    m_indexForNextAddition = position;
    m_urisForNextAddition = { uri };
    DBusUtils::waitForFinished(call);
}

void TrackListPrivate::addTracksWithUriAt(const QVector<QUrl> &uris,
                                          int position)
{
    if (!ensureProxy()) return;

    QStringList urisAsStrings;
    for (const QUrl &uri: uris) {
        urisAsStrings.append(uri.toString());
    }
    QDBusPendingCall call =
        m_proxy->asyncCall(QStringLiteral("AddTracks"),
                           urisAsStrings, remotePos(position));
    m_indexForNextAddition = position;
    m_urisForNextAddition = uris;
    DBusUtils::waitForFinished(call);
}

void TrackListPrivate::moveTrack(int index, int to)
{
    if (!ensureProxy()) return;

    QDBusPendingCall call =
        m_proxy->asyncCall(QStringLiteral("MoveTrack"),
                           remotePos(index), remotePos(to));
    DBusUtils::waitForFinished(call);
}

void TrackListPrivate::removeTrack(int index)
{
    if (!ensureProxy()) return;

    QDBusPendingCall call =
        m_proxy->asyncCall(QStringLiteral("RemoveTrack"), remotePos(index));
    DBusUtils::waitForFinished(call);
}

void TrackListPrivate::goTo(int index)
{
    if (!ensureProxy()) return;

    QDBusPendingCall call =
        m_proxy->asyncCall(QStringLiteral("GoTo"), remotePos(index));
    DBusUtils::waitForFinished(call);
}

void TrackListPrivate::reset()
{
    if (!ensureProxy()) return;

    QDBusPendingCall call = m_proxy->asyncCall(QStringLiteral("Reset"));
    DBusUtils::waitForFinished(call);
}

void TrackListPrivate::onTrackAdded(const QString &id)
{
    onTracksAdded({id});
}

void TrackListPrivate::onTracksAdded(const QStringList &ids)
{
    Q_Q(TrackList);
    // TODO: rewrite once we change the service to be spec-compliant
    if (ids.count() != m_urisForNextAddition.count()) {
        qWarning() << "Mismatching counters for TrackAdded signal";
        return;
    }
    if (m_indexForNextAddition < 0 || m_indexForNextAddition > m_trackIds.count()) {
        m_indexForNextAddition = m_trackIds.count();
    }
    for (int i = 0; i < m_urisForNextAddition.count(); i++) {
        const QUrl &uri = m_urisForNextAddition[i];
        m_tracks.insert(m_indexForNextAddition + i, Track(uri));
        m_trackIds.insert(m_indexForNextAddition + i, ids[i]);
    }

    Q_EMIT q->tracksAdded(m_indexForNextAddition,
                          m_indexForNextAddition + ids.count() - 1);
}

void TrackListPrivate::onTrackMoved(const QString &id, const QString &to)
{
    Q_Q(TrackList);
    int idIndex = m_trackIds.indexOf(id);
    int toIndex = m_trackIds.indexOf(to);
    m_trackIds.move(idIndex, toIndex);
    m_tracks.move(idIndex, toIndex);
    Q_EMIT q->trackMoved(idIndex, toIndex);
}

void TrackListPrivate::onTrackRemoved(const QString &id)
{
    Q_Q(TrackList);
    int idIndex = m_trackIds.indexOf(id);
    m_trackIds.remove(idIndex);
    m_tracks.remove(idIndex);
    Q_EMIT q->trackRemoved(idIndex);
}

void TrackListPrivate::onTrackListReset()
{
    Q_Q(TrackList);
    m_trackIds.clear();
    m_tracks.clear();
    m_currentTrack = -1;
    Q_EMIT q->trackListReset();
}

void TrackListPrivate::onTrackChanged(const QString &id)
{
    Q_Q(TrackList);
    m_currentTrack = m_trackIds.indexOf(id);
    Q_EMIT q->currentTrackChanged();
}

TrackList::TrackList(QObject *parent):
    QObject(parent),
    d_ptr(new TrackListPrivate(this))
{
}

TrackList::~TrackList() = default;

const QVector<Track> &TrackList::tracks() const
{
    Q_D(const TrackList);
    return d->m_tracks;
}

bool TrackList::canEditTracks() const
{
    Q_D(const TrackList);
    return d->m_canEditTracks;
}

int TrackList::currentTrack() const
{
    Q_D(const TrackList);
    return d->m_currentTrack;
}

void TrackList::addTrackWithUriAt(const QUrl &uri, int position,
                                  bool makeCurrent)
{
    Q_D(TrackList);
    d->addTrackWithUriAt(uri, position, makeCurrent);
}

void TrackList::addTracksWithUriAt(const QVector<QUrl> &uris, int position)
{
    Q_D(TrackList);
    d->addTracksWithUriAt(uris, position);
}

void TrackList::moveTrack(int index, int to)
{
    Q_D(TrackList);
    d->moveTrack(index, to);
}

void TrackList::removeTrack(int index)
{
    Q_D(TrackList);
    d->removeTrack(index);
}

void TrackList::goTo(int index)
{
    Q_D(TrackList);
    d->goTo(index);
}

void TrackList::reset()
{
    Q_D(TrackList);
    d->reset();
}

#include "track_list.moc"
