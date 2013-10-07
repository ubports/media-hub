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
#ifndef COM_UBUNTU_MUSIC_PLAYER_H_
#define COM_UBUNTU_MUSIC_PLAYER_H_

#include <com/ubuntu/music/track.h>

#include <chrono>
#include <memory>

namespace com
{
namespace ubuntu
{
namespace music
{
template<typename T> class Property;
class Service;
template<typename T> class Signal;
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

    enum PlaybackStatus
    {
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

    virtual const Property<bool>& can_play() const = 0;
    virtual const Property<bool>& can_pause() const = 0;
    virtual const Property<bool>& can_seek() const = 0;
    virtual const Property<bool>& can_go_previous() const = 0;
    virtual const Property<bool>& can_go_next() const = 0;
    virtual const Property<PlaybackStatus>& playback_status() const = 0;    
    virtual const Property<LoopStatus>& loop_status() const = 0;
    virtual const Property<PlaybackRate>& playback_rate() const = 0;
    virtual const Property<bool>& is_shuffle() const = 0;
    virtual const Property<Track::MetaData>& meta_data_for_current_track() const = 0;
    virtual const Property<Volume>& volume() const = 0;
    virtual const Property<PlaybackRate>& minimum_playback_rate() const = 0;
    virtual const Property<PlaybackRate>& maximum_playback_rate() const = 0;
        
    virtual Property<LoopStatus>& loop_status() = 0;
    virtual Property<PlaybackRate>& playback_rate() = 0;
    virtual Property<bool>& is_shuffle() = 0;
    virtual Property<Volume>& volume() = 0;
    
    virtual const Signal<uint64_t>& seeked_to() const = 0;

  protected:
    Player();

};
}
}
}

#endif // COM_UBUNTU_MUSIC_PLAYER_H_
