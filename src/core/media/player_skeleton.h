/**
 * Copyright (C) 2013-2015 Canonical Ltd
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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */

#ifndef CORE_UBUNTU_MEDIA_PLAYER_SKELETON_H_
#define CORE_UBUNTU_MEDIA_PLAYER_SKELETON_H_

#include "player.h"

#include "apparmor/ubuntu.h"

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusContext>
#include <QMap>
#include <QString>

class QDBusObjectPath;
class QUrl;

namespace core
{
namespace ubuntu
{
namespace media
{

class PlayerImplementation;
class Service;

class PlayerSkeletonPrivate;
class PlayerSkeleton: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(bool CanPlay READ canPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool CanPause READ canPause NOTIFY canPauseChanged)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious
               NOTIFY canGoPreviousChanged)
    Q_PROPERTY(bool CanGoNext READ canGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool IsVideoSource READ isVideoSource)
    Q_PROPERTY(bool IsAudioSource READ isAudioSource)
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus
               NOTIFY PlaybackStatusChanged)
    Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double PlaybackRate READ playbackRate WRITE setPlaybackRate)
    Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata NOTIFY metadataChanged)
    Q_PROPERTY(double Volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(qint64 Position READ position)
    Q_PROPERTY(qint64 Duration READ duration)
    Q_PROPERTY(qint16 TypedBackend READ backend)
    Q_PROPERTY(qint16 Orientation READ orientation NOTIFY orientationChanged)
    Q_PROPERTY(qint16 Lifetime READ lifetime)
    Q_PROPERTY(qint16 AudioStreamRole READ audioStreamRole
               WRITE setAudioStreamRole)
    Q_PROPERTY(qint16 TypedLoopStatus READ typedLoopStatus
               WRITE setTypedLoopStatus)

public:
    typedef QMap<QString,QString> Headers;

    enum LoopStatus {
        None = Player::LoopStatus::none,
        Track = Player::LoopStatus::track,
        Playlist = Player::LoopStatus::playlist,
    };
    Q_ENUM(LoopStatus)

    // All creation time arguments go here.
    struct Configuration
    {
        QDBusConnection connection;
        PlayerImplementation *player;
        // Our functional dependencies.
        apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
        apparmor::ubuntu::RequestAuthenticator::Ptr request_authenticator;
    };

    PlayerSkeleton(const Configuration& configuration, QObject *parent = nullptr);
    ~PlayerSkeleton();

    PlayerImplementation *player();
    const PlayerImplementation *player() const;

    bool registerAt(const QString &objectPath);

    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canGoPrevious() const;
    bool canGoNext() const;
    bool isVideoSource() const;
    bool isAudioSource() const;
    QString playbackStatus() const;
    void setLoopStatus(const QString &status);
    QString loopStatus() const;
    void setTypedLoopStatus(qint16 status);
    qint16 typedLoopStatus() const;
    void setPlaybackRate(double rate);
    double playbackRate() const;
    void setShuffle(bool shuffle);
    bool shuffle() const;
    QVariantMap metadata() const;
    void setVolume(double volume);
    double volume() const;
    double minimumRate() const;
    double maximumRate() const;
    qint64 position() const;
    qint64 duration() const;
    qint16 backend() const;
    qint16 orientation() const;
    qint16 lifetime() const;
    void setAudioStreamRole(qint16 role);
    qint16 audioStreamRole() const;

public Q_SLOTS:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(quint64 microSeconds);
    void SetPosition(const QDBusObjectPath &trackObject,
                     quint64 microSeconds);
    void CreateVideoSink(quint32 textureId);
    quint32 Key() const;
    /* The OpenUri should not return anything, but since the previous
     * implementation was returning a boolean, let's keep doing that. */
    void OpenUri(const QDBusMessage &);
    void OpenUriExtended(const QDBusMessage &);

Q_SIGNALS:
    Q_SCRIPTABLE void Seeked(quint64 microSeconds);
    Q_SCRIPTABLE void AboutToFinish();
    Q_SCRIPTABLE void EndOfStream();
    Q_SCRIPTABLE void PlaybackStatusChanged(qint16 status); // TODO: remove, we have PropertiesChanged already
    Q_SCRIPTABLE void VideoDimensionChanged(quint32 height, quint32 width);
    Q_SCRIPTABLE void Error(qint16 code);
    Q_SCRIPTABLE void Buffering(int percent); // TODO: set a fixed type

    void canPlayChanged();
    void canPauseChanged();
    void canGoPreviousChanged();
    void canGoNextChanged();
    void metadataChanged();
    void volumeChanged();
    void orientationChanged();

private:
    Q_DECLARE_PRIVATE(PlayerSkeleton)
    QScopedPointer<PlayerSkeletonPrivate> d_ptr;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_PLAYER_SKELETON_H_
