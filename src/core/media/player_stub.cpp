/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include <core/media/service.h>
#include <core/media/track_list.h>

#include "codec.h"
#include "player_stub.h"
#include "player_traits.h"
#include "property_stub.h"
#include "the_session_bus.h"
#include "track_list_stub.h"

#include "mpris/player.h"

#include <core/dbus/property.h>
#include <core/dbus/types/object_path.h>

// Hybris
#include <hybris/media/media_codec_layer.h>
#include <hybris/media/surface_texture_client_hybris.h>

#include <limits>

#define UNUSED __attribute__((unused))

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

struct media::PlayerStub::Private
{
    Private(const std::shared_ptr<Service>& parent,
            const std::shared_ptr<dbus::Service>& remote,
            const dbus::types::ObjectPath& path
            ) : parent(parent),
                texture_id(0),
                igbc_wrapper(nullptr),
                glc_wrapper(nullptr),
                decoding_session(decoding_service_create_session()),
                frame_available_cb(nullptr),
                frame_available_context(nullptr),
                path(path),
                object(remote->object_for_path(path)),
                properties
                {
                    object->get_property<mpris::Player::Properties::CanPlay>(),
                    object->get_property<mpris::Player::Properties::CanPause>(),
                    object->get_property<mpris::Player::Properties::CanSeek>(),
                    object->get_property<mpris::Player::Properties::CanControl>(),
                    object->get_property<mpris::Player::Properties::CanGoNext>(),
                    object->get_property<mpris::Player::Properties::CanGoPrevious>(),
                    object->get_property<mpris::Player::Properties::IsVideoSource>(),
                    object->get_property<mpris::Player::Properties::IsAudioSource>(),
                    object->get_property<mpris::Player::Properties::PlaybackStatus>(),
                    object->get_property<mpris::Player::Properties::LoopStatus>(),
                    object->get_property<mpris::Player::Properties::PlaybackRate>(),
                    object->get_property<mpris::Player::Properties::Shuffle>(),
                    object->get_property<mpris::Player::Properties::MetaData>(),
                    object->get_property<mpris::Player::Properties::Volume>(),
                    object->get_property<mpris::Player::Properties::Position>(),
                    object->get_property<mpris::Player::Properties::Duration>(),
                    object->get_property<mpris::Player::Properties::MinimumRate>(),
                    object->get_property<mpris::Player::Properties::MaximumRate>()
                },
                signals
                {
                    object->get_signal<mpris::Player::Signals::Seeked>(),
                    object->get_signal<mpris::Player::Signals::EndOfStream>()
                }
    {
    }

    ~Private()
    {
    }

    static void on_frame_available_cb(UNUSED GLConsumerWrapperHybris wrapper, void *context)
    {
        if (context != nullptr) {
            Private *p = static_cast<Private*>(context);
            p->on_frame_available();
        }
        else
            std::cout << "context is nullptr, can't call on_frame_available()" << std::endl;
    }

    void on_frame_available()
    {
        if (frame_available_cb != nullptr) {
            frame_available_cb(frame_available_context);
        }
        else
            std::cout << "frame_available_cb is nullptr, can't call frame_available_cb()" << std::endl;
    }

    void set_frame_available_cb(FrameAvailableCb cb, void *context)
    {
        frame_available_cb = cb;
        frame_available_context = context;

        gl_consumer_set_frame_available_cb(glc_wrapper, &Private::on_frame_available_cb, static_cast<void*>(this));
    }

    /** We need a GLConsumerHybris instance for doing texture streaming over the
     * process boundary **/
    void get_gl_consumer()
    {
        igbc_wrapper = decoding_service_get_igraphicbufferconsumer();
        glc_wrapper = gl_consumer_create_by_id_with_igbc(texture_id, igbc_wrapper);

    }

    std::shared_ptr<Service> parent;
    std::shared_ptr<TrackList> track_list;

    uint32_t texture_id;
    IGBCWrapperHybris igbc_wrapper;
    GLConsumerWrapperHybris glc_wrapper;

    DSSessionWrapperHybris decoding_session;

    FrameAvailableCb frame_available_cb;
    void *frame_available_context;

    dbus::Bus::Ptr bus;
    dbus::types::ObjectPath path;
    dbus::Object::Ptr object;

    struct
    {
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanPlay>> can_play;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanPause>> can_pause;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanSeek>> can_seek;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanControl>> can_control;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanGoNext>> can_go_next;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanGoPrevious>> can_go_previous;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::IsVideoSource>> is_video_source;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::IsAudioSource>> is_audio_source;

        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::PlaybackStatus>> playback_status;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::LoopStatus>> loop_status;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::PlaybackRate>> playback_rate;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Shuffle>> is_shuffle;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::MetaData>> meta_data_for_current_track;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Volume>> volume;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Position>> position;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Duration>> duration;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::MinimumRate>> minimum_playback_rate;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::MaximumRate>> maximum_playback_rate;
    } properties;

    struct Signals
    {
        typedef core::dbus::Signal<mpris::Player::Signals::Seeked, mpris::Player::Signals::Seeked::ArgumentType> DBusSeekedToSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::EndOfStream, mpris::Player::Signals::EndOfStream::ArgumentType> DBusEndOfStreamSignal;

        Signals(const std::shared_ptr<DBusSeekedToSignal>& seeked,
                const std::shared_ptr<DBusEndOfStreamSignal>& eos)
            : dbus
              {
                  seeked,
                  eos
              },
              playback_complete_cb(nullptr),
              playback_complete_context(nullptr),
              seeked_to(),
              end_of_stream()
        {
            dbus.seeked_to->connect([this](std::uint64_t value)
            {
                std::cout << "seeked_to signal arrived via the bus." << std::endl;
                seeked_to(value);
            });

            dbus.end_of_stream->connect([this]()
            {
                std::cout << "EndOfStream signal arrived via the bus." << std::endl;
                if (playback_complete_cb)
                    playback_complete_cb(playback_complete_context);
                end_of_stream();
            });
        }

        struct DBus
        {
            std::shared_ptr<DBusSeekedToSignal> seeked_to;
            std::shared_ptr<DBusEndOfStreamSignal> end_of_stream;
        } dbus;

        void set_playback_complete_cb(PlaybackCompleteCb cb, void *context)
        {
            playback_complete_cb = cb;
            playback_complete_context = context;
        }

        PlaybackCompleteCb playback_complete_cb;
        void *playback_complete_context;
        core::Signal<uint64_t> seeked_to;
        core::Signal<void> end_of_stream;
    } signals;
};

media::PlayerStub::PlayerStub(
    const std::shared_ptr<Service>& parent,
    const dbus::types::ObjectPath& object_path)
        : dbus::Stub<Player>(the_session_bus()),
          d(new Private{parent, access_service(), object_path})
{
    auto bus = the_session_bus();
    worker = std::move(std::thread([bus]()
    {
        bus->run();
    }));
}

media::PlayerStub::~PlayerStub()
{
}

std::shared_ptr<media::TrackList> media::PlayerStub::track_list()
{
    if (!d->track_list)
    {
        d->track_list = std::make_shared<media::TrackListStub>(
                    shared_from_this(),
                    dbus::types::ObjectPath(d->path.as_string() + "/TrackList"));
    }
    return d->track_list;
}

media::Player::PlayerKey media::PlayerStub::key() const
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Key, media::Player::PlayerKey>();

    return op.value();
}

bool media::PlayerStub::open_uri(const media::Track::UriType& uri)
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::OpenUri, bool>(uri);

    return op.value();
}

void media::PlayerStub::create_video_sink(uint32_t texture_id)
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::CreateVideoSink, void>(texture_id);
    d->texture_id = texture_id;
    d->get_gl_consumer();

    if (op.is_error())
        throw std::runtime_error("Problem creating new video sink instance on remote object");
}

GLConsumerWrapperHybris media::PlayerStub::gl_consumer() const
{
    return d->glc_wrapper;
}

void media::PlayerStub::next()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Next, void>();

    if (op.is_error())
        throw std::runtime_error("Problem switching to next track on remote object");
}

void media::PlayerStub::previous()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Previous, void>();

    if (op.is_error())
        throw std::runtime_error("Problem switching to previous track on remote object");
}

void media::PlayerStub::play()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Play, void>();

    if (op.is_error())
        throw std::runtime_error("Problem starting playback on remote object");
}

void media::PlayerStub::pause()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Pause, void>();

    if (op.is_error())
        throw std::runtime_error("Problem pausing playback on remote object");
}

void media::PlayerStub::seek_to(const std::chrono::microseconds& offset)
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Seek, void, uint64_t>(offset.count());

    if (op.is_error())
        throw std::runtime_error("Problem seeking on remote object");
}

void media::PlayerStub::stop()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Stop, void>();

    if (op.is_error())
        throw std::runtime_error("Problem stopping playback on remote object");
}

void media::PlayerStub::set_frame_available_callback(FrameAvailableCb cb, void *context)
{
    d->set_frame_available_cb(cb, context);
}

void media::PlayerStub::set_playback_complete_callback(PlaybackCompleteCb cb, void *context)
{
    d->signals.set_playback_complete_cb(cb, context);
}

const core::Property<bool>& media::PlayerStub::can_play() const
{
    return *d->properties.can_play;
}

const core::Property<bool>& media::PlayerStub::can_pause() const
{
    return *d->properties.can_pause;
}

const core::Property<bool>& media::PlayerStub::can_seek() const
{
    return *d->properties.can_seek;
}

const core::Property<bool>& media::PlayerStub::can_go_previous() const
{
    return *d->properties.can_go_previous;
}

const core::Property<bool>& media::PlayerStub::can_go_next() const
{
    return *d->properties.can_go_next;
}

const core::Property<bool>& media::PlayerStub::is_video_source() const
{
    return *d->properties.is_video_source;
}

const core::Property<bool>& media::PlayerStub::is_audio_source() const
{
    return *d->properties.is_audio_source;
}

const core::Property<media::Player::PlaybackStatus>& media::PlayerStub::playback_status() const
{
    return *d->properties.playback_status;
}

const core::Property<media::Player::LoopStatus>& media::PlayerStub::loop_status() const
{
    return *d->properties.loop_status;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerStub::playback_rate() const
{
    return *d->properties.playback_rate;
}

const core::Property<bool>& media::PlayerStub::is_shuffle() const
{
    return *d->properties.is_shuffle;
}

const core::Property<media::Track::MetaData>& media::PlayerStub::meta_data_for_current_track() const
{
    return *d->properties.meta_data_for_current_track;
}

const core::Property<media::Player::Volume>& media::PlayerStub::volume() const
{
    return *d->properties.volume;
}

const core::Property<uint64_t>& media::PlayerStub::position() const
{
    return *d->properties.position;
}

const core::Property<uint64_t>& media::PlayerStub::duration() const
{
    return *d->properties.duration;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerStub::minimum_playback_rate() const
{
    return *d->properties.minimum_playback_rate;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerStub::maximum_playback_rate() const
{
    return *d->properties.maximum_playback_rate;
}

core::Property<media::Player::LoopStatus>& media::PlayerStub::loop_status()
{
    return *d->properties.loop_status;
}

core::Property<media::Player::PlaybackRate>& media::PlayerStub::playback_rate()
{
    return *d->properties.playback_rate;
}

core::Property<bool>& media::PlayerStub::is_shuffle()
{
    return *d->properties.is_shuffle;
}

core::Property<media::Player::Volume>& media::PlayerStub::volume()
{
    return *d->properties.volume;
}

const core::Signal<uint64_t>& media::PlayerStub::seeked_to() const
{
    return d->signals.seeked_to;
}

const core::Signal<void>& media::PlayerStub::end_of_stream() const
{
    return d->signals.end_of_stream;
}
