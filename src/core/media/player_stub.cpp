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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */

#include <core/media/service.h>
#include <core/media/track_list.h>
#include <core/media/video/platform_default_sink.h>

#include "codec.h"
#include "player_stub.h"
#include "player_traits.h"
#include "property_stub.h"
#include "the_session_bus.h"
#include "track_list_stub.h"

#include "mpris/player.h"

#include <core/dbus/property.h>
#include <core/dbus/types/object_path.h>

#include <limits>

#define UNUSED __attribute__((unused))

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

struct media::PlayerStub::Private
{
    Private(const std::shared_ptr<Service>& parent,
            const std::shared_ptr<core::dbus::Object>& object
            ) : parent(parent),
                object(object),
                key(object->invoke_method_synchronously<mpris::Player::Key, media::Player::PlayerKey>().value()),
                properties
                {
                    // Link the properties from the server side to the client side over the bus
                    object->get_property<mpris::Player::Properties::CanPlay>(),
                    object->get_property<mpris::Player::Properties::CanPause>(),
                    object->get_property<mpris::Player::Properties::CanSeek>(),
                    object->get_property<mpris::Player::Properties::CanControl>(),
                    object->get_property<mpris::Player::Properties::CanGoNext>(),
                    object->get_property<mpris::Player::Properties::CanGoPrevious>(),
                    object->get_property<mpris::Player::Properties::IsVideoSource>(),
                    object->get_property<mpris::Player::Properties::IsAudioSource>(),
                    object->get_property<mpris::Player::Properties::TypedPlaybackStatus>(),
                    object->get_property<mpris::Player::Properties::TypedLoopStatus>(),
                    object->get_property<mpris::Player::Properties::PlaybackRate>(),
                    object->get_property<mpris::Player::Properties::Shuffle>(),
                    object->get_property<mpris::Player::Properties::TypedMetaData>(),
                    object->get_property<mpris::Player::Properties::Volume>(),
                    object->get_property<mpris::Player::Properties::Position>(),
                    object->get_property<mpris::Player::Properties::Duration>(),
                    object->get_property<mpris::Player::Properties::AudioStreamRole>(),
                    object->get_property<mpris::Player::Properties::Orientation>(),
                    object->get_property<mpris::Player::Properties::Lifetime>(),
                    object->get_property<mpris::Player::Properties::MinimumRate>(),
                    object->get_property<mpris::Player::Properties::MaximumRate>()
                },
                signals
                {
                    object->get_signal<mpris::Player::Signals::Seeked>(),
                    object->get_signal<mpris::Player::Signals::EndOfStream>(),
                    object->get_signal<mpris::Player::Signals::PlaybackStatusChanged>(),
                    object->get_signal<mpris::Player::Signals::VideoDimensionChanged>()
                }
    {
    }

    ~Private()
    {
    }

    std::shared_ptr<Service> parent;
    std::shared_ptr<TrackList> track_list;

    dbus::Object::Ptr object;
    media::Player::PlayerKey key;
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

        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::TypedPlaybackStatus>> playback_status;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::TypedLoopStatus>> loop_status;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::PlaybackRate>> playback_rate;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Shuffle>> is_shuffle;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::TypedMetaData>> meta_data_for_current_track;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Volume>> volume;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Position>> position;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Duration>> duration;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::AudioStreamRole>> audio_role;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Orientation>> orientation;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Lifetime>> lifetime;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::MinimumRate>> minimum_playback_rate;
        std::shared_ptr<core::dbus::Property<mpris::Player::Properties::MaximumRate>> maximum_playback_rate;
    } properties;

    struct Signals
    {
        typedef core::dbus::Signal<mpris::Player::Signals::Seeked, mpris::Player::Signals::Seeked::ArgumentType> DBusSeekedToSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::EndOfStream, mpris::Player::Signals::EndOfStream::ArgumentType> DBusEndOfStreamSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::PlaybackStatusChanged, mpris::Player::Signals::PlaybackStatusChanged::ArgumentType> DBusPlaybackStatusChangedSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::VideoDimensionChanged, mpris::Player::Signals::VideoDimensionChanged::ArgumentType> DBusVideoDimensionChangedSignal;

        Signals(const std::shared_ptr<DBusSeekedToSignal>& seeked,
                const std::shared_ptr<DBusEndOfStreamSignal>& eos,
                const std::shared_ptr<DBusPlaybackStatusChangedSignal>& status,
                const std::shared_ptr<DBusVideoDimensionChangedSignal>& d)
            : seeked_to(),
              end_of_stream(),
              playback_status_changed(),
              video_dimension_changed(),
              dbus
              {
                  seeked,
                  eos,
                  status,
                  d
              }
        {
            dbus.seeked_to->connect([this](std::uint64_t value)
            {
                std::cout << "seeked_to signal arrived via the bus." << std::endl;
                seeked_to(value);
            });

            dbus.end_of_stream->connect([this]()
            {
                std::cout << "EndOfStream signal arrived via the bus." << std::endl;
                end_of_stream();
            });

            dbus.playback_status_changed->connect([this](const media::Player::PlaybackStatus& status)
            {
                std::cout << "PlaybackStatusChanged signal arrived via the bus." << std::endl;
                playback_status_changed(status);
            });

            dbus.video_dimension_changed->connect([this](const media::video::Dimensions dimensions)
            {
                std::cout << "VideoDimensionChanged signal arrived via the bus." << std::endl;
                video_dimension_changed(dimensions);
            });
        }

        core::Signal<int64_t> seeked_to;
        core::Signal<void> end_of_stream;
        core::Signal<media::Player::PlaybackStatus> playback_status_changed;
        core::Signal<media::video::Dimensions> video_dimension_changed;

        struct DBus
        {
            std::shared_ptr<DBusSeekedToSignal> seeked_to;
            std::shared_ptr<DBusEndOfStreamSignal> end_of_stream;
            std::shared_ptr<DBusPlaybackStatusChangedSignal> playback_status_changed;
            std::shared_ptr<DBusVideoDimensionChangedSignal> video_dimension_changed;
        } dbus;
    } signals;
};

media::PlayerStub::PlayerStub(
    const std::shared_ptr<Service>& parent,
    const std::shared_ptr<core::dbus::Object>& object)
        : d(new Private{parent, object})
{
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
                    dbus::types::ObjectPath(d->object->path().as_string() + "/TrackList"));
    }
    return d->track_list;
}

media::Player::PlayerKey media::PlayerStub::key() const
{
    return d->key;
}

bool media::PlayerStub::open_uri(const media::Track::UriType& uri)
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::OpenUri, bool>(uri);

    return op.value();
}


bool media::PlayerStub::open_uri(const Track::UriType& uri, const Player::HeadersType& headers)
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::OpenUriExtended, bool>(uri, headers);

    return op.value();
}

media::video::Sink::Ptr media::PlayerStub::create_gl_texture_video_sink(std::uint32_t texture_id)
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::CreateVideoSink, void>(texture_id);

    if (op.is_error())
    {
        if (op.error().name() == mpris::Player::Error::OutOfProcessBufferStreamingNotSupported::name)
            throw media::Player::Error::OutOfProcessBufferStreamingNotSupported{};
        else
            throw std::runtime_error{op.error().print()};
    }

    return media::video::make_platform_default_sink(texture_id, d->key);
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

const core::Property<int64_t>& media::PlayerStub::position() const
{
    return *d->properties.position;
}

const core::Property<int64_t>& media::PlayerStub::duration() const
{
    return *d->properties.duration;
}

const core::Property<media::Player::AudioStreamRole>& media::PlayerStub::audio_stream_role() const
{
    return *d->properties.audio_role;
}

const core::Property<media::Player::Orientation>& media::PlayerStub::orientation() const
{
    return *d->properties.orientation;
}

const core::Property<media::Player::Lifetime>& media::PlayerStub::lifetime() const
{
    return *d->properties.lifetime;
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

core::Property<media::Player::AudioStreamRole>& media::PlayerStub::audio_stream_role()
{
    return *d->properties.audio_role;
}

core::Property<media::Player::Lifetime>& media::PlayerStub::lifetime()
{
    return *d->properties.lifetime;
}

const core::Signal<int64_t>& media::PlayerStub::seeked_to() const
{
    return d->signals.seeked_to;
}

const core::Signal<void>& media::PlayerStub::end_of_stream() const
{
    return d->signals.end_of_stream;
}

core::Signal<media::Player::PlaybackStatus>& media::PlayerStub::playback_status_changed()
{
    return d->signals.playback_status_changed;
}

const core::Signal<media::video::Dimensions>& media::PlayerStub::video_dimension_changed() const
{
    return d->signals.video_dimension_changed;
}
