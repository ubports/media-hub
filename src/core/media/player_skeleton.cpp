/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include "codec.h"
#include "engine.h"
#include "external_services.h"
#include "player_skeleton.h"
#include "player_traits.h"
#include "property_stub.h"
#include "the_session_bus.h"
#include "xesam.h"

#include "apparmor/ubuntu.h"
#include "mpris/media_player2.h"
#include "mpris/metadata.h"
#include "mpris/player.h"
#include "mpris/playlists.h"

#include "core/media/logger/logger.h"
#include "util/uri_check.h"

#include <core/dbus/object.h>
#include <core/dbus/property.h>
#include <core/dbus/stub.h>

#include <core/dbus/asio/executor.h>
#include <core/dbus/interfaces/properties.h>

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

struct media::PlayerSkeleton::Private
{
    Private(media::PlayerSkeleton* player,
            const std::shared_ptr<core::dbus::Bus>& bus,
            const std::shared_ptr<core::dbus::Object>& session,
            const apparmor::ubuntu::RequestContextResolver::Ptr& request_context_resolver,
            const apparmor::ubuntu::RequestAuthenticator::Ptr& request_authenticator)
        : impl(player),
          bus(bus),
          object(session),
          request_context_resolver{request_context_resolver},
          request_authenticator{request_authenticator},
          uri_check(std::make_shared<UriCheck>()),
          skeleton{mpris::Player::Skeleton::Configuration{bus, session, mpris::Player::Skeleton::Configuration::Defaults{}}},
          signals
          {
              skeleton.signals.seeked_to,
              skeleton.signals.about_to_finish,
              skeleton.signals.end_of_stream,
              skeleton.signals.playback_status_changed,
              skeleton.signals.video_dimension_changed,
              skeleton.signals.error,
              skeleton.signals.buffering_changed
          }
    {
    }

    void handle_next(const core::dbus::Message::Ptr& msg)
    {
        impl->next();
        auto reply = dbus::Message::make_method_return(msg);
        bus->send(reply);
    }

    void handle_previous(const core::dbus::Message::Ptr& msg)
    {
        impl->previous();
        auto reply = dbus::Message::make_method_return(msg);
        bus->send(reply);
    }

    void handle_pause(const core::dbus::Message::Ptr& msg)
    {
        impl->pause();
        auto reply = dbus::Message::make_method_return(msg);
        bus->send(reply);
    }

    void handle_stop(const core::dbus::Message::Ptr& msg)
    {
        impl->stop();
        auto reply = dbus::Message::make_method_return(msg);
        bus->send(reply);
    }

    void handle_play(const core::dbus::Message::Ptr& msg)
    {
        impl->play();
        auto reply = dbus::Message::make_method_return(msg);
        bus->send(reply);
    }

    void handle_play_pause(const core::dbus::Message::Ptr& msg)
    {
        switch(impl->playback_status().get())
        {
        case core::ubuntu::media::Player::PlaybackStatus::ready:
        case core::ubuntu::media::Player::PlaybackStatus::paused:
        case core::ubuntu::media::Player::PlaybackStatus::stopped:
            impl->play();
            break;
        case core::ubuntu::media::Player::PlaybackStatus::playing:
            impl->pause();
            break;
        default:
            break;
        }

        bus->send(dbus::Message::make_method_return(msg));
    }

    void handle_seek(const core::dbus::Message::Ptr& in)
    {
        uint64_t ticks;
        in->reader() >> ticks;
        impl->seek_to(std::chrono::microseconds(ticks));

        auto reply = dbus::Message::make_method_return(in);
        bus->send(reply);
    }

    void handle_set_position(const core::dbus::Message::Ptr&)
    {
    }

    void handle_create_video_sink(const core::dbus::Message::Ptr& in)
    {
        uint32_t texture_id;
        in->reader() >> texture_id;

        core::dbus::Message::Ptr reply;

        try
        {
            impl->create_gl_texture_video_sink(texture_id);
            reply = dbus::Message::make_method_return(in);
        }
        catch (const media::Player::Errors::OutOfProcessBufferStreamingNotSupported& e)
        {
            reply = dbus::Message::make_error(
                        in,
                        mpris::Player::Error::OutOfProcessBufferStreamingNotSupported::name,
                        e.what());
        }
        catch (...)
        {
            reply = dbus::Message::make_error(
                        in,
                        mpris::Player::Error::OutOfProcessBufferStreamingNotSupported::name,
                        std::string{});
        }

        bus->send(reply);
    }

    void handle_key(const core::dbus::Message::Ptr& in)
    {
        auto reply = dbus::Message::make_method_return(in);
        reply->writer() << impl->key();
        bus->send(reply);
    }

    void handle_open_uri(const core::dbus::Message::Ptr& in)
    {
        request_context_resolver->resolve_context_for_dbus_name_async(in->sender(), [this, in](const media::apparmor::ubuntu::Context& context)
        {
            Track::UriType uri;
            in->reader() >> uri;

            auto reply = dbus::Message::make_method_return(in);
            uri_check->set(uri);
            const bool valid_uri = !uri_check->is_local_file() or
                    (uri_check->is_local_file() and uri_check->file_exists());
            if (!valid_uri)
            {
                const std::string err_str = {"Warning: Failed to open uri " + uri +
                     " because it can't be found."};
                MH_ERROR("%s", err_str);
                reply = dbus::Message::make_error(
                            in,
                            mpris::Player::Error::UriNotFound::name,
                            err_str);
            }
            else
            {
                // Make sure the client has adequate apparmor permissions to open the URI
                const auto result = request_authenticator->authenticate_open_uri_request(context, uri);
                if (std::get<0>(result))
                {
                    reply->writer() << (std::get<0>(result) ? impl->open_uri(uri) : false);
                }
                else
                {
                    const std::string err_str = {"Warning: Failed to authenticate necessary "
                        "apparmor permissions to open uri: " + std::get<1>(result)};
                    MH_ERROR("%s", err_str);
                    reply = dbus::Message::make_error(
                                in,
                                mpris::Player::Error::InsufficientAppArmorPermissions::name,
                                err_str);
                }
            }

            bus->send(reply);
        });
    }

    void handle_open_uri_extended(const core::dbus::Message::Ptr& in)
    {
        request_context_resolver->resolve_context_for_dbus_name_async(in->sender(), [this, in](const media::apparmor::ubuntu::Context& context)
        {
            Track::UriType uri;
            Player::HeadersType headers;

            in->reader() >> uri >> headers;

            auto reply = dbus::Message::make_method_return(in);
            uri_check->set(uri);
            const bool valid_uri = !uri_check->is_local_file() or
                    (uri_check->is_local_file() and uri_check->file_exists());
            if (!valid_uri)
            {
                const std::string err_str = {"Warning: Failed to open uri " + uri +
                     " because it can't be found."};
                MH_ERROR("%s", err_str);
                reply = dbus::Message::make_error(
                            in,
                            mpris::Player::Error::UriNotFound::name,
                            err_str);
            }
            else
            {
                // Make sure the client has adequate apparmor permissions to open the URI
                const auto result = request_authenticator->authenticate_open_uri_request(context, uri);
                if (std::get<0>(result))
                {
                    reply->writer() << (std::get<0>(result) ? impl->open_uri(uri, headers) : false);
                }
                else
                {
                    const std::string err_str = {"Warning: Failed to authenticate necessary "
                        "apparmor permissions to open uri: " + std::get<1>(result)};
                    MH_ERROR("%s", err_str);
                    reply = dbus::Message::make_error(
                                in,
                                mpris::Player::Error::InsufficientAppArmorPermissions::name,
                                err_str);
                }
            }

            bus->send(reply);
        });
    }

    template<typename Property>
    void on_property_value_changed(
            const typename Property::ValueType& value,
            const dbus::Signal
            <
                core::dbus::interfaces::Properties::Signals::PropertiesChanged,
                core::dbus::interfaces::Properties::Signals::PropertiesChanged::ArgumentType
            >::Ptr& signal)
    {
        typedef std::map<std::string, dbus::types::Variant> Dictionary;

        static const std::vector<std::string> the_empty_list_of_invalidated_properties;

        Dictionary dict; dict[Property::name()] = dbus::types::Variant::encode(value);

        signal->emit(std::make_tuple(
                        dbus::traits::Service<typename Property::Interface>::interface_name(),
                        dict,
                        the_empty_list_of_invalidated_properties));
    }

    media::PlayerSkeleton* impl;
    dbus::Bus::Ptr bus;
    dbus::Object::Ptr object;
    media::apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
    media::apparmor::ubuntu::RequestAuthenticator::Ptr request_authenticator;
    media::UriCheck::Ptr uri_check;

    mpris::Player::Skeleton skeleton;

    struct Signals
    {
        typedef core::dbus::Signal<mpris::Player::Signals::Seeked, mpris::Player::Signals::Seeked::ArgumentType> DBusSeekedToSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::EndOfStream, mpris::Player::Signals::EndOfStream::ArgumentType> DBusEndOfStreamSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::AboutToFinish, mpris::Player::Signals::AboutToFinish::ArgumentType> DBusAboutToFinishSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::PlaybackStatusChanged, mpris::Player::Signals::PlaybackStatusChanged::ArgumentType> DBusPlaybackStatusChangedSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::VideoDimensionChanged, mpris::Player::Signals::VideoDimensionChanged::ArgumentType> DBusVideoDimensionChangedSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::Error, mpris::Player::Signals::Error::ArgumentType> DBusErrorSignal;
        typedef core::dbus::Signal<mpris::Player::Signals::Buffering, mpris::Player::Signals::Buffering::ArgumentType> DBusBufferingChangedSignal;

        Signals(const std::shared_ptr<DBusSeekedToSignal>& remote_seeked,
                const std::shared_ptr<DBusAboutToFinishSignal>& remote_atf,
                const std::shared_ptr<DBusEndOfStreamSignal>& remote_eos,
                const std::shared_ptr<DBusPlaybackStatusChangedSignal>& remote_playback_status_changed,
                const std::shared_ptr<DBusVideoDimensionChangedSignal>& remote_video_dimension_changed,
                const std::shared_ptr<DBusErrorSignal>& remote_error,
                const std::shared_ptr<DBusBufferingChangedSignal>& remote_buffering_changed)
        {
            seeked_to.connect([remote_seeked](std::uint64_t value)
            {
                remote_seeked->emit(value);
            });

            about_to_finish.connect([remote_atf]()
            {
                remote_atf->emit();
            });

            end_of_stream.connect([remote_eos]()
            {
                remote_eos->emit();
            });

            playback_status_changed.connect([remote_playback_status_changed](const media::Player::PlaybackStatus& status)
            {
                remote_playback_status_changed->emit(status);
            });

            video_dimension_changed.connect([remote_video_dimension_changed](const media::video::Dimensions& dimensions)
            {
                remote_video_dimension_changed->emit(dimensions);
            });

            error.connect([remote_error](const media::Player::Error& e)
            {
                remote_error->emit(e);
            });

            buffering_changed.connect([remote_buffering_changed](int value)
            {
                remote_buffering_changed->emit(value);
            });

        }

        core::Signal<int64_t> seeked_to;
        core::Signal<void> about_to_finish;
        core::Signal<void> end_of_stream;
        core::Signal<media::Player::PlaybackStatus> playback_status_changed;
        core::Signal<media::video::Dimensions> video_dimension_changed;
        core::Signal<media::Player::Error> error;
        core::Signal<int> buffering_changed;
    } signals;

};

media::PlayerSkeleton::PlayerSkeleton(const media::PlayerSkeleton::Configuration& config)
        : d(new Private{this, config.bus, config.session, config.request_context_resolver, config.request_authenticator})
{
    // Setup method handlers for mpris::Player methods.
    auto next = std::bind(&Private::handle_next, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::Next>(next);

    auto previous = std::bind(&Private::handle_previous, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::Previous>(previous);

    auto pause = std::bind(&Private::handle_pause, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::Pause>(pause);

    auto stop = std::bind(&Private::handle_stop, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::Stop>(stop);

    auto play = std::bind(&Private::handle_play, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::Play>(play);

    auto play_pause = std::bind(&Private::handle_play_pause, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::PlayPause>(play_pause);

    auto seek = std::bind(&Private::handle_seek, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::Seek>(seek);

    auto set_position = std::bind(&Private::handle_set_position, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::SetPosition>(set_position);

    auto open_uri = std::bind(&Private::handle_open_uri, d, std::placeholders::_1);
    d->object->install_method_handler<mpris::Player::OpenUri>(open_uri);

    // All the method handlers that exceed the mpris spec go here.
    d->object->install_method_handler<mpris::Player::CreateVideoSink>(
        std::bind(&Private::handle_create_video_sink,
                  d,
                  std::placeholders::_1));

    d->object->install_method_handler<mpris::Player::Key>(
        std::bind(&Private::handle_key,
                  d,
                  std::placeholders::_1));

    d->object->install_method_handler<mpris::Player::OpenUriExtended>(
        std::bind(&Private::handle_open_uri_extended,
                  d,
                  std::placeholders::_1));
}

media::PlayerSkeleton::~PlayerSkeleton()
{
   // The session object may outlive the private instance
   // so uninstall all method handlers.
   d->object->uninstall_method_handler<mpris::Player::Next>();
   d->object->uninstall_method_handler<mpris::Player::Previous>();
   d->object->uninstall_method_handler<mpris::Player::Pause>();
   d->object->uninstall_method_handler<mpris::Player::Stop>();
   d->object->uninstall_method_handler<mpris::Player::Play>();
   d->object->uninstall_method_handler<mpris::Player::PlayPause>();
   d->object->uninstall_method_handler<mpris::Player::Seek>();
   d->object->uninstall_method_handler<mpris::Player::SetPosition>();
   d->object->uninstall_method_handler<mpris::Player::OpenUri>();
   d->object->uninstall_method_handler<mpris::Player::CreateVideoSink>();
   d->object->uninstall_method_handler<mpris::Player::Key>();
   d->object->uninstall_method_handler<mpris::Player::OpenUriExtended>();
}

const core::Property<bool>& media::PlayerSkeleton::can_play() const
{
    return *d->skeleton.properties.can_play;
}

const core::Property<bool>& media::PlayerSkeleton::can_pause() const
{
    return *d->skeleton.properties.can_pause;
}

const core::Property<bool>& media::PlayerSkeleton::can_seek() const
{
    return *d->skeleton.properties.can_seek;
}

const core::Property<bool>& media::PlayerSkeleton::can_go_previous() const
{
    return *d->skeleton.properties.can_go_previous;
}

const core::Property<bool>& media::PlayerSkeleton::can_go_next() const
{
    return *d->skeleton.properties.can_go_next;
}

const core::Property<bool>& media::PlayerSkeleton::is_video_source() const
{
    return *d->skeleton.properties.is_video_source;
}

const core::Property<bool>& media::PlayerSkeleton::is_audio_source() const
{
    return *d->skeleton.properties.is_audio_source;
}

const core::Property<media::Player::PlaybackStatus>& media::PlayerSkeleton::playback_status() const
{
    return *d->skeleton.properties.typed_playback_status;
}

const core::Property<media::AVBackend::Backend>& media::PlayerSkeleton::backend() const
{
    return *d->skeleton.properties.typed_backend;
}

const core::Property<media::Player::LoopStatus>& media::PlayerSkeleton::loop_status() const
{
    return *d->skeleton.properties.typed_loop_status;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::playback_rate() const
{
    return *d->skeleton.properties.playback_rate;
}

const core::Property<bool>& media::PlayerSkeleton::shuffle() const
{
    return *d->skeleton.properties.shuffle;
}

const core::Property<media::Track::MetaData>& media::PlayerSkeleton::meta_data_for_current_track() const
{
    return *d->skeleton.properties.meta_data_for_current_track;
}

const core::Property<media::Player::Volume>& media::PlayerSkeleton::volume() const
{
    return *d->skeleton.properties.volume;
}

const core::Property<int64_t>& media::PlayerSkeleton::position() const
{
    return *d->skeleton.properties.position;
}

const core::Property<int64_t>& media::PlayerSkeleton::duration() const
{
    return *d->skeleton.properties.duration;
}

const core::Property<media::Player::AudioStreamRole>& media::PlayerSkeleton::audio_stream_role() const
{
    return *d->skeleton.properties.audio_stream_role;
}

const core::Property<media::Player::Orientation>& media::PlayerSkeleton::orientation() const
{
    return *d->skeleton.properties.orientation;
}

const core::Property<media::Player::Lifetime>& media::PlayerSkeleton::lifetime() const
{
    return *d->skeleton.properties.lifetime;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::minimum_playback_rate() const
{
    return *d->skeleton.properties.minimum_playback_rate;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::maximum_playback_rate() const
{
    return *d->skeleton.properties.maximum_playback_rate;
}

core::Property<media::Player::LoopStatus>& media::PlayerSkeleton::loop_status()
{
    return *d->skeleton.properties.typed_loop_status;
}

core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::playback_rate()
{
    return *d->skeleton.properties.playback_rate;
}

core::Property<bool>& media::PlayerSkeleton::shuffle()
{
    return *d->skeleton.properties.shuffle;
}

core::Property<media::Player::Volume>& media::PlayerSkeleton::volume()
{
    return *d->skeleton.properties.volume;
}

core::Property<int64_t>& media::PlayerSkeleton::position()
{
    return *d->skeleton.properties.position;
}

core::Property<int64_t>& media::PlayerSkeleton::duration()
{
    return *d->skeleton.properties.duration;
}

core::Property<media::Player::AudioStreamRole>& media::PlayerSkeleton::audio_stream_role()
{
    return *d->skeleton.properties.audio_stream_role;
}

core::Property<media::Player::Orientation>& media::PlayerSkeleton::orientation()
{
    return *d->skeleton.properties.orientation;
}

core::Property<media::Player::Lifetime>& media::PlayerSkeleton::lifetime()
{
    return *d->skeleton.properties.lifetime;
}

core::Property<media::Player::PlaybackStatus>& media::PlayerSkeleton::playback_status()
{
    return *d->skeleton.properties.typed_playback_status;
}

core::Property<media::AVBackend::Backend>& media::PlayerSkeleton::backend()
{
    return *d->skeleton.properties.typed_backend;
}

core::Property<bool>& media::PlayerSkeleton::can_play()
{
    return *d->skeleton.properties.can_play;
}

core::Property<bool>& media::PlayerSkeleton::can_pause()
{
    return *d->skeleton.properties.can_pause;
}

core::Property<bool>& media::PlayerSkeleton::can_seek()
{
    return *d->skeleton.properties.can_seek;
}

core::Property<bool>& media::PlayerSkeleton::can_go_previous()
{
    return *d->skeleton.properties.can_go_previous;
}

core::Property<bool>& media::PlayerSkeleton::can_go_next()
{
    return *d->skeleton.properties.can_go_next;
}

core::Property<bool>& media::PlayerSkeleton::is_video_source()
{
    return *d->skeleton.properties.is_video_source;
}

core::Property<bool>& media::PlayerSkeleton::is_audio_source()
{
    return *d->skeleton.properties.is_audio_source;
}

core::Property<media::Track::MetaData>& media::PlayerSkeleton::meta_data_for_current_track()
{
    return *d->skeleton.properties.meta_data_for_current_track;
}

core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::minimum_playback_rate()
{
    return *d->skeleton.properties.minimum_playback_rate;
}

core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::maximum_playback_rate()
{
    return *d->skeleton.properties.maximum_playback_rate;
}

const core::Signal<int64_t>& media::PlayerSkeleton::seeked_to() const
{
    return d->signals.seeked_to;
}

core::Signal<int64_t>& media::PlayerSkeleton::seeked_to()
{
    return d->signals.seeked_to;
}

const core::Signal<void>& media::PlayerSkeleton::about_to_finish() const
{
    return d->signals.about_to_finish;
}

core::Signal<void>& media::PlayerSkeleton::about_to_finish()
{
    return d->signals.about_to_finish;
}

const core::Signal<void>& media::PlayerSkeleton::end_of_stream() const
{
    return d->signals.end_of_stream;
}

core::Signal<void>& media::PlayerSkeleton::end_of_stream()
{
    return d->signals.end_of_stream;
}

core::Signal<media::Player::PlaybackStatus>& media::PlayerSkeleton::playback_status_changed()
{
    return d->signals.playback_status_changed;
}

const core::Signal<media::video::Dimensions>& media::PlayerSkeleton::video_dimension_changed() const
{
    return d->signals.video_dimension_changed;
}

core::Signal<media::video::Dimensions>& media::PlayerSkeleton::video_dimension_changed()
{
    return d->signals.video_dimension_changed;
}

core::Signal<media::Player::Error>& media::PlayerSkeleton::error()
{
    return d->signals.error;
}

const core::Signal<media::Player::Error>& media::PlayerSkeleton::error() const
{
    return d->signals.error;
}

const core::Signal<int>& media::PlayerSkeleton::buffering_changed() const
{
    return d->signals.buffering_changed;
}

core::Signal<int>& media::PlayerSkeleton::buffering_changed()
{
    return d->signals.buffering_changed;
}
