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
 *              Alfonso Sanchez-Beato <alfonso.sanchez-beato@canonical.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include "bus.h"
#include "engine.h"
#include "meta_data_extractor.h"
#include "meta_data_support.h"
#include "playbin.h"

#include "core/media/logger/logger.h"

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
    media::Player::PlaybackStatus gst_state_to_player_status(const gstreamer::Bus::Message::Detail::StateChanged& state)
    {
        if (state.new_state == GST_STATE_PLAYING)
            return media::Player::PlaybackStatus::playing;
        else if (state.new_state == GST_STATE_PAUSED)
            return media::Player::PlaybackStatus::paused;
        else if (state.new_state == GST_STATE_READY)
            return media::Player::PlaybackStatus::ready;
        else if (state.new_state == GST_STATE_NULL)
            return media::Player::PlaybackStatus::null;
        else
            return media::Player::PlaybackStatus::stopped;
    }

    void on_playbin_state_changed(const std::pair<gstreamer::Bus::Message::Detail::StateChanged,std::string>& p)
    {
        if (p.second == "playbin")
        {
            MH_INFO("State changed on playbin: %s",
                      gst_element_state_get_name(p.first.new_state));
            const auto status = gst_state_to_player_status(p.first);
            /*
             * When state moves to "paused" the pipeline is already set. We check that we
             * have streams to play.
             */
            if (status == media::Player::PlaybackStatus::paused &&
                    !playbin.can_play_streams()) {
                MH_ERROR("** Cannot play: some codecs are missing");
                playbin.reset();
                const media::Player::Error e = media::Player::Error::format_error;
                error(e);
            } else {
                playback_status_changed(status);
            }
        }
    }

    // Converts from a GStreamer GError to a media::Player:Error enum
    media::Player::Error from_gst_errorwarning(const gstreamer::Bus::Message::Detail::ErrorWarningInfo& ewi)
    {
        media::Player::Error ret_error = media::Player::Error::no_error;

        if (g_strcmp0(g_quark_to_string(ewi.error->domain), "gst-core-error-quark") == 0)
        {
            switch (ewi.error->code)
            {
            case GST_CORE_ERROR_FAILED:
                MH_ERROR("** Encountered a GST_CORE_ERROR_FAILED");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_CORE_ERROR_NEGOTIATION:
                MH_ERROR("** Encountered a GST_CORE_ERROR_NEGOTIATION");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_CORE_ERROR_MISSING_PLUGIN:
                MH_ERROR("** Encountered a GST_CORE_ERROR_MISSING_PLUGIN");
                ret_error = media::Player::Error::format_error;
                break;
            default:
                MH_ERROR("** Encountered an unhandled core error: '%s' (code: %d)",
                    ewi.debug, ewi.error->code);
                ret_error = media::Player::Error::no_error;
                break;
            }
        }
        else if (g_strcmp0(g_quark_to_string(ewi.error->domain), "gst-resource-error-quark") == 0)
        {
            switch (ewi.error->code)
            {
            case GST_RESOURCE_ERROR_FAILED:
                MH_ERROR("** Encountered a GST_RESOURCE_ERROR_FAILED");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_RESOURCE_ERROR_NOT_FOUND:
                MH_ERROR("** Encountered a GST_RESOURCE_ERROR_NOT_FOUND");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_RESOURCE_ERROR_OPEN_READ:
                MH_ERROR("** Encountered a GST_RESOURCE_ERROR_OPEN_READ");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_RESOURCE_ERROR_OPEN_WRITE:
                MH_ERROR("** Encountered a GST_RESOURCE_ERROR_OPEN_WRITE");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_RESOURCE_ERROR_READ:
                MH_ERROR("** Encountered a GST_RESOURCE_ERROR_READ");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_RESOURCE_ERROR_WRITE:
                MH_ERROR("** Encountered a GST_RESOURCE_ERROR_WRITE");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_RESOURCE_ERROR_NOT_AUTHORIZED:
                MH_ERROR("** Encountered a GST_RESOURCE_ERROR_NOT_AUTHORIZED");
                ret_error = media::Player::Error::access_denied_error;
                break;
            default:
                MH_ERROR("** Encountered an unhandled resource error: '%s' (code: %d)",
                    ewi.debug, ewi.error->code);
                ret_error = media::Player::Error::no_error;
                break;
            }
        }
        else if (g_strcmp0(g_quark_to_string(ewi.error->domain), "gst-stream-error-quark") == 0)
        {
            switch (ewi.error->code)
            {
            case GST_STREAM_ERROR_FAILED:
                MH_ERROR("** Encountered a GST_STREAM_ERROR_FAILED");
                ret_error = media::Player::Error::resource_error;
                break;
            case GST_STREAM_ERROR_CODEC_NOT_FOUND:
                MH_ERROR("** Encountered a GST_STREAM_ERROR_CODEC_NOT_FOUND");
                // Missing codecs are handled later, when state switches to "paused"
                ret_error = media::Player::Error::no_error;
                break;
            case GST_STREAM_ERROR_DECODE:
                MH_ERROR("** Encountered a GST_STREAM_ERROR_DECODE");
                ret_error = media::Player::Error::format_error;
                break;
            default:
                MH_ERROR("** Encountered an unhandled stream error: '%s' code(%d)",
                    ewi.debug, ewi.error->code);
                ret_error = media::Player::Error::no_error;
                break;
            }
        }

        if (ret_error != media::Player::Error::no_error) {
            MH_ERROR("Resetting playbin pipeline after unrecoverable error");
            playbin.reset();
        }
        return ret_error;
    }

    void on_playbin_error(const gstreamer::Bus::Message::Detail::ErrorWarningInfo& ewi)
    {
        const media::Player::Error e = from_gst_errorwarning(ewi);
        if (e != media::Player::Error::no_error)
            error(e);
    }

    void on_playbin_warning(const gstreamer::Bus::Message::Detail::ErrorWarningInfo& ewi)
    {
        const media::Player::Error e = from_gst_errorwarning(ewi);
        if (e != media::Player::Error::no_error)
            error(e);
    }

    void on_playbin_info(const gstreamer::Bus::Message::Detail::ErrorWarningInfo& ewi)
    {
        MH_DEBUG("Got a playbin info message (no action taken): %s", ewi.debug);
    }

    void cleanupAlbumArtFile(const core::ubuntu::media::Track::MetaData& md) 
    {
        MetaDataSupport::cleanupTempAlbumArtFile(md);
    }

    void on_tag_available(const gstreamer::Bus::Message::Detail::Tag& tag)
    {
        media::Track::MetaData md;

        // We update instead of creating from scratch if same uri
        auto &tuple = track_meta_data.get();
        if (playbin.uri() == std::get<0>(tuple)) 
            md = std::get<1>(tuple);
        else 
            cleanupAlbumArtFile(std::get<1>(tuple));

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

    void on_orientation_changed(const media::Player::Orientation& o)
    {
        // Update the local orientation Property, which should then update the Player
        // orientation Property
        orientation.set(o);
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
        cleanupAlbumArtFile(std::get<1>(track_meta_data.get()));
    }

    void on_end_of_stream()
    {
        end_of_stream();
    }

    void on_video_dimension_changed(const media::video::Dimensions& dimensions)
    {
        video_dimension_changed(dimensions);
    }

    void on_buffering_changed(int value)
    {
        buffering_changed(value);
    }

    Private(const core::ubuntu::media::Player::PlayerKey key)
        : playbin(key),
          meta_data_extractor(new gstreamer::MetaDataExtractor()),
          volume(media::Engine::Volume(1.)),
          orientation(media::Player::Orientation::rotate0),
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
          on_error_connection(
              playbin.signals.on_error.connect(
                  std::bind(
                      &Private::on_playbin_error,
                      this,
                      std::placeholders::_1))),
          on_warning_connection(
              playbin.signals.on_warning.connect(
                  std::bind(
                      &Private::on_playbin_warning,
                      this,
                      std::placeholders::_1))),
          on_info_connection(
              playbin.signals.on_info.connect(
                  std::bind(
                      &Private::on_playbin_info,
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
          on_orientation_changed_connection(
              playbin.signals.on_orientation_changed.connect(
                  std::bind(
                      &Private::on_orientation_changed,
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
                      this))),
          on_video_dimension_changed_connection(
              playbin.signals.on_video_dimensions_changed.connect(
                  std::bind(
                      &Private::on_video_dimension_changed,
                      this,
                      std::placeholders::_1))),
          on_buffering_changed_connection(
              playbin.signals.on_buffering_changed.connect(
                  std::bind(
                      &Private::on_buffering_changed,
                      this,
                      std::placeholders::_1)))
    {
    }

    // Ensure the playbin is the last item destroyed
    // otherwise properties could try to access a dead playbin object
    gstreamer::Playbin playbin;

    std::shared_ptr<Engine::MetaDataExtractor> meta_data_extractor;
    core::Property<Engine::State> state;
    core::Property<std::tuple<media::Track::UriType, media::Track::MetaData>> track_meta_data;
    core::Property<uint64_t> position;
    core::Property<uint64_t> duration;
    core::Property<media::Engine::Volume> volume;
    core::Property<media::Player::AudioStreamRole> audio_role;
    core::Property<media::Player::Orientation> orientation;
    core::Property<media::Player::Lifetime> lifetime;
    core::Property<bool> is_video_source;
    core::Property<bool> is_audio_source;

    core::ScopedConnection about_to_finish_connection;
    core::ScopedConnection on_state_changed_connection;
    core::ScopedConnection on_error_connection;
    core::ScopedConnection on_warning_connection;
    core::ScopedConnection on_info_connection;
    core::ScopedConnection on_tag_available_connection;
    core::ScopedConnection on_volume_changed_connection;
    core::ScopedConnection on_audio_stream_role_changed_connection;
    core::ScopedConnection on_orientation_changed_connection;
    core::ScopedConnection on_lifetime_changed_connection;
    core::ScopedConnection on_seeked_to_connection;
    core::ScopedConnection client_disconnected_connection;
    core::ScopedConnection on_end_of_stream_connection;
    core::ScopedConnection on_video_dimension_changed_connection;
    core::ScopedConnection on_buffering_changed_connection;

    core::Signal<void> about_to_finish;
    core::Signal<uint64_t> seeked_to;
    core::Signal<void> client_disconnected;
    core::Signal<void> end_of_stream;
    core::Signal<media::Player::PlaybackStatus> playback_status_changed;
    core::Signal<core::ubuntu::media::video::Dimensions> video_dimension_changed;
    core::Signal<media::Player::Error> error;
    core::Signal<int> buffering_changed;
};

gstreamer::Engine::Engine(const core::ubuntu::media::Player::PlayerKey key)
    : d(new Private{key})
{
    d->state = media::Engine::State::no_media;
}

gstreamer::Engine::~Engine()
{
    stop();
    d->state = media::Engine::State::no_media;
}

const std::shared_ptr<media::Engine::MetaDataExtractor>&
        gstreamer::Engine::meta_data_extractor() const
{
    return d->meta_data_extractor;
}

const core::Property<media::Engine::State>& gstreamer::Engine::state() const
{
    return d->state;
}

bool gstreamer::Engine::open_resource_for_uri(const media::Track::UriType& uri,
                                              bool do_pipeline_reset)
{
    d->playbin.set_uri(uri, core::ubuntu::media::Player::HeadersType{}, do_pipeline_reset);
    return true;
}

bool gstreamer::Engine::open_resource_for_uri(const media::Track::UriType& uri,
                                              const core::ubuntu::media::Player::HeadersType& headers)
{
    d->playbin.set_uri(uri, headers);
    return true;
}

void gstreamer::Engine::create_video_sink(uint32_t texture_id)
{
    d->playbin.create_video_sink(texture_id);
}

bool gstreamer::Engine::play(bool use_main_thread /* = false */)
{
    const auto result = d->playbin.set_state_and_wait(GST_STATE_PLAYING, use_main_thread);

    if (result)
    {
        d->state = media::Engine::State::playing;
        MH_INFO("Engine: playing uri: %s", d->playbin.uri());
        d->playback_status_changed(media::Player::PlaybackStatus::playing);
    }

    return result;
}

bool gstreamer::Engine::stop(bool use_main_thread /* = false */)
{
    // No need to wait, and we can immediately return.
    if (d->state == media::Engine::State::stopped)
    {
        MH_DEBUG("Current player state is already stopped - no need to change state to stopped");
        return true;
    }

    const auto result = d->playbin.set_state_and_wait(GST_STATE_NULL, use_main_thread);
    if (result)
    {
        d->state = media::Engine::State::stopped;
        MH_TRACE("");
        d->playback_status_changed(media::Player::PlaybackStatus::stopped);
    }

    return result;
}

bool gstreamer::Engine::pause()
{
    const auto result = d->playbin.set_state_and_wait(GST_STATE_PAUSED);

    if (result)
    {
        d->state = media::Engine::State::paused;
        MH_TRACE("");
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

const core::Property<core::ubuntu::media::Player::Orientation>& gstreamer::Engine::orientation() const
{
    return d->orientation;
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

const core::Signal<core::ubuntu::media::video::Dimensions>& gstreamer::Engine::video_dimension_changed_signal() const
{
    return d->video_dimension_changed;
}

const core::Signal<core::ubuntu::media::Player::Error>& gstreamer::Engine::error_signal() const
{
    return d->error;
}

const core::Signal<int>& gstreamer::Engine::on_buffering_changed_signal() const
{
    return d->buffering_changed;
}

void gstreamer::Engine::reset()
{
    d->playbin.reset();
}
