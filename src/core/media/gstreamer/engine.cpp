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

#include <stdio.h>
#include <stdlib.h>

#include "bus.h"
#include "engine.h"
#include "meta_data_extractor.h"
#include "playbin.h"

#include <cassert>

namespace media = core::ubuntu::media;

using namespace std;

namespace gstreamer
{
struct Init
{
    Init()
    {
        gst_init(nullptr, nullptr);
    }

    ~Init()
    {
        gst_deinit();
    }
} init;
}

struct gstreamer::Engine::Private
{
    void on_playbin_state_changed(
            const gstreamer::Bus::Message::Detail::StateChanged& state_change)
    {
        (void) state_change;
    }

    void on_tag_available(const gstreamer::Bus::Message::Detail::Tag& tag)
    {
        media::Track::MetaData md;
        gstreamer::MetaDataExtractor::on_tag_available(tag, md);
        track_meta_data.set(std::make_tuple(playbin.uri(), md));
    }

    void on_volume_changed(const media::Engine::Volume& new_volume)
    {
        playbin.set_volume(new_volume.value);
    }

    void on_audio_stream_role_changed(const media::Player::AudioStreamRole& new_audio_role)
    {
        playbin.set_audio_stream_role(new_audio_role);
    }

    void on_lifetime_changed(const media::Player::Lifetime& lifetime)
    {
        playbin.set_lifetime(lifetime);
    }

    void on_about_to_finish()
    {
        state = Engine::State::ready;
        about_to_finish();
    }

    void on_seeked_to(uint64_t value)
    {
        seeked_to(value);
    }

    void on_client_disconnected()
    {
        client_disconnected();
    }

    void on_end_of_stream()
    {
        end_of_stream();
    }

    Private()
        : meta_data_extractor(new gstreamer::MetaDataExtractor()),
          volume(media::Engine::Volume(1.)),
          is_video_source(false),
          is_audio_source(false),
          about_to_finish_connection(
              playbin.signals.about_to_finish.connect(
                  std::bind(
                      &Private::on_about_to_finish,
                      this))),
          on_state_changed_connection(
              playbin.signals.on_state_changed.connect(
                  std::bind(
                      &Private::on_playbin_state_changed,
                      this,
                      std::placeholders::_1))),
          on_tag_available_connection(
              playbin.signals.on_tag_available.connect(
                  std::bind(
                      &Private::on_tag_available,
                      this,
                      std::placeholders::_1))),
          on_volume_changed_connection(
              volume.changed().connect(
                  std::bind(
                      &Private::on_volume_changed,
                      this,
                      std::placeholders::_1))),
          on_audio_stream_role_changed_connection(
              audio_role.changed().connect(
                  std::bind(
                      &Private::on_audio_stream_role_changed,
                      this,
                      std::placeholders::_1))),
          on_lifetime_changed_connection(
              lifetime.changed().connect(
                  std::bind(
                      &Private::on_lifetime_changed,
                      this,
                      std::placeholders::_1))),
          on_seeked_to_connection(
              playbin.signals.on_seeked_to.connect(
                  std::bind(
                      &Private::on_seeked_to,
                      this,
                      std::placeholders::_1))),
          client_disconnected_connection(
              playbin.signals.client_disconnected.connect(
                  std::bind(
                      &Private::on_client_disconnected,
                      this))),
          on_end_of_stream_connection(
              playbin.signals.on_end_of_stream.connect(
                  std::bind(
                      &Private::on_end_of_stream,
                      this)))
    {
    }

    std::shared_ptr<Engine::MetaDataExtractor> meta_data_extractor;
    core::Property<Engine::State> state;
    core::Property<std::tuple<media::Track::UriType, media::Track::MetaData>> track_meta_data;
    core::Property<uint64_t> position;
    core::Property<uint64_t> duration;
    core::Property<media::Engine::Volume> volume;
    core::Property<media::Player::AudioStreamRole> audio_role;
    core::Property<media::Player::Lifetime> lifetime;
    core::Property<bool> is_video_source;
    core::Property<bool> is_audio_source;
    gstreamer::Playbin playbin;
    core::ScopedConnection about_to_finish_connection;
    core::ScopedConnection on_state_changed_connection;
    core::ScopedConnection on_tag_available_connection;
    core::ScopedConnection on_volume_changed_connection;
    core::ScopedConnection on_audio_stream_role_changed_connection;
    core::ScopedConnection on_lifetime_changed_connection;
    core::ScopedConnection on_seeked_to_connection;
    core::ScopedConnection client_disconnected_connection;
    core::ScopedConnection on_end_of_stream_connection;

    core::Signal<void> about_to_finish;
    core::Signal<uint64_t> seeked_to;
    core::Signal<void> client_disconnected;
    core::Signal<void> end_of_stream;
    core::Signal<media::Player::PlaybackStatus> playback_status_changed;
};

gstreamer::Engine::Engine() : d(new Private{})
{
    cout << "Creating a new Engine instance in " << __PRETTY_FUNCTION__ << endl;
    d->state = media::Engine::State::ready;
}

gstreamer::Engine::~Engine()
{
    stop();
}

const std::shared_ptr<media::Engine::MetaDataExtractor>& gstreamer::Engine::meta_data_extractor() const
{
    return d->meta_data_extractor;
}

const core::Property<media::Engine::State>& gstreamer::Engine::state() const
{
    return d->state;
}

bool gstreamer::Engine::open_resource_for_uri(const media::Track::UriType& uri)
{
    d->playbin.set_uri(uri);
    return true;
}

bool gstreamer::Engine::open_resource_for_uri(const media::Track::UriType& uri, const core::ubuntu::media::Player::HeadersType& headers)
{
    d->playbin.set_uri(uri, headers);
    return true;
}

void gstreamer::Engine::create_video_sink(uint32_t texture_id)
{
    d->playbin.create_video_sink(texture_id);
}

bool gstreamer::Engine::play()
{
    auto result = d->playbin.set_state_and_wait(GST_STATE_PLAYING);

    if (result)
    {
        d->state = media::Engine::State::playing;
        cout << "play" << endl;
        d->playback_status_changed(media::Player::PlaybackStatus::playing);
    }

    return result;
}

bool gstreamer::Engine::stop()
{
    // No need to wait, and we can immediately return.
    if (d->state == media::Engine::State::stopped)
        return true;

    auto result = d->playbin.set_state_and_wait(GST_STATE_NULL);

    if (result)
    {
        d->state = media::Engine::State::stopped;
        cout << "stop" << endl;
        d->playback_status_changed(media::Player::PlaybackStatus::stopped);
    }

    return result;
}

bool gstreamer::Engine::pause()
{
    auto result = d->playbin.set_state_and_wait(GST_STATE_PAUSED);

    if (result)
    {
        d->state = media::Engine::State::paused;
        cout << "pause" << endl;
        d->playback_status_changed(media::Player::PlaybackStatus::paused);
    }

    return result;
}

bool gstreamer::Engine::seek_to(const std::chrono::microseconds& ts)
{
    return d->playbin.seek(ts);
}

const core::Property<bool>& gstreamer::Engine::is_video_source() const
{
    gstreamer::Playbin::MediaFileType type = d->playbin.media_file_type();
    if (type == gstreamer::Playbin::MediaFileType::MEDIA_FILE_TYPE_VIDEO)
        d->is_video_source.set(true);
    else
        d->is_video_source.set(false);

    return d->is_video_source;
}

const core::Property<bool>& gstreamer::Engine::is_audio_source() const
{
    gstreamer::Playbin::MediaFileType type = d->playbin.media_file_type();
    if (type == gstreamer::Playbin::MediaFileType::MEDIA_FILE_TYPE_AUDIO)
        d->is_audio_source.set(true);
    else
        d->is_audio_source.set(false);

    return d->is_audio_source;
}

const core::Property<uint64_t>& gstreamer::Engine::position() const
{
    d->position.set(d->playbin.position());
    return d->position;
}

const core::Property<uint64_t>& gstreamer::Engine::duration() const
{
    d->duration.set(d->playbin.duration());
    return d->duration;
}

const core::Property<core::ubuntu::media::Engine::Volume>& gstreamer::Engine::volume() const
{
    return d->volume;
}

core::Property<core::ubuntu::media::Engine::Volume>& gstreamer::Engine::volume()
{
    return d->volume;
}

const core::Property<core::ubuntu::media::Player::AudioStreamRole>& gstreamer::Engine::audio_stream_role() const
{
    return d->audio_role;
}

const core::Property<core::ubuntu::media::Player::Lifetime>& gstreamer::Engine::lifetime() const
{
    return d->lifetime;
}

core::Property<core::ubuntu::media::Player::AudioStreamRole>& gstreamer::Engine::audio_stream_role()
{
    return d->audio_role;
}

core::Property<core::ubuntu::media::Player::Lifetime>& gstreamer::Engine::lifetime()
{
    return d->lifetime;
}

const core::Property<std::tuple<media::Track::UriType, media::Track::MetaData>>&
gstreamer::Engine::track_meta_data() const
{
    return d->track_meta_data;
}

const core::Signal<void>& gstreamer::Engine::about_to_finish_signal() const
{
    return d->about_to_finish;
}

const core::Signal<uint64_t>& gstreamer::Engine::seeked_to_signal() const
{
    return d->seeked_to;
}

const core::Signal<void>& gstreamer::Engine::client_disconnected_signal() const
{
    return d->client_disconnected;
}

const core::Signal<void>& gstreamer::Engine::end_of_stream_signal() const
{
    return d->end_of_stream;
}

const core::Signal<media::Player::PlaybackStatus>& gstreamer::Engine::playback_status_changed_signal() const
{
    return d->playback_status_changed;
}
