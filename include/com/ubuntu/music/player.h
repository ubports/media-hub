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

#include "com/ubuntu/music/connection.h"
#include "com/ubuntu/music/track.h"

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
    ~Player();

    Player& operator=(const Player&) = delete;
    bool operator==(const Player&) const = delete;

    const std::shared_ptr<TrackList>& track_list() const;

    bool can_go_next();
    void next();
    
    bool can_go_previous();
    void previous();
    
    bool can_play();
    void play();
    
    bool can_pause();
    void pause();
    
    bool can_seek();
    void seek_to(const std::chrono::microseconds& offset);

    void stop();
    
    PlaybackStatus playback_status() const;
    Connection on_playback_status_changed(const std::function<void(PlaybackStatus)>& handler);
    
    LoopStatus loop_status() const;
    void set_loop_status(LoopStatus new_status);
    Connection on_loop_status_changed(const std::function<void(LoopStatus)>& handler);

    PlaybackRate playback_rate() const;
    void set_playback_rate(PlaybackRate rate);
    Connection on_playback_rate_changed(const std::function<void(PlaybackRate)>& handler);

    bool is_shuffle() const;
    void set_shuffle(bool b);
    Connection on_shuffle_changed(const std::function<void(bool)>& handler);

    Track::MetaData meta_data_for_current_track() const;
    Connection on_meta_data_for_current_track_changed(const std::function<void(const Track::MetaData&)>& handler);

    Volume volume() const;
    void set_volume(Volume new_volume);
    Connection on_volume_changed(const std::function<void(Volume)>& handler);

    PlaybackRate minimum_playback_rate() const;
    PlaybackRate maximum_playback_rate() const;
    
  private:
    friend class Service;

    Player(const std::shared_ptr<Service>& parent);

    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_PLAYER_H_
