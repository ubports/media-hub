/*
 * Copyright © 2013-2014 Canonical Ltd.
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
#ifndef CORE_UBUNTU_MEDIA_ENGINE_H_
#define CORE_UBUNTU_MEDIA_ENGINE_H_

#include "player.h"
#include "track.h"

#include <QObject>
#include <QPair>
#include <QSharedPointer>
#include <QSize>
#include <QUrl>
#include <QVariantMap>

#include <chrono>

class QSize;

namespace core
{
namespace ubuntu
{
namespace media
{
class EnginePrivate;
class Engine: public QObject
{
    Q_OBJECT

public:

    enum class State
    {
        no_media,
        ready,
        busy,
        playing,
        paused,
        stopped
    };
    Q_ENUM(State)

    class MetaDataExtractor
    {
    public:
        typedef std::function<void(const QVariantMap &)> Callback;
        virtual void meta_data_for_track_with_uri(const QUrl &uri,
                                                  const Callback &cb) = 0;

    protected:
        MetaDataExtractor() = default;
        MetaDataExtractor(const MetaDataExtractor&) = delete;
        virtual ~MetaDataExtractor() = default;

        MetaDataExtractor& operator=(const MetaDataExtractor&) = delete;
    };

    Engine(QObject *parent = nullptr);
    virtual ~Engine();

    const QSharedPointer<MetaDataExtractor> &metadataExtractor() const;

    virtual bool open_resource_for_uri(const QUrl &uri, bool do_pipeline_reset) = 0;
    virtual bool open_resource_for_uri(const QUrl &uri, const Player::HeadersType&) = 0;
    // Throws core::ubuntu::media::Player::Error::OutOfProcessBufferStreamingNotSupported if the implementation does not
    // support this feature.
    virtual void create_video_sink(uint32_t texture_id) = 0;

    virtual bool play() = 0;
    virtual bool stop() = 0;
    virtual bool pause() = 0;
    virtual bool seek_to(const std::chrono::microseconds& ts) = 0;

    State state() const;

    bool isVideoSource() const;
    bool isAudioSource() const;

    virtual uint64_t position() const = 0;
    virtual uint64_t duration() const = 0;

    void setAudioStreamRole(Player::AudioStreamRole role);
    Player::AudioStreamRole audioStreamRole() const;

    void setLifetime(Player::Lifetime lifetime);
    Player::Lifetime lifetime() const;

    Player::Orientation orientation() const;
    QPair<QUrl,Track::MetaData> trackMetadata() const;
    Player::PlaybackStatus playbackStatus() const;
    QSize videoDimension() const;

    void setVolume(double volume);
    double volume() const;

    virtual void reset() = 0;

Q_SIGNALS:
    void stateChanged();

    void isVideoSourceChanged();
    void isAudioSourceChanged();

    // TODO These two are actually not emitted, decide whether to remove them
    void positionChanged();
    void durationChanged();

    void orientationChanged();

    void trackMetadataChanged();

    void aboutToFinish();
    void seekedTo(uint64_t offset);
    void clientDisconnected();
    void endOfStream();
    void playbackStatusChanged();
    void videoDimensionChanged();
    void errorOccurred(Player::Error error);
    void bufferingChanged(int);

protected:
    void setMetadataExtractor(const QSharedPointer<MetaDataExtractor> &extractor);
    void setState(State state);
    void setIsVideoSource(bool value);
    void setIsAudioSource(bool value);
    void setOrientation(Player::Orientation o);
    void setTrackMetadata(const QPair<QUrl,Track::MetaData> &metadata);
    void setVideoDimension(const QSize &size);
    void setPlaybackStatus(Player::PlaybackStatus status);

    virtual void doSetAudioStreamRole(Player::AudioStreamRole role) = 0;
    virtual void doSetLifetime(Player::Lifetime lifetime) = 0;
    virtual void doSetVolume(double volume) = 0;

private:
    Q_DECLARE_PRIVATE(Engine)
    QScopedPointer<EnginePrivate> d_ptr;
};

}
}
}

#endif // CORE_UBUNTU_MEDIA_ENGINE_H_
