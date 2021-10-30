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

#include "player.h"

#include "dbus_constants.h"
#include "error_p.h"
#include "track_list_p.h"
#include "video_sink_p.h"

#include <QDBusAbstractInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QHash>
#include <QMetaEnum>
#include <QVariantMap>
#include <QDebug>
#include <functional>

using namespace lomiri::MediaHub;

typedef std::function<void(const QDBusMessage &)> MethodCb;
typedef std::function<void()> VoidMethodCb;

class DBusService: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    DBusService();

    QDBusMessage createSession();
    void destroySession(const QString &uuid);
};

class DBusPlayer: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    DBusPlayer(const QDBusConnection &conn, const QString &path,
               PlayerPrivate *d);

public Q_SLOTS:
    void onPropertiesChanged(const QString &interface,
                             const QVariantMap &changed,
                             const QStringList &invalidated);
    void onVideoDimensionChanged(quint32 height, quint32 width);
    void onError(quint16 code);

private:
    PlayerPrivate *d;
};

namespace lomiri {
namespace MediaHub {

class PlayerPrivate
{
    Q_DECLARE_PUBLIC(Player)

public:
    PlayerPrivate(Player *q);
    ~PlayerPrivate();

    // Internal methods
    void updateProperties(const QVariantMap &properties);
    void onVideoDimensionChanged(quint32 height, quint32 width);
    void onError(quint16 dbusCode);
    void watchErrors(const QDBusPendingCall &call);
    void onSuccessfulCompletion(const QDBusPendingCall &call,
                                MethodCb callback);
    void setProperty(const QString &name, const QVariant &value,
                     VoidMethodCb callback = [](){});
    QVariant getProperty(const QString &name) const;

    // Implementation of public methods
    void setTrackList(TrackList *trackList);
    void call(const QString &method,
              const QVariant &arg1 = QVariant(),
              const QVariant &arg2 = QVariant());
    VideoSink &createGLTextureVideoSink(uint32_t textureId);

private:
    bool m_canPlay = false;
    bool m_canPause = false;
    bool m_canSeek = false;
    bool m_canGoPrevious = false;
    bool m_canGoNext = false;
    bool m_isVideoSource = false;
    bool m_isAudioSource = false;
    bool m_shuffle = false;
    Player::Volume m_volume = 1.0;
    Player::Orientation m_orientation = Player::Rotate0;
    Track::MetaData m_metaData;

    Player::PlaybackRate m_playbackRate = 1.0;
    Player::PlaybackRate m_minimumPlaybackRate = 1.0;
    Player::PlaybackRate m_maximumPlaybackRate = 1.0;

    Player::PlaybackStatus m_playbackStatus = Player::Null;

    AVBackend::Backend m_backend = AVBackend::Backend::None;
    QHash<quint32, VideoSink*> m_videoSinks;

    QString m_uuid;
    DBusService m_service;
    QDBusServiceWatcher m_serviceWatcher;
    QScopedPointer<DBusPlayer> m_proxy;
    TrackList *m_trackList = nullptr;
    VideoSinkFactory m_videoSinkFactory;
    Player *q_ptr;
};

} // namespace MediaHub
} // namespace lomiri

DBusService::DBusService():
    QDBusAbstractInterface(QStringLiteral(MEDIAHUB_SERVICE_NAME),
                           QStringLiteral(MEDIAHUB_SERVICE_PATH),
                           MEDIAHUB_SERVICE_INTERFACE,
                           QDBusConnection::sessionBus(),
                           nullptr)
{
}

QDBusMessage DBusService::createSession()
{
    return call(QStringLiteral("CreateSession"));
}

void DBusService::destroySession(const QString &uuid)
{
    call(QDBus::NoBlock, QStringLiteral("DestroySession"), uuid);
}

DBusPlayer::DBusPlayer(const QDBusConnection &conn,
                       const QString &objectPath,
                       PlayerPrivate *d):
    QDBusAbstractInterface(QStringLiteral(MEDIAHUB_SERVICE_NAME),
                           objectPath,
                           MPRIS_PLAYER_INTERFACE,
                           conn,
                           nullptr),
    d(d)
{
}

void DBusPlayer::onPropertiesChanged(const QString &interface,
                                     const QVariantMap &changed,
                                     const QStringList &invalidated)
{
    if (interface == QStringLiteral(MPRIS_PLAYER_INTERFACE)) {
        d->updateProperties(changed);
        // we know that our service will never invalidate properties
        Q_UNUSED(invalidated);
    }
}

void DBusPlayer::onVideoDimensionChanged(quint32 height, quint32 width)
{
    d->onVideoDimensionChanged(height, width);
}

void DBusPlayer::onError(quint16 code)
{
    d->onError(code);
}

PlayerPrivate::PlayerPrivate(Player *q):
    m_serviceWatcher(m_service.service(), m_service.connection()),
    q_ptr(q)
{
    {
        QDBusMessage reply = m_service.createSession();
        if (Q_UNLIKELY(reply.type() == QDBusMessage::ErrorMessage)) {
            qWarning() << "Failed to create session:" << reply.errorMessage();
            return;
        }
        const QString path = reply.arguments()[0].value<QDBusObjectPath>().path();
        m_uuid = reply.arguments()[1].toString();

        m_proxy.reset(new DBusPlayer(m_service.connection(), path, this));
    }

    // We'll connect the result later down
    QDBusPendingReply<quint32> keyCall =
        m_proxy->asyncCall(QStringLiteral("Key"));

    QObject::connect(&m_serviceWatcher,
                     &QDBusServiceWatcher::serviceRegistered,
                     q, &Player::serviceReconnected);
    QObject::connect(&m_serviceWatcher,
                     &QDBusServiceWatcher::serviceUnregistered,
                     q, &Player::serviceDisconnected);

    QDBusConnection c(m_service.connection());

    const QString service = m_proxy->service();
    const QString path = m_proxy->path();
    const QString interface = m_proxy->interface();

    c.connect(service, path, QStringLiteral(FDO_PROPERTIES_INTERFACE), QStringLiteral("PropertiesChanged"),
              m_proxy.data(), SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    c.connect(service, path, interface, QStringLiteral("Seeked"),
              q, SLOT(seekedTo(quint64)));
    c.connect(service, path, interface, QStringLiteral("AboutToFinish"),
              q, SLOT(aboutToFinish()));
    c.connect(service, path, interface, QStringLiteral("EndOfStream"),
              q, SLOT(endOfStream()));
    c.connect(service, path, interface, QStringLiteral("VideoDimensionChanged"),
              m_proxy.data(), SLOT(onVideoDimensionChanged(quint32,quint32)));
    c.connect(service, path, interface, QStringLiteral("Error"),
              m_proxy.data(), SLOT(onError(quint16)));
    c.connect(service, path, interface, QStringLiteral("Buffering"),
              q, SLOT(bufferingChanged(int)));

    // Blocking call to get the initial properties
    QDBusMessage msg = QDBusMessage::createMethodCall(
        service, path,
        QStringLiteral(FDO_PROPERTIES_INTERFACE),
        QStringLiteral("GetAll"));
    msg.setArguments({ interface });
    QDBusMessage reply = c.call(msg);
    if (Q_UNLIKELY(reply.type()) == QDBusMessage::ErrorMessage) {
        qWarning() << "Cannot get player properties:" <<
            reply.errorMessage();
    } else {
        QDBusArgument arg = reply.arguments().first().value<QDBusArgument>();
        updateProperties(qdbus_cast<QVariantMap>(arg));
    }

    // Collect the result from the Key() call
    keyCall.waitForFinished();
    if (Q_UNLIKELY(keyCall.isError())) {
        qWarning() << "Key() call failed:" << keyCall.error().message();
    } else {
        PlayerKey key = keyCall.value();
        m_videoSinkFactory = createVideoSinkFactory(key, m_backend);
    }
}

PlayerPrivate::~PlayerPrivate()
{
    m_service.destroySession(m_uuid);
}

void PlayerPrivate::updateProperties(const QVariantMap &properties)
{
    Q_Q(Player);

    bool controlsChanged = false;
    bool sourceTypeChanged = false;

    for (auto i = properties.begin(); i != properties.end(); i++) {
        const QString &name = i.key();
        if (name == "CanPlay") {
            m_canPlay = i.value().toBool();
            controlsChanged = true;
        } else if (name == "CanPause") {
            m_canPause = i.value().toBool();
            controlsChanged = true;
        } else if (name == "CanSeek") {
            m_canSeek = i.value().toBool();
            controlsChanged = true;
        } else if (name == "CanGoPrevious") {
            m_canGoPrevious = i.value().toBool();
            controlsChanged = true;
        } else if (name == "CanGoNext") {
            m_canGoNext = i.value().toBool();
            controlsChanged = true;
        } else if (name == "IsVideoSource") {
            m_isVideoSource = i.value().toBool();
            sourceTypeChanged = true;
        } else if (name == "IsAudioSource") {
            m_isAudioSource = i.value().toBool();
            sourceTypeChanged = true;
        } else if (name == "PlaybackStatus") {
            bool ok = false;
            const QString status = i.value().toString();
            int v = QMetaEnum::fromType<Player::PlaybackStatus>().
                keyToValue(status.toUtf8().constData(), &ok);
            if (Q_UNLIKELY(!ok)) {
                qWarning() << "Unknown player status" << status;
            } else {
                m_playbackStatus = static_cast<Player::PlaybackStatus>(v);
                Q_EMIT q->playbackStatusChanged();
            }
        } else if (name == "Metadata") {
            QDBusArgument arg = i.value().value<QDBusArgument>();
            // strip the mpris:trackid
            const QVariantMap dbusMap = qdbus_cast<QVariantMap>(arg);
            m_metaData.clear();
            for (auto i = dbusMap.begin(); i != dbusMap.end(); i++) {
                if (i.key() == QStringLiteral("mpris:trackid")) continue;
                m_metaData.insert(i.key(), i.value());
            }
            Q_EMIT q->metaDataForCurrentTrackChanged();
        } else if (name == "Orientation") {
            m_orientation =
                static_cast<Player::Orientation>(i.value().toInt());
            Q_EMIT q->orientationChanged();
        } else if (name == "TypedBackend") {
            m_backend =
                static_cast<AVBackend::Backend>(i.value().toInt());
        }
    }

    if (controlsChanged) {
        Q_EMIT q->controlsChanged();
    }
    if (sourceTypeChanged) {
        Q_EMIT q->sourceTypeChanged();
    }
}

void PlayerPrivate::onVideoDimensionChanged(quint32 height, quint32 width)
{
    Q_Q(Player);
    Q_EMIT q->videoDimensionChanged(QSize(width, height));
}

void PlayerPrivate::onError(quint16 dbusCode)
{
    Q_Q(Player);
    Error error = errorFromApiCode(dbusCode);
    if (error.isError()) {
        Q_EMIT q->errorOccurred(error);
    }
}

void PlayerPrivate::watchErrors(const QDBusPendingCall &call)
{
    Q_Q(Player);
    auto watcher = new QDBusPendingCallWatcher(call);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     q, [this](QDBusPendingCallWatcher *call) {
        Q_Q(Player);
        call->deleteLater();
        QDBusPendingReply<void> reply(*call);
        if (reply.isError()) {
            Q_EMIT q->errorOccurred(errorFromDBus(reply.reply()));
        }
    });
}

void PlayerPrivate::onSuccessfulCompletion(const QDBusPendingCall &call,
                                           MethodCb callback)
{
    Q_Q(Player);
    auto watcher = new QDBusPendingCallWatcher(call);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     q, [this, callback](QDBusPendingCallWatcher *call) {
        Q_Q(Player);
        call->deleteLater();
        const QDBusMessage reply = QDBusPendingReply<void>(*call).reply();
        if (Q_UNLIKELY(reply.type()) == QDBusMessage::ErrorMessage) {
            Q_EMIT q->errorOccurred(errorFromDBus(reply));
        } else if (callback) {
            callback(reply);
        }
    });
}

void PlayerPrivate::setProperty(const QString &name, const QVariant &value,
                                VoidMethodCb callback)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_proxy->service(),
        m_proxy->path(),
        QStringLiteral(FDO_PROPERTIES_INTERFACE),
        QStringLiteral("Set"));
    msg.setArguments({
        QStringLiteral(MPRIS_PLAYER_INTERFACE),
        name,
        value,
    });
    QDBusPendingCall call = m_proxy->connection().asyncCall(msg);
    onSuccessfulCompletion(call, [callback](const QDBusMessage &) {
        callback();
    });
}

/* This makes a blocking call: use sparingly! */
QVariant PlayerPrivate::getProperty(const QString &name) const
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_proxy->service(),
        m_proxy->path(),
        QStringLiteral(FDO_PROPERTIES_INTERFACE),
        QStringLiteral("Get"));
    msg.setArguments({
        QStringLiteral(MPRIS_PLAYER_INTERFACE),
        name,
    });
    QVariant ret;
    QDBusReply<QVariant> reply = m_proxy->connection().call(msg);
    if (Q_LIKELY(reply.isValid())) {
        ret = reply.value();
    } else {
        qWarning() << "Error getting property" << name << reply.error();
    }
    return ret;
}

void PlayerPrivate::setTrackList(TrackList *trackList)
{
    if (trackList != m_trackList) {
        if (trackList) {
            trackList->d_ptr->createProxy(m_proxy->connection(),
                                          m_proxy->path() + "/TrackList");
        }
        m_trackList = trackList;
    }
}

void PlayerPrivate::call(const QString &method,
                         const QVariant &arg1, const QVariant &arg2)
{
    QDBusPendingCall call = m_proxy->asyncCall(method, arg1, arg2);
    watchErrors(call);
}

VideoSink &PlayerPrivate::createGLTextureVideoSink(uint32_t textureId)
{
    Q_Q(Player);
    auto i = m_videoSinks.find(textureId);
    if (i == m_videoSinks.end()) {
        VideoSink *videoSink = m_videoSinkFactory(textureId, q);
        i = m_videoSinks.insert(textureId, videoSink);
    }
    QDBusMessage reply =
        m_proxy->call(QStringLiteral("CreateVideoSink"), textureId);
    if (Q_UNLIKELY(reply.type() == QDBusMessage::ErrorMessage)) {
        Q_EMIT q->errorOccurred(errorFromDBus(reply));
    }
    return *i.value();
}

Player::Player(QObject *parent):
    QObject(parent),
    d_ptr(new PlayerPrivate(this))
{
    qRegisterMetaType<Error>("Error");
    QMetaType::registerConverter(&Error::toString);
    QMetaType::registerEqualsComparator<Error>();
    qRegisterMetaType<Volume>("Volume");
    qRegisterMetaType<QVariantMap>("Track::MetaData");
    qRegisterMetaType<PlaybackRate>("PlaybackRate");
    qDBusRegisterMetaType<Headers>();
}

Player::~Player() = default;

QString Player::uuid() const
{
    Q_D(const Player);
    return d->m_uuid;
}

void Player::setTrackList(TrackList *trackList)
{
    Q_D(Player);
    d->setTrackList(trackList);
}

TrackList *Player::trackList() const
{
    Q_D(const Player);
    return d->m_trackList;
}

VideoSink &Player::createGLTextureVideoSink(uint32_t textureId)
{
    Q_D(Player);
    return d->createGLTextureVideoSink(textureId);
}

void Player::openUri(const QUrl &uri, const Headers &headers)
{
    Q_D(Player);
    d->call(QStringLiteral("OpenUriExtended"),
            uri.toString(), QVariant::fromValue(headers));
}

void Player::goToNext()
{
    Q_D(Player);
    d->call(QStringLiteral("Next"));
}

void Player::goToPrevious()
{
    Q_D(Player);
    d->call(QStringLiteral("Previous"));
}

void Player::play()
{
    Q_D(Player);
    d->call(QStringLiteral("Play"));
}

void Player::pause()
{
    Q_D(Player);
    d->call(QStringLiteral("Pause"));
}

void Player::stop()
{
    Q_D(Player);
    d->call(QStringLiteral("Stop"));
}

void Player::seekTo(uint64_t microseconds)
{
    Q_D(Player);
    d->call(QStringLiteral("Seek"), quint64(microseconds));
}

bool Player::canPlay() const
{
    Q_D(const Player);
    return d->m_canPlay;
}

bool Player::canPause() const
{
    Q_D(const Player);
    return d->m_canPause;
}

bool Player::canSeek() const
{
    Q_D(const Player);
    return d->m_canSeek;
}

bool Player::canGoPrevious() const
{
    Q_D(const Player);
    return d->m_canGoPrevious;
}

bool Player::canGoNext() const
{
    Q_D(const Player);
    return d->m_canGoNext;
}

bool Player::isVideoSource() const
{
    Q_D(const Player);
    return d->m_isVideoSource;
}

bool Player::isAudioSource() const
{
    Q_D(const Player);
    return d->m_isAudioSource;
}

Player::PlaybackStatus Player::playbackStatus() const
{
    Q_D(const Player);
    return d->m_playbackStatus;
}

void Player::setPlaybackRate(PlaybackRate rate)
{
    Q_D(Player);
    d->setProperty(QStringLiteral("PlaybackRate"), rate,
                   [=]() { d->m_playbackRate = rate; });
}

Player::PlaybackRate Player::playbackRate() const
{
    Q_D(const Player);
    return d->m_playbackRate;
}

void Player::setShuffle(bool shuffle)
{
    Q_D(Player);
    d->setProperty(QStringLiteral("Shuffle"), shuffle,
                   [=]() { d->m_shuffle = shuffle; });
}

bool Player::shuffle() const
{
    Q_D(const Player);
    return d->m_shuffle;
}

void Player::setVolume(Volume volume)
{
    Q_D(Player);
    d->setProperty(QStringLiteral("Volume"), volume,
                   // TODO: rewrite when the service gains signals
                   [=]() { d->m_volume = volume; });
}

Player::Volume Player::volume() const
{
    Q_D(const Player);
    return d->m_volume;
}

Track::MetaData Player::metaDataForCurrentTrack() const
{
    Q_D(const Player);
    return d->m_metaData;
}

Player::PlaybackRate Player::minimumPlaybackRate() const
{
    Q_D(const Player);
    return d->m_minimumPlaybackRate;
}

Player::PlaybackRate Player::maximumPlaybackRate() const
{
    Q_D(const Player);
    return d->m_maximumPlaybackRate;
}

quint64 Player::position() const
{
    Q_D(const Player);
    return d->getProperty(QStringLiteral("Position")).toULongLong();
}

quint64 Player::duration() const
{
    Q_D(const Player);
    return d->getProperty(QStringLiteral("Duration")).toULongLong();
}

Player::Orientation Player::orientation() const
{
    Q_D(const Player);
    return d->m_orientation;
}

void Player::setLoopStatus(LoopStatus loopStatus)
{
    Q_D(Player);
    QString status;
    switch (loopStatus) {
    case Player::LoopNone: status = QStringLiteral("None"); break;
    case Player::LoopTrack: status = QStringLiteral("Track"); break;
    case Player::LoopPlaylist: status = QStringLiteral("Playlist"); break;
    default:
        qWarning() << "Unknown loop status enum:" << loopStatus;
        return;
    }
    d->setProperty(QStringLiteral("LoopStatus"), status);
}

Player::LoopStatus Player::loopStatus() const
{
    Q_D(const Player);
    // TODO: add to PropertiesChanged
    const QString status =
        d->getProperty(QStringLiteral("LoopStatus")).toString();
    if (status == "None") return Player::LoopNone;
    if (status == "Track") return Player::LoopTrack;
    if (status == "Playlist") return Player::LoopPlaylist;
    qWarning() << "Unknown player status" << status;
    return Player::LoopNone;
}

void Player::setAudioStreamRole(AudioStreamRole role)
{
    Q_D(Player);
    d->setProperty(QStringLiteral("AudioStreamRole"), role);
}

Player::AudioStreamRole Player::audioStreamRole() const
{
    Q_D(const Player);
    // TODO: add to PropertiesChanged
    return static_cast<AudioStreamRole>(
        d->getProperty(QStringLiteral("AudioStreamRole")).toInt());
}

#include "player.moc"
