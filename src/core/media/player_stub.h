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

#ifndef CORE_UBUNTU_MEDIA_PLAYER_STUB_H_
#define CORE_UBUNTU_MEDIA_PLAYER_STUB_H_

#include <core/media/player.h>

#include <core/dbus/stub.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class Service;

class PlayerStub : public core::dbus::Stub<Player>
{
  public:
    explicit PlayerStub(
        const std::shared_ptr<Service>& parent,
        const core::dbus::types::ObjectPath& object);

    ~PlayerStub();

    virtual std::shared_ptr<TrackList> track_list();

    virtual bool open_uri(const Track::UriType& uri);
    virtual void next();
    virtual void previous();
    virtual void play();
    virtual void pause();
    virtual void seek_to(const std::chrono::microseconds& offset);
    virtual void stop();

    virtual const core::Property<bool>& can_play() const;
    virtual const core::Property<bool>& can_pause() const;
    virtual const core::Property<bool>& can_seek() const;
    virtual const core::Property<bool>& can_go_previous() const;
    virtual const core::Property<bool>& can_go_next() const;
    virtual const core::Property<PlaybackStatus>& playback_status() const;
    virtual const core::Property<LoopStatus>& loop_status() const;
    virtual const core::Property<PlaybackRate>& playback_rate() const;
    virtual const core::Property<bool>& is_shuffle() const;
    virtual const core::Property<Track::MetaData>& meta_data_for_current_track() const;
    virtual const core::Property<Volume>& volume() const;
    virtual const core::Property<PlaybackRate>& minimum_playback_rate() const;
    virtual const core::Property<PlaybackRate>& maximum_playback_rate() const;
    virtual const core::Property<uint64_t>& position() const;

    virtual core::Property<LoopStatus>& loop_status();
    virtual core::Property<PlaybackRate>& playback_rate();
    virtual core::Property<bool>& is_shuffle();
    virtual core::Property<Volume>& volume();

    virtual const core::Signal<uint64_t>& seeked_to() const;

  private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_PLAYER_STUB_H_
