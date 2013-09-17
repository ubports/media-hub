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

#ifndef COM_UBUNTU_MUSIC_PLAYER_STUB_H_
#define COM_UBUNTU_MUSIC_PLAYER_STUB_H_

#include <com/ubuntu/music/player.h>

#include <org/freedesktop/dbus/stub.h>
#include <org/freedesktop/dbus/traits/service.h>

#include <memory>

namespace org
{
namespace freedesktop
{
namespace dbus
{
namespace traits
{
template<>
struct Service<com::ubuntu::music::Player>
{
    static const std::string& interface_name()
    {
        static const std::string s
        {
            "com.ubuntu.music.Player"
        };
        return s;
    }
};
}
}
}
}

namespace com
{
namespace ubuntu
{
namespace music
{
class Service;

class PlayerStub : public org::freedesktop::dbus::Stub<Player>
{
  public:
    explicit PlayerStub(
        const std::shared_ptr<Service>& parent,
        const org::freedesktop::dbus::types::ObjectPath& object);

    ~PlayerStub();

    virtual std::shared_ptr<TrackList> track_list();

    virtual bool can_go_next();
    virtual void next();
    
    virtual bool can_go_previous();
    virtual void previous();
    
    virtual bool can_play();
    virtual void play();
    
    virtual bool can_pause();
    virtual void pause();
    
    virtual bool can_seek();
    virtual void seek_to(const std::chrono::microseconds& offset);

    virtual void stop();
    
    virtual PlaybackStatus playback_status() const;
    virtual Connection on_playback_status_changed(const std::function<void(PlaybackStatus)>& handler);
    
    virtual LoopStatus loop_status() const;
    virtual void set_loop_status(LoopStatus new_status);
    virtual Connection on_loop_status_changed(const std::function<void(LoopStatus)>& handler);

    virtual PlaybackRate playback_rate() const;
    virtual void set_playback_rate(PlaybackRate rate);
    virtual Connection on_playback_rate_changed(const std::function<void(PlaybackRate)>& handler);

    virtual bool is_shuffle() const;
    virtual void set_shuffle(bool b);
    virtual Connection on_shuffle_changed(const std::function<void(bool)>& handler);

    virtual Track::MetaData meta_data_for_current_track() const;
    virtual Connection on_meta_data_for_current_track_changed(const std::function<void(const Track::MetaData&)>& handler);

    virtual Volume volume() const;
    virtual void set_volume(Volume new_volume);
    virtual Connection on_volume_changed(const std::function<void(Volume)>& handler);

    virtual PlaybackRate minimum_playback_rate() const;
    virtual PlaybackRate maximum_playback_rate() const;
    
  private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_PLAYER_STUB_H_
