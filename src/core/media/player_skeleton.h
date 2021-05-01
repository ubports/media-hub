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
class PlayerSkeleton: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(bool CanPlay READ canPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool CanPause READ canPause NOTIFY canPauseChanged)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious
               NOTIFY canGoPreviousChanged)
    Q_PROPERTY(bool CanGoNext READ canGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool CanControl READ canControl NOTIFY canControlChanged)
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
    Q_PROPERTY(int64_t Position READ position)
    Q_PROPERTY(int64_t Duration READ duration)
    Q_PROPERTY(int16_t TypedBackend READ backend)
    Q_PROPERTY(int16_t Orientation READ orientation NOTIFY orientationChanged)
    Q_PROPERTY(int16_t Lifetime READ lifetime)
    Q_PROPERTY(int16_t AudioStreamRole READ audioStreamRole)
    Q_PROPERTY(int16_t TypedLoopStatus READ typedLoopStatus
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
        // Our functional dependencies.
        apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
        apparmor::ubuntu::RequestAuthenticator::Ptr request_authenticator;
    };

    PlayerSkeleton(const Configuration& configuration, QObject *parent = nullptr);
    ~PlayerSkeleton();

    void setPlayer(PlayerImplementation *impl);
    PlayerImplementation *player();
    const PlayerImplementation *player() const;

    bool registerAt(const QString &objectPath);

    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canGoPrevious() const;
    bool canGoNext() const;
    bool canControl() const;
    bool isVideoSource() const;
    bool isAudioSource() const;
    QString playbackStatus() const;
    void setLoopStatus(const QString &status);
    QString loopStatus() const;
    void setTypedLoopStatus(int16_t status);
    int16_t typedLoopStatus() const;
    void setPlaybackRate(double rate);
    double playbackRate() const;
    void setShuffle(bool shuffle);
    bool shuffle() const;
    QVariantMap metadata() const;
    void setVolume(double volume);
    double volume() const;
    double minimumRate() const;
    double maximumRate() const;
    int64_t position() const;
    int64_t duration() const;
    int16_t backend() const;
    int16_t orientation() const;
    int16_t lifetime() const;
    int16_t audioStreamRole() const;

public Q_SLOTS:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(uint64_t microSeconds);
    void SetPosition(const QDBusObjectPath &trackObject,
                     uint64_t microSeconds);
    void CreateVideoSink(uint32_t textureId);
    uint32_t Key() const;
    /* The OpenUri should not return anything, but since the previous
     * implementation was returning a boolean, let's keep doing that. */
    void OpenUri(const QDBusMessage &);
    void OpenUriExtended(const QDBusMessage &);

Q_SIGNALS:
    Q_SCRIPTABLE void Seeked(uint64_t microSeconds);
    Q_SCRIPTABLE void AboutToFinish();
    Q_SCRIPTABLE void EndOfStream();
    Q_SCRIPTABLE void PlaybackStatusChanged(int16_t status); // TODO: remove, we have PropertiesChanged already
    Q_SCRIPTABLE void VideoDimensionChanged(uint32_t height, uint32_t width);
    Q_SCRIPTABLE void Error(int16_t code);
    Q_SCRIPTABLE void Buffering(int percent); // TODO: set a fixed type

    void canControlChanged();
    void canPlayChanged();
    void canPauseChanged();
    void canGoPreviousChanged();
    void canGoNextChanged();
    void metadataChanged();
    void volumeChanged();
    void orientationChanged();

protected: // proxy QDBusContext
    inline const QDBusContext *context() const {
        return reinterpret_cast<QDBusContext *>(
            parent()->qt_metacast("QDBusContext"));
    }
    QDBusConnection connection() const { return context()->connection(); }
    const QDBusMessage &message() const { return context()->message(); }
    void sendErrorReply(const QString &name, const QString &msg) const {
        return context()->sendErrorReply(name, msg);
    }

private:
    Q_DECLARE_PRIVATE(PlayerSkeleton)
    QScopedPointer<PlayerSkeletonPrivate> d_ptr;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_PLAYER_SKELETON_H_
