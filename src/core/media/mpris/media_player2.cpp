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

#include "mpris/media_player2.h"

#include "dbus_property_notifier.h"
#include "logging.h"
#include "player_implementation.h"

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QMetaEnum>
#include <QString>
#include <QStringList>

namespace media = core::ubuntu::media;

using namespace mpris;

namespace mpris {

const QString objectPath = QStringLiteral("/org/mpris/MediaPlayer2");

// Models interface org.mpris.MediaPlayer2, see:
//   http://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html
// for detailed documentation
class RootAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ falseProperty)
    Q_PROPERTY(bool Fullscreen READ falseProperty)
    Q_PROPERTY(bool CanSetFullscreen READ falseProperty)
    Q_PROPERTY(bool CanRaise READ falseProperty)
    Q_PROPERTY(bool HasTrackList READ falseProperty)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)

public:
    RootAdaptor(QObject *parent): QDBusAbstractAdaptor(parent) {}
    virtual ~RootAdaptor() = default;

    bool falseProperty() const { return false; }
    QString identity() const { return QStringLiteral("core::media::Hub"); }
    QString desktopEntry() const { return QStringLiteral("mediaplayer-app"); }
    QStringList supportedUriSchemes() const { return {}; }
    QStringList supportedMimeTypes() const {
        return {"audio/mpeg3", "video/mpeg4"};
    }

public Q_SLOTS:
    void Raise() {}
    void Quit() {}
};

// Models interface org.mpris.MediaPlayer2.Playlists, see:
//   http://specifications.freedesktop.org/mpris-spec/latest/Playlists_Interface.html
// for detailed documentation
class PlaylistsAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Playlists")
    Q_PROPERTY(quint32 PlaylistCount READ playlistCount)
    Q_PROPERTY(QStringList Orderings READ orderings)
    // FIXME: There's also an ActivePlaylist property in the specs

public:
    PlaylistsAdaptor(QObject *parent): QDBusAbstractAdaptor(parent) {}
    virtual ~PlaylistsAdaptor() = default;

    quint32 playlistCount() const { return 0; }
    QStringList orderings() const { return {"Alphabetical"}; }

public Q_SLOTS:
    void ActivePlaylist() {}
    void GetPlaylists() {}
};

class PlayerAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanControl READ canControl)
    Q_PROPERTY(bool IsVideoSource READ isVideoSource)
    Q_PROPERTY(bool IsAudioSource READ isAudioSource)
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double PlaybackRate READ playbackRate WRITE setPlaybackRate)
    Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(qint64 Position READ position)
    Q_PROPERTY(qint64 Duration READ duration)
    Q_PROPERTY(qint16 TypedBackend READ backend)
    Q_PROPERTY(qint16 Orientation READ orientation)
    Q_PROPERTY(qint16 Lifetime READ lifetime)
    Q_PROPERTY(qint16 AudioStreamRole READ audioStreamRole)
    Q_PROPERTY(qint16 TypedLoopStatus READ typedLoopStatus
               WRITE setTypedLoopStatus)

public:
    enum LoopStatus {
        None = media::Player::LoopStatus::none,
        Track = media::Player::LoopStatus::track,
        Playlist = media::Player::LoopStatus::playlist,
    };
    Q_ENUM(LoopStatus)

    PlayerAdaptor(const QDBusConnection &connection, QObject *parent);

    void setPlayer(media::PlayerImplementation *impl);
    media::PlayerImplementation *player() { return m_player; }
    const media::PlayerImplementation *player() const { return m_player; }

    bool canPlay() const { return m_player ? m_player->canPlay() : false; }
    bool canPause() const { return m_player ? m_player->canPause() : false; }
    bool canSeek() const { return m_player ? m_player->canSeek() : false; }
    bool canGoPrevious() const { return m_player ? m_player->canGoPrevious() : false; }
    bool canGoNext() const { return m_player ? m_player->canGoNext() : false; }
    bool canControl() const { return m_player ? true : false; }
    bool isVideoSource() const { return m_player ? m_player->isVideoSource() : false; }
    bool isAudioSource() const { return m_player ? m_player->isAudioSource() : false; }
    QString playbackStatus() const;
    void setLoopStatus(const QString &status);
    QString loopStatus() const;
    void setTypedLoopStatus(qint16 status);
    qint16 typedLoopStatus() const;
    void setPlaybackRate(double rate) { if (m_player) m_player->setPlaybackRate(rate); }
    double playbackRate() const { return m_player ? m_player->playbackRate() : 1.0; }
    void setShuffle(bool shuffle) { if (m_player) m_player->setShuffle(shuffle); }
    bool shuffle() const { return m_player ? m_player->shuffle() : false; }
    QVariantMap metadata() const { return m_player ? m_player->metadataForCurrentTrack() : QVariantMap(); }
    void setVolume(double volume) { if (m_player) m_player->setVolume(volume); }
    double volume() const { return m_player ? m_player->volume() : 1.0; }
    double minimumRate() const { return m_player ? m_player->minimumRate() : 1.0; }
    double maximumRate() const { return m_player ? m_player->maximumRate() : 1.0; }
    qint64 position() const { return m_player ? m_player->position() : 0; }
    qint64 duration() const { return m_player ? m_player->duration() : 0; }
    qint16 backend() const { return m_player ? m_player->backend() : 0; }
    qint16 orientation() const { return m_player ? m_player->orientation() : 0; }
    qint16 lifetime() const { return m_player ? m_player->lifetime() : 0; }
    qint16 audioStreamRole() const { return m_player ? m_player->audioStreamRole() : 0; }

public Q_SLOTS:
    void Next() { if (m_player) m_player->next(); }
    void Previous() { if (m_player) m_player->previous(); }
    void Pause() { if (m_player) m_player->pause(); }
    void PlayPause();
    void Stop() { if (m_player) m_player->stop(); }
    void Play() { if (m_player) m_player->play(); }
    void Seek(quint64 microSeconds);
    void SetPosition(const QDBusObjectPath &, quint64) {} // Never implemented
    /* The OpenUri should not return anything, but since the previous
     * implementation was returning a boolean, let's keep doing that. */
    void OpenUri(const QDBusMessage &);

Q_SIGNALS:
    void Seeked(quint64 microSeconds);
    void AboutToFinish();
    void EndOfStream();
    void PlaybackStatusChanged(quint16 status); // TODO: remove, we have PropertiesChanged already
    void VideoDimensionChanged(quint32 height, quint32 width);
    void Error(qint16 code);
    void Buffering(int percent); // TODO: set a fixed type

private:
    media::PlayerImplementation *m_player;
    DBusPropertyNotifier m_propertyNotifier;
};

class MediaPlayer2Private {
public:
    MediaPlayer2Private(MediaPlayer2 *q);

private:
    friend class MediaPlayer2;
    QDBusConnection m_connection;
    PlayerAdaptor *m_playerAdaptor;
};

} // namespace mpris

PlayerAdaptor::PlayerAdaptor(const QDBusConnection &connection,
                             QObject *parent):
    QDBusAbstractAdaptor(parent),
    m_player(nullptr),
    m_propertyNotifier(connection, objectPath, this)
{
}

void PlayerAdaptor::setPlayer(media::PlayerImplementation *impl)
{
    using PlayerImplementation = media::PlayerImplementation;

    if (m_player == impl) return;

    if (m_player) {
        m_player->disconnect(this);
    }

    m_player = impl;
    if (impl) {
        QObject::connect(impl, &PlayerImplementation::seekedTo,
                         this, &PlayerAdaptor::Seeked);

        /* Property signals */
        QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                         this, [this]() {
            m_propertyNotifier.notify({
                "CanPlay", "CanPause", "CanSeek",
                "CanGoPrevious", "CanGoNext",
            });
        });
        QObject::connect(impl, &PlayerImplementation::playbackStatusChanged,
                         this, [this]() {
            m_propertyNotifier.notify({ "PlaybackStatus" });
        });
        QObject::connect(impl, &PlayerImplementation::metadataForCurrentTrackChanged,
                         this, [this]() {
            m_propertyNotifier.notify({ "Metadata" });
        });
    }

    m_propertyNotifier.notify();
}

QString PlayerAdaptor::playbackStatus() const
{
    using Player = media::Player;
    const Player::PlaybackStatus s = player() ?
        player()->playbackStatus() : Player::PlaybackStatus::stopped;
    switch (s) {
    case Player::PlaybackStatus::playing:
        return QStringLiteral("Playing");
    case Player::PlaybackStatus::paused:
        return QStringLiteral("Paused");
    default:
        return QStringLiteral("Stopped");
    }
}

void PlayerAdaptor::setLoopStatus(const QString &status)
{
    if (!player()) return;
    bool ok;
    int value = QMetaEnum::fromType<LoopStatus>().
        keyToValue(status.toUtf8().constData(), &ok);
    if (!ok) {
        MH_ERROR("Invalid loop status: %s", qUtf8Printable(status));
        return;
    }
    player()->setLoopStatus(static_cast<media::Player::LoopStatus>(value));
}

QString PlayerAdaptor::loopStatus() const
{
    int value = player() ? static_cast<LoopStatus>(player()->loopStatus()) : None;
    return QMetaEnum::fromType<LoopStatus>().valueToKey(value);
}

void PlayerAdaptor::setTypedLoopStatus(qint16 status)
{
    if (!player()) return;
    player()->setLoopStatus(static_cast<media::Player::LoopStatus>(status));
}

qint16 PlayerAdaptor::typedLoopStatus() const
{
    return player() ? static_cast<LoopStatus>(player()->loopStatus()) : None;
}

void PlayerAdaptor::PlayPause()
{
    if (!m_player) return;

    switch (m_player->playbackStatus()) {
    case core::ubuntu::media::Player::PlaybackStatus::ready:
    case core::ubuntu::media::Player::PlaybackStatus::paused:
    case core::ubuntu::media::Player::PlaybackStatus::stopped:
        m_player->play();
        break;
    case core::ubuntu::media::Player::PlaybackStatus::playing:
        m_player->pause();
        break;
    default:
        break;
    }
}

void PlayerAdaptor::OpenUri(const QDBusMessage &)
{
    // FIXME: unused, but we should implement it anyway
}

void PlayerAdaptor::Seek(quint64 microSeconds)
{
    if (!player()) return;
    player()->seek_to(std::chrono::microseconds(microSeconds));
}

MediaPlayer2Private::MediaPlayer2Private(MediaPlayer2 *q):
    m_connection(QDBusConnection::sessionBus()),
    m_playerAdaptor(new PlayerAdaptor(m_connection, q))
{
    new RootAdaptor(q);
    new PlaylistsAdaptor(q);
}

MediaPlayer2::MediaPlayer2(QObject *parent):
    QObject(parent),
    d_ptr(new MediaPlayer2Private(this))
{
}

MediaPlayer2::~MediaPlayer2() = default;

void MediaPlayer2::setPlayer(media::PlayerImplementation *impl)
{
    Q_D(MediaPlayer2);
    return d->m_playerAdaptor->setPlayer(impl);
}

media::PlayerImplementation *MediaPlayer2::player()
{
    Q_D(MediaPlayer2);
    return d->m_playerAdaptor->player();
}

const media::PlayerImplementation *MediaPlayer2::player() const
{
    Q_D(const MediaPlayer2);
    return d->m_playerAdaptor->player();
}

bool MediaPlayer2::registerObject()
{
    Q_D(MediaPlayer2);
    return d->m_connection.registerObject(objectPath, this);
}

#include "media_player2.moc"
