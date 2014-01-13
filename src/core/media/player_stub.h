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

#include <core/media/player.h>

#include <org/freedesktop/dbus/stub.h>

#include <memory>

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

    virtual bool open_uri(const Track::UriType& uri);
    virtual void next();
    virtual void previous();
    virtual void play();
    virtual void pause();
    virtual void seek_to(const std::chrono::microseconds& offset);
    virtual void stop();

    virtual const Property<bool>& can_play() const;
    virtual const Property<bool>& can_pause() const;
    virtual const Property<bool>& can_seek() const;
    virtual const Property<bool>& can_go_previous() const;
    virtual const Property<bool>& can_go_next() const;
    virtual const Property<PlaybackStatus>& playback_status() const;    
    virtual const Property<LoopStatus>& loop_status() const;
    virtual const Property<PlaybackRate>& playback_rate() const;
    virtual const Property<bool>& is_shuffle() const;
    virtual const Property<Track::MetaData>& meta_data_for_current_track() const;
    virtual const Property<Volume>& volume() const;
    virtual const Property<PlaybackRate>& minimum_playback_rate() const;
    virtual const Property<PlaybackRate>& maximum_playback_rate() const;
        
    virtual Property<LoopStatus>& loop_status();
    virtual Property<PlaybackRate>& playback_rate();
    virtual Property<bool>& is_shuffle();
    virtual Property<Volume>& volume();
    
    virtual const Signal<uint64_t>& seeked_to() const;
    
  private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_PLAYER_STUB_H_
