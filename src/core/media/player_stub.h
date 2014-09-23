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

class PlayerStub : public Player
{
  public:
    explicit PlayerStub(
        const std::shared_ptr<Service>& parent,
        const std::shared_ptr<core::dbus::Object>& object);

    ~PlayerStub();

    virtual std::shared_ptr<TrackList> track_list();
    virtual PlayerKey key() const;

    virtual bool open_uri(const Track::UriType& uri);
    virtual bool open_uri(const Track::UriType& uri, const Player::HeadersType& headers);
    virtual void create_video_sink(uint32_t texture_id);
    virtual GLConsumerWrapperHybris gl_consumer() const;
    virtual void next();
    virtual void previous();
    virtual void play();
    virtual void pause();
    virtual void seek_to(const std::chrono::microseconds& offset);
    virtual void stop();

    virtual void set_frame_available_callback(FrameAvailableCb cb, void *context);
    virtual void set_playback_complete_callback(PlaybackCompleteCb cb, void *context);

    virtual const core::Property<bool>& can_play() const;
    virtual const core::Property<bool>& can_pause() const;
    virtual const core::Property<bool>& can_seek() const;
    virtual const core::Property<bool>& can_go_previous() const;
    virtual const core::Property<bool>& can_go_next() const;
    virtual const core::Property<bool>& is_video_source() const;
    virtual const core::Property<bool>& is_audio_source() const;
    virtual const core::Property<PlaybackStatus>& playback_status() const;
    virtual const core::Property<LoopStatus>& loop_status() const;
    virtual const core::Property<PlaybackRate>& playback_rate() const;
    virtual const core::Property<bool>& is_shuffle() const;
    virtual const core::Property<Track::MetaData>& meta_data_for_current_track() const;
    virtual const core::Property<Volume>& volume() const;
    virtual const core::Property<PlaybackRate>& minimum_playback_rate() const;
    virtual const core::Property<PlaybackRate>& maximum_playback_rate() const;
    virtual const core::Property<int64_t>& position() const;
    virtual const core::Property<int64_t>& duration() const;
    virtual const core::Property<AudioStreamRole>& audio_stream_role() const;

    virtual core::Property<LoopStatus>& loop_status();
    virtual core::Property<PlaybackRate>& playback_rate();
    virtual core::Property<bool>& is_shuffle();
    virtual core::Property<Volume>& volume();
    virtual core::Property<AudioStreamRole>& audio_stream_role();

    virtual const core::Signal<int64_t>& seeked_to() const;
    virtual const core::Signal<void>& end_of_stream() const;
    virtual core::Signal<PlaybackStatus>& playback_status_changed();

  private:
    struct Private;
    std::unique_ptr<Private> d;
    std::thread worker;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_PLAYER_STUB_H_
