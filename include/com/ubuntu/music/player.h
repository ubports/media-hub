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

#include <com/ubuntu/music/connection.h>
#include <com/ubuntu/music/track.h>

#include <chrono>
#include <memory>

namespace com
{
namespace ubuntu
{
namespace music
{
class Service;
class TrackList;

class Player
{
  public:
    typedef double PlaybackRate;
    typedef double Volume;
    
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
    virtual ~Player() = default;

    Player& operator=(const Player&) = delete;
    bool operator==(const Player&) const = delete;

    virtual std::shared_ptr<TrackList> track_list() = 0;

    virtual bool can_go_next() = 0;
    virtual void next() = 0;
    
    virtual bool can_go_previous() = 0;
    virtual void previous() = 0;
    
    virtual bool can_play() = 0;
    virtual void play() = 0;
    
    virtual bool can_pause() = 0;
    virtual void pause() = 0;
    
    virtual bool can_seek() = 0;
    virtual void seek_to(const std::chrono::microseconds& offset) = 0;

    virtual void stop() = 0;
    
    virtual PlaybackStatus playback_status() const = 0;
    virtual Connection on_playback_status_changed(const std::function<void(PlaybackStatus)>& handler) = 0;
    
    virtual LoopStatus loop_status() const = 0;
    virtual void set_loop_status(LoopStatus new_status) = 0;
    virtual Connection on_loop_status_changed(const std::function<void(LoopStatus)>& handler) = 0;

    virtual PlaybackRate playback_rate() const = 0;
    virtual void set_playback_rate(PlaybackRate rate) = 0;
    virtual Connection on_playback_rate_changed(const std::function<void(PlaybackRate)>& handler) = 0;

    virtual bool is_shuffle() const = 0;
    virtual void set_shuffle(bool b) = 0;
    virtual Connection on_shuffle_changed(const std::function<void(bool)>& handler) = 0;

    virtual Track::MetaData meta_data_for_current_track() const = 0;
    virtual Connection on_meta_data_for_current_track_changed(const std::function<void(const Track::MetaData&)>& handler) = 0;

    virtual Volume volume() const = 0;
    virtual void set_volume(Volume new_volume) = 0;
    virtual Connection on_volume_changed(const std::function<void(Volume)>& handler) = 0;

    virtual PlaybackRate minimum_playback_rate() const = 0;
    virtual PlaybackRate maximum_playback_rate() const = 0;
    
  protected:
    Player() = default;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_PLAYER_H_
