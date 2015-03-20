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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */
#ifndef CORE_UBUNTU_MEDIA_PLAYER_H_
#define CORE_UBUNTU_MEDIA_PLAYER_H_

#include <core/media/track.h>
#include <core/media/video/sink.h>

#include <core/property.h>

#include <chrono>
#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class Service;
class TrackList;

class Player : public std::enable_shared_from_this<Player>
{
  public:
    typedef double PlaybackRate;
    typedef double Volume;
    typedef uint32_t PlayerKey;
    typedef void* GLConsumerWrapperHybris;
    typedef std::map<std::string, std::string> HeadersType;

    struct Errors
    {
        Errors() = delete;

        struct OutOfProcessBufferStreamingNotSupported : public std::runtime_error
        {
            OutOfProcessBufferStreamingNotSupported();
        };
    };

    struct Configuration;

    struct Client
    {
        Client() = delete;

        static const Configuration& default_configuration();
    };

    enum PlaybackStatus
    {
        null,
        ready,
        playing,
        paused,
        stopped
    };

    enum LoopStatus
    {
        none,
        track,
        playlist
    };

    /**
     * Audio stream role types used to categorize audio playback.
     * multimedia is the default role type and will be automatically
     * paused by media-hub when other types need to play.
     */
    enum AudioStreamRole
    {
        alarm,
        alert,
        multimedia,
        phone
    };

    enum Orientation
    {
        rotate0,
        rotate90,
        rotate180,
        rotate270
    };

    enum Lifetime
    {
        normal,
        resumable
    };

    enum Error
    {
        no_error,
        resource_error,
        format_error,
        network_error,
        access_denied_error,
        service_missing_error
    };

    Player(const Player&) = delete;
    virtual ~Player();

    Player& operator=(const Player&) = delete;
    bool operator==(const Player&) const = delete;

    virtual std::shared_ptr<TrackList> track_list() = 0;
    virtual PlayerKey key() const = 0;

    virtual video::Sink::Ptr create_gl_texture_video_sink(std::uint32_t texture_id) = 0;

    virtual bool open_uri(const Track::UriType& uri) = 0;
    virtual bool open_uri(const Track::UriType& uri, const HeadersType&) = 0;
    virtual void next() = 0;
    virtual void previous() = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek_to(const std::chrono::microseconds& offset) = 0;

    virtual const core::Property<bool>& can_play() const = 0;
    virtual const core::Property<bool>& can_pause() const = 0;
    virtual const core::Property<bool>& can_seek() const = 0;
    virtual const core::Property<bool>& can_go_previous() const = 0;
    virtual const core::Property<bool>& can_go_next() const = 0;
    virtual const core::Property<bool>& is_video_source() const = 0;
    virtual const core::Property<bool>& is_audio_source() const = 0;
    virtual const core::Property<PlaybackStatus>& playback_status() const = 0;
    virtual const core::Property<LoopStatus>& loop_status() const = 0;
    virtual const core::Property<PlaybackRate>& playback_rate() const = 0;
    virtual const core::Property<bool>& is_shuffle() const = 0;
    virtual const core::Property<Track::MetaData>& meta_data_for_current_track() const = 0;
    virtual const core::Property<Volume>& volume() const = 0;
    virtual const core::Property<PlaybackRate>& minimum_playback_rate() const = 0;
    virtual const core::Property<PlaybackRate>& maximum_playback_rate() const = 0;
    virtual const core::Property<int64_t>& position() const = 0;
    virtual const core::Property<int64_t>& duration() const = 0;
    virtual const core::Property<AudioStreamRole>& audio_stream_role() const = 0;
    virtual const core::Property<Orientation>& orientation() const = 0;
    virtual const core::Property<Lifetime>& lifetime() const = 0;

    virtual core::Property<LoopStatus>& loop_status() = 0;
    virtual core::Property<PlaybackRate>& playback_rate() = 0;
    virtual core::Property<bool>& is_shuffle() = 0;
    virtual core::Property<Volume>& volume() = 0;
    virtual core::Property<AudioStreamRole>& audio_stream_role() = 0;
    virtual core::Property<Lifetime>& lifetime() = 0;

    virtual const core::Signal<int64_t>& seeked_to() const = 0;
    virtual const core::Signal<void>& end_of_stream() const = 0;
    virtual core::Signal<PlaybackStatus>& playback_status_changed() = 0;
    /**
     * Called when the video height/width change. Passes height and width as a bitmask with
     * height in the upper 32 bits and width in the lower 32 bits (both unsigned values)
     */
    virtual const core::Signal<uint64_t>& video_dimension_changed() const = 0;
    /** Signals all errors and warnings (typically from GStreamer and below) */
    virtual const core::Signal<Error>& error() const = 0;

  protected:
    Player();

};
}
}
}

#endif // CORE_UBUNTU_MEDIA_PLAYER_H_
