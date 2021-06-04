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
#include "playbin.h"

#include "core/media/logging.h"

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

class EnginePrivate
{
    Q_DECLARE_PUBLIC(Engine)

public:
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

    void on_playbin_state_changed(const gstreamer::Bus::Message::Detail::StateChanged &state,
                                  const QByteArray &source)
    {
        Q_Q(Engine);

        if (source == "playbin")
        {
            MH_INFO("State changed on playbin: %s",
                      gst_element_state_get_name(state.new_state));
            const auto status = gst_state_to_player_status(state);
            /*
             * When state moves to "paused" the pipeline is already set. We check that we
             * have streams to play.
             */
            if (status == media::Player::PlaybackStatus::paused &&
                    !playbin.can_play_streams()) {
                MH_ERROR("** Cannot play: some codecs are missing");
                playbin.reset();
                const media::Player::Error e = media::Player::Error::format_error;
                Q_EMIT q->errorOccurred(e);
            } else if (status == media::Player::PlaybackStatus::paused &&
                       q->state() == Engine::State::playing) {
                /* This is a spontaneus state change happening during the
                 * playbin initialization; we can ignore it. */
            } else {
                q->setPlaybackStatus(status);
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
            MH_ERROR("Resetting playbin pipeline after unrecoverable error: %s", ewi.debug);
            playbin.reset();
        }
        return ret_error;
    }

    void on_playbin_error(const gstreamer::Bus::Message::Detail::ErrorWarningInfo& ewi)
    {
        Q_Q(Engine);
        const media::Player::Error e = from_gst_errorwarning(ewi);
        if (e != media::Player::Error::no_error)
            Q_EMIT q->errorOccurred(e);
    }

    void on_playbin_warning(const gstreamer::Bus::Message::Detail::ErrorWarningInfo& ewi)
    {
        Q_Q(Engine);
        const media::Player::Error e = from_gst_errorwarning(ewi);
        if (e != media::Player::Error::no_error)
            Q_EMIT q->errorOccurred(e);
    }

    void on_playbin_info(const gstreamer::Bus::Message::Detail::ErrorWarningInfo& ewi)
    {
        MH_DEBUG("Got a playbin info message (no action taken): %s", ewi.debug);
    }

    void on_tag_available(const gstreamer::Bus::Message::Detail::Tag& tag)
    {
        Q_Q(Engine);
        media::Track::MetaData md;

        // We update instead of creating from scratch if same uri
        auto pair = q->trackMetadata();
        if (playbin.uri() == pair.first)
            md = pair.second;

        gstreamer::MetaDataExtractor::on_tag_available(tag, &md);
        q->setTrackMetadata(qMakePair(playbin.uri(), md));
    }

    EnginePrivate(const core::ubuntu::media::Player::PlayerKey key,
            Engine *q)
        : playbin(key),
          q_ptr(q)
    {
        QObject::connect(&playbin, &Playbin::errorOccurred,
                         q, [this](const Bus::Message::Detail::ErrorWarningInfo &ewi) {
            on_playbin_error(ewi);
        });
        QObject::connect(&playbin, &Playbin::warningOccurred,
                         q, [this](const Bus::Message::Detail::ErrorWarningInfo &ewi) {
            on_playbin_warning(ewi);
        });
        QObject::connect(&playbin, &Playbin::infoOccurred,
                         q, [this](const Bus::Message::Detail::ErrorWarningInfo &ewi) {
            on_playbin_info(ewi);
        });

        QObject::connect(&playbin, &Playbin::aboutToFinish,
                         q, [this, q]() {
            q->setState(Engine::State::ready);
            Q_EMIT q->aboutToFinish();
        });
        QObject::connect(&playbin, &Playbin::seekedTo,
                         q, &Engine::seekedTo);
        QObject::connect(&playbin, &Playbin::bufferingChanged,
                         q, &Engine::bufferingChanged);
        QObject::connect(&playbin, &Playbin::clientDisconnected,
                         q, &Engine::clientDisconnected);
        QObject::connect(&playbin, &Playbin::endOfStream,
                         q, &Engine::endOfStream);

        QObject::connect(&playbin, &Playbin::stateChanged,
                         q, [this](const Bus::Message::Detail::StateChanged &state,
                                   const QByteArray &source) {
            on_playbin_state_changed(state, source);
        });

        QObject::connect(&playbin, &Playbin::tagAvailable,
                         q, [this](const Bus::Message::Detail::Tag &tag) {
            on_tag_available(tag);
        });
        QObject::connect(&playbin, &Playbin::orientationChanged,
                         q, [q](media::Player::Orientation o) {
            q->setOrientation(o);
        });
        QObject::connect(&playbin, &Playbin::videoDimensionChanged,
                         q, [q](const QSize &size) {
            q->setVideoDimension(size);
        });
    }

    gstreamer::Playbin playbin;
    Engine *q_ptr;
};

} // namespace

gstreamer::Engine::Engine(const core::ubuntu::media::Player::PlayerKey key)
    : d_ptr(new EnginePrivate(key, this))
{
    Q_D(Engine);

    setMetadataExtractor(QSharedPointer<gstreamer::MetaDataExtractor>::create());

    doSetAudioStreamRole(audioStreamRole());
    doSetLifetime(lifetime());

    QObject::connect(&d->playbin, &Playbin::mediaFileTypeChanged,
                     this, [this, d]() {
        const auto fileType = d->playbin.mediaFileType();
        using ft = Playbin::MediaFileType;
        setIsVideoSource(fileType == ft::MEDIA_FILE_TYPE_VIDEO);
        setIsAudioSource(fileType == ft::MEDIA_FILE_TYPE_AUDIO);
    });
}

gstreamer::Engine::~Engine()
{
    stop();
    setState(media::Engine::State::no_media);
}

bool gstreamer::Engine::open_resource_for_uri(const QUrl &uri,
                                              bool do_pipeline_reset)
{
    Q_D(Engine);
    d->playbin.set_uri(uri, media::Player::HeadersType{}, do_pipeline_reset);
    return true;
}

bool gstreamer::Engine::open_resource_for_uri(const QUrl &uri,
                                              const core::ubuntu::media::Player::HeadersType& headers)
{
    Q_D(Engine);
    d->playbin.set_uri(uri, headers);
    return true;
}

void gstreamer::Engine::create_video_sink(uint32_t texture_id)
{
    Q_D(Engine);
    d->playbin.create_video_sink(texture_id);
}

bool gstreamer::Engine::play()
{
    Q_D(Engine);
    const auto result = d->playbin.set_state(GST_STATE_PLAYING);

    if (result)
    {
        setState(media::Engine::State::playing);
        MH_INFO("Engine: playing uri: %s", qUtf8Printable(d->playbin.uri().toString()));
    }

    return result;
}

bool gstreamer::Engine::stop()
{
    Q_D(Engine);
    // No need to wait, and we can immediately return.
    if (state() == media::Engine::State::stopped)
    {
        MH_DEBUG("Current player state is already stopped - no need to change state to stopped");
        return true;
    }

    const auto result = d->playbin.set_state(GST_STATE_NULL);
    if (result)
    {
        setState(media::Engine::State::stopped);
        MH_TRACE("");
        setPlaybackStatus(media::Player::stopped);
    }

    return result;
}

bool gstreamer::Engine::pause()
{
    Q_D(Engine);
    const auto result = d->playbin.set_state(GST_STATE_PAUSED);

    if (result)
    {
        setState(media::Engine::State::paused);
        MH_TRACE("");
    }

    return result;
}

bool gstreamer::Engine::seek_to(const std::chrono::microseconds& ts)
{
    Q_D(Engine);
    return d->playbin.seek(ts);
}

uint64_t gstreamer::Engine::position() const
{
    Q_D(const Engine);
    return d->playbin.position();
}

uint64_t gstreamer::Engine::duration() const
{
    Q_D(const Engine);
    return d->playbin.duration();
}

void gstreamer::Engine::reset()
{
    Q_D(Engine);
    d->playbin.reset();
}

void gstreamer::Engine::doSetAudioStreamRole(media::Player::AudioStreamRole role)
{
    Q_D(Engine);
    d->playbin.set_audio_stream_role(role);
}

void gstreamer::Engine::doSetLifetime(media::Player::Lifetime lifetime)
{
    Q_D(Engine);
    d->playbin.set_lifetime(lifetime);
}

void gstreamer::Engine::doSetVolume(double volume)
{
    Q_D(Engine);
    d->playbin.set_volume(volume);
}
