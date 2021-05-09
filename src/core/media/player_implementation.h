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

#ifndef CORE_UBUNTU_MEDIA_PLAYER_IMPLEMENTATION_H_
#define CORE_UBUNTU_MEDIA_PLAYER_IMPLEMENTATION_H_

#include "apparmor/ubuntu.h"
#include "client_death_observer.h"
#include "player.h"
#include "power/state_controller.h"
#include "track.h"

#include <QMap>
#include <QObject>
#include <QScopedPointer>
#include <QString>

namespace core
{
namespace ubuntu
{
namespace media
{
class Engine;
class Service;

class PlayerImplementationPrivate;
class PlayerImplementation : public QObject
{
    Q_OBJECT

public:
    using Headers = Player::HeadersType;

    // All creation time arguments go here
    struct Configuration
    {
        // The unique client identifying the player instance.
        Player::Client client;
        // Functional dependencies
        ClientDeathObserver::Ptr client_death_observer;
    };

    PlayerImplementation(const Configuration& configuration,
                         QObject *parent = nullptr);
    ~PlayerImplementation();

    AVBackend::Backend backend() const;

    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canGoPrevious() const;
    bool canGoNext() const;

    void setPlaybackRate(double rate);
    double playbackRate() const;
    double minimumRate() const;
    double maximumRate() const;

    void setLoopStatus(Player::LoopStatus status);
    Player::LoopStatus loopStatus() const;

    void setShuffle(bool shuffle);
    bool shuffle() const;

    void setVolume(double volume);
    double volume() const;

    Player::PlaybackStatus playbackStatus() const;

    bool isVideoSource() const;
    bool isAudioSource() const;
    QSize videoDimension() const;
    Player::Orientation orientation() const;
    Track::MetaData metadataForCurrentTrack() const;

    uint64_t position() const;
    uint64_t duration() const;

    void setAudioStreamRole(Player::AudioStreamRole role);
    Player::AudioStreamRole audioStreamRole() const;

    void setLifetime(Player::Lifetime lifetime);
    Player::Lifetime lifetime() const;

    void reconnect();
    void abandon();

    QSharedPointer<TrackListImplementation> trackList();
    Player::PlayerKey key() const;

    void create_gl_texture_video_sink(std::uint32_t texture_id);

    bool open_uri(const QUrl &uri);
    bool open_uri(const QUrl &uri, const Headers &headers);
    void next();
    void previous();
    void play();
    void pause();
    void stop();
    void seek_to(const std::chrono::microseconds& offset);

Q_SIGNALS:
    void isVideoSourceChanged();
    void isAudioSourceChanged();
    void metadataForCurrentTrackChanged();
    void mprisPropertiesChanged();

    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void playbackStatusChanged();

    void orientationChanged();
    void videoDimensionChanged();

    void aboutToFinish();
    void clientDisconnected();
    void seekedTo(uint64_t offset);
    void bufferingChanged(int);
    void endOfStream();
    void errorOccurred(Player::Error error);
    /* TODO: the Service should pause all other players and set this one as the current one */
    void playbackRequested();

private:
    Q_DECLARE_PRIVATE(PlayerImplementation)
    QScopedPointer<PlayerImplementationPrivate> d_ptr;
};

}
}
}
#endif // CORE_UBUNTU_MEDIA_PLAYER_IMPLEMENTATION_H_
