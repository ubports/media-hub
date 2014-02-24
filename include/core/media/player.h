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
#ifndef CORE_UBUNTU_MEDIA_PLAYER_H_
#define CORE_UBUNTU_MEDIA_PLAYER_H_

#include <core/media/track.h>

#include <core/property.h>

#include <chrono>
#include <memory>

#define SIGNAL(Name, Itf, ArgType) \
    struct Name \
    { \
        inline static std::string name() \
        { \
            return #Name; \
        }; \
        typedef Itf Interface; \
        typedef ArgType ArgumentType; \
    };\

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

    struct Configuration;

    struct Client
    {
        Client() = delete;

        static const Configuration& default_configuration();
    };

    struct Signals
    {
        SIGNAL(EndOfStream, Player, int)
#if 0
        // Signals when the end of the current media stream has been reached
        // e.g. one song or video
        struct EndOfStream
        {
            inline static std::string name() { return "EndOfStream"; };
            typedef Service Interface;
            typedef int ArgumentType;
        };
#endif
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

    Player(const Player&) = delete;
    virtual ~Player();

    Player& operator=(const Player&) = delete;
    bool operator==(const Player&) const = delete;

    virtual std::shared_ptr<TrackList> track_list() = 0;

    virtual bool open_uri(const Track::UriType& uri) = 0;
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
    virtual const core::Property<PlaybackStatus>& playback_status() const = 0;
    virtual const core::Property<LoopStatus>& loop_status() const = 0;
    virtual const core::Property<PlaybackRate>& playback_rate() const = 0;
    virtual const core::Property<bool>& is_shuffle() const = 0;
    virtual const core::Property<Track::MetaData>& meta_data_for_current_track() const = 0;
    virtual const core::Property<Volume>& volume() const = 0;
    virtual const core::Property<PlaybackRate>& minimum_playback_rate() const = 0;
    virtual const core::Property<PlaybackRate>& maximum_playback_rate() const = 0;
    virtual const core::Property<uint64_t>& position() const = 0;
    virtual const core::Property<uint64_t>& duration() const = 0;


    virtual core::Property<LoopStatus>& loop_status() = 0;
    virtual core::Property<PlaybackRate>& playback_rate() = 0;
    virtual core::Property<bool>& is_shuffle() = 0;
    virtual core::Property<Volume>& volume() = 0;


    virtual const core::Signal<uint64_t>& seeked_to() const = 0;

  protected:
    Player();

};
}
}
}

#endif // CORE_UBUNTU_MEDIA_PLAYER_H_
