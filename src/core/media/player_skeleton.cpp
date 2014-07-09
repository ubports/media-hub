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
 */

#include "apparmor.h"
#include "codec.h"
#include "player_skeleton.h"
#include "player_traits.h"
#include "property_stub.h"
#include "the_session_bus.h"

#include "mpris/player.h"

#include <core/dbus/object.h>
#include <core/dbus/property.h>
#include <core/dbus/stub.h>
#include <core/dbus/asio/executor.h>

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

struct media::PlayerSkeleton::Private
{
    Private(media::PlayerSkeleton* player, const dbus::types::ObjectPath& session)
        : impl(player),
          object(impl->access_service()->add_object_for_path(session)),
          apparmor_session(nullptr),
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
              object->get_signal<mpris::Player::Signals::EndOfStream>(),
              object->get_signal<mpris::Player::Signals::PlaybackStatusChanged>()
          }
    {
    }

    void handle_next(const core::dbus::Message::Ptr& msg)
    {
        impl->next();
        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    void handle_previous(const core::dbus::Message::Ptr& msg)
    {
        impl->previous();
        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    void handle_pause(const core::dbus::Message::Ptr& msg)
    {
        impl->pause();
        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    void handle_playpause(DBusMessage*)
    {
    }

    void handle_stop(const core::dbus::Message::Ptr& msg)
    {
        impl->stop();
        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    void handle_play(const core::dbus::Message::Ptr& msg)
    {
        impl->play();
        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    void handle_seek(const core::dbus::Message::Ptr& in)
    {
        uint64_t ticks;
        in->reader() >> ticks;
        impl->seek_to(std::chrono::microseconds(ticks));

        auto reply = dbus::Message::make_method_return(in);
        impl->access_bus()->send(reply);
    }

    void handle_set_position(const core::dbus::Message::Ptr&)
    {
    }

    void handle_create_video_sink(const core::dbus::Message::Ptr& in)
    {
        uint32_t texture_id;
        in->reader() >> texture_id;
        impl->create_video_sink(texture_id);

        auto reply = dbus::Message::make_method_return(in);
        impl->access_bus()->send(reply);
    }

    std::string get_client_apparmor_context(const core::dbus::Message::Ptr& msg)
    {
        auto bus = std::shared_ptr<dbus::Bus>(new dbus::Bus(core::dbus::WellKnownBus::session));
        bus->install_executor(dbus::asio::make_executor(bus));

        auto stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::Apparmor>::interface_name());
        apparmor_session = stub_service->object_for_path(dbus::types::ObjectPath("/org/freedesktop/DBus"));
        // Get the AppArmor security context for the client
        auto result = apparmor_session->invoke_method_synchronously<core::Apparmor::getConnectionAppArmorSecurityContext, std::string>(msg->sender());
        if (result.is_error())
        {
            std::cout << "Error getting apparmor profile: " << result.error().print() << std::endl;
            return std::string();
        }

        return result.value();
    }

    bool does_client_have_access(const std::string& context, const std::string& uri)
    {
        if (context.empty() || uri.empty())
        {
            std::cout << "Client denied access since context or uri are empty" << std::endl;
            return false;
        }

        if (context == "unconfined")
        {
            std::cout << "Client allowed access since it's unconfined" << std::endl;
            return true;
        }

        size_t pos = context.find_first_of('_');
        if (pos == std::string::npos)
        {
            std::cout << "Client denied access since it's an invalid apparmor security context" << std::endl;
            return false;
        }

        const std::string pkgname = context.substr(0, pos);
        std::cout << "client pkgname: " << pkgname << std::endl;
        std::cout << "uri: " << uri << std::endl;

        // All confined apps can access their own files
        if (uri.find(std::string(".local/share/" + pkgname + "/")) != std::string::npos
                || uri.find(std::string(".cache/" + pkgname + "/")) != std::string::npos)
        {
            std::cout << "Client can access content in ~/.local/share/" << pkgname << " or ~/.cache/" << pkgname << std::endl;
            return true;
        }
        else if (uri.find(std::string("opt/click.ubuntu.com/")) != std::string::npos
                && uri.find(pkgname) != std::string::npos)
        {
            std::cout << "Client can access content in own opt directory" << std::endl;
            return true;
        }
        else if ((uri.find(std::string("/system/media/audio/ui/")) != std::string::npos
                || uri.find(std::string("/android/system/media/audio/ui/")) != std::string::npos)
                && pkgname == "com.ubuntu.camera")
        {
            std::cout << "Camera app can access ui sounds" << std::endl;
            return true;
        }
        // TODO: Check if the trust store previously allowed direct access to uri

        // Check in ~/Music and ~/Videos
        // TODO: when the trust store lands, check it to see if this app can access the dirs and
        // then remove the explicit whitelist of the music-app, and gallery-app
        else if ((pkgname == "com.ubuntu.music" || pkgname == "com.ubuntu.gallery") &&
                (uri.find(std::string("Music/")) != std::string::npos
                || uri.find(std::string("Videos/")) != std::string::npos))
        {
            std::cout << "Client can access content in ~/Music or ~/Videos" << std::endl;
            return true;
        }
        else if (uri.find(std::string("http://")) != std::string::npos
                || uri.find(std::string("rtsp://")) != std::string::npos)
        {
            std::cout << "Client can access streaming content" << std::endl;
            return true;
        }
        else
        {
            std::cout << "Client denied access to open_uri()" << std::endl;
            return false;
        }
    }

    void handle_key(const core::dbus::Message::Ptr& in)
    {
        auto reply = dbus::Message::make_method_return(in);
        reply->writer() << impl->key();
        impl->access_bus()->send(reply);
    }

    void handle_open_uri(const core::dbus::Message::Ptr& in)
    {
        Track::UriType uri;
        in->reader() >> uri;

        std::string context = get_client_apparmor_context(in);
        bool have_access = does_client_have_access(context, uri);

        auto reply = dbus::Message::make_method_return(in);
        if (have_access)
            reply->writer() << impl->open_uri(uri);
        else
            reply->writer() << false;
        impl->access_bus()->send(reply);
    }

    void handle_open_uri_extended(const core::dbus::Message::Ptr& in)
    {
        Track::UriType uri, cookies;
        std::string user_agent;
        in->reader() >> uri;
        in->reader() >> cookies;
        in->reader() >> user_agent;

        std::string context = get_client_apparmor_context(in);
        bool have_access = does_client_have_access(context, uri);

        auto reply = dbus::Message::make_method_return(in);
        if (have_access)
            reply->writer() << impl->open_uri(uri, cookies, user_agent);
        else
            reply->writer() << false;
        impl->access_bus()->send(reply);
    }

    media::PlayerSkeleton* impl;
    dbus::Object::Ptr object;
    dbus::Object::Ptr apparmor_session;
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
        typedef core::dbus::Signal<mpris::Player::Signals::PlaybackStatusChanged, mpris::Player::Signals::PlaybackStatusChanged::ArgumentType> DBusPlaybackStatusChangedSignal;

        Signals(const std::shared_ptr<DBusSeekedToSignal>& seeked,
                const std::shared_ptr<DBusEndOfStreamSignal>& eos,
                const std::shared_ptr<DBusPlaybackStatusChangedSignal>& status)
            : dbus
              {
                  seeked,
                  eos,
                  status
              },
              seeked_to(),
              end_of_stream(),
              playback_status_changed()
        {
            seeked_to.connect([this](std::uint64_t value)
            {
                dbus.seeked_to->emit(value);
            });

            end_of_stream.connect([this]()
            {
                dbus.end_of_stream->emit();
            });

            playback_status_changed.connect([this](const media::Player::PlaybackStatus& status)
            {
                dbus.playback_status_changed->emit(status);
            });
        }

        struct DBus
        {
            std::shared_ptr<DBusSeekedToSignal> seeked_to;
            std::shared_ptr<DBusEndOfStreamSignal> end_of_stream;
            std::shared_ptr<DBusPlaybackStatusChangedSignal> playback_status_changed;
        } dbus;
        core::Signal<uint64_t> seeked_to;
        core::Signal<void> end_of_stream;
        core::Signal<media::Player::PlaybackStatus> playback_status_changed;
    } signals;

};

media::PlayerSkeleton::PlayerSkeleton(
    const core::dbus::types::ObjectPath& session_path)
        : dbus::Skeleton<media::Player>(the_session_bus()),
          d(new Private{this, session_path})
{
    d->object->install_method_handler<mpris::Player::Next>(
        std::bind(&Private::handle_next,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Previous>(
        std::bind(&Private::handle_previous,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Pause>(
        std::bind(&Private::handle_pause,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Stop>(
        std::bind(&Private::handle_stop,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Play>(
        std::bind(&Private::handle_play,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Seek>(
        std::bind(&Private::handle_seek,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::SetPosition>(
        std::bind(&Private::handle_set_position,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::CreateVideoSink>(
        std::bind(&Private::handle_create_video_sink,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Key>(
        std::bind(&Private::handle_key,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::OpenUri>(
        std::bind(&Private::handle_open_uri,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::OpenUriExtended>(
        std::bind(&Private::handle_open_uri_extended,
                  std::ref(d),
                  std::placeholders::_1));
}

media::PlayerSkeleton::~PlayerSkeleton()
{
}

const core::Property<bool>& media::PlayerSkeleton::can_play() const
{
    return *d->properties.can_play;
}

const core::Property<bool>& media::PlayerSkeleton::can_pause() const
{
    return *d->properties.can_pause;
}

const core::Property<bool>& media::PlayerSkeleton::can_seek() const
{
    return *d->properties.can_seek;
}

const core::Property<bool>& media::PlayerSkeleton::can_go_previous() const
{
    return *d->properties.can_go_previous;
}

const core::Property<bool>& media::PlayerSkeleton::can_go_next() const
{
    return *d->properties.can_go_next;
}

const core::Property<bool>& media::PlayerSkeleton::is_video_source() const
{
    return *d->properties.is_video_source;
}

const core::Property<bool>& media::PlayerSkeleton::is_audio_source() const
{
    return *d->properties.is_audio_source;
}

const core::Property<media::Player::PlaybackStatus>& media::PlayerSkeleton::playback_status() const
{
    return *d->properties.playback_status;
}

const core::Property<media::Player::LoopStatus>& media::PlayerSkeleton::loop_status() const
{
    return *d->properties.loop_status;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::playback_rate() const
{
    return *d->properties.playback_rate;
}

const core::Property<bool>& media::PlayerSkeleton::is_shuffle() const
{
    return *d->properties.is_shuffle;
}

const core::Property<media::Track::MetaData>& media::PlayerSkeleton::meta_data_for_current_track() const
{
    return *d->properties.meta_data_for_current_track;
}

const core::Property<media::Player::Volume>& media::PlayerSkeleton::volume() const
{
    return *d->properties.volume;
}

const core::Property<uint64_t>& media::PlayerSkeleton::position() const
{
    return *d->properties.position;
}

const core::Property<uint64_t>& media::PlayerSkeleton::duration() const
{
    return *d->properties.duration;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::minimum_playback_rate() const
{
    return *d->properties.minimum_playback_rate;
}

const core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::maximum_playback_rate() const
{
    return *d->properties.maximum_playback_rate;
}

core::Property<media::Player::LoopStatus>& media::PlayerSkeleton::loop_status()
{
    return *d->properties.loop_status;
}

core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::playback_rate()
{
    return *d->properties.playback_rate;
}

core::Property<bool>& media::PlayerSkeleton::is_shuffle()
{
    return *d->properties.is_shuffle;
}

core::Property<media::Player::Volume>& media::PlayerSkeleton::volume()
{
    return *d->properties.volume;
}

core::Property<uint64_t>& media::PlayerSkeleton::position()
{
    return *d->properties.position;
}

core::Property<uint64_t>& media::PlayerSkeleton::duration()
{
    return *d->properties.duration;
}

core::Property<media::Player::PlaybackStatus>& media::PlayerSkeleton::playback_status()
{
    return *d->properties.playback_status;
}

core::Property<bool>& media::PlayerSkeleton::can_play()
{
    return *d->properties.can_play;
}

core::Property<bool>& media::PlayerSkeleton::can_pause()
{
    return *d->properties.can_pause;
}

core::Property<bool>& media::PlayerSkeleton::can_seek()
{
    return *d->properties.can_seek;
}

core::Property<bool>& media::PlayerSkeleton::can_go_previous()
{
    return *d->properties.can_go_previous;
}

core::Property<bool>& media::PlayerSkeleton::can_go_next()
{
    return *d->properties.can_go_next;
}

core::Property<bool>& media::PlayerSkeleton::is_video_source()
{
    return *d->properties.is_video_source;
}

core::Property<bool>& media::PlayerSkeleton::is_audio_source()
{
    return *d->properties.is_audio_source;
}


core::Property<media::Track::MetaData>& media::PlayerSkeleton::meta_data_for_current_track()
{
    return *d->properties.meta_data_for_current_track;
}

core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::minimum_playback_rate()
{
    return *d->properties.minimum_playback_rate;
}

core::Property<media::Player::PlaybackRate>& media::PlayerSkeleton::maximum_playback_rate()
{
    return *d->properties.maximum_playback_rate;
}

const core::Signal<uint64_t>& media::PlayerSkeleton::seeked_to() const
{
    return d->signals.seeked_to;
}

core::Signal<uint64_t>& media::PlayerSkeleton::seeked_to()
{
    return d->signals.seeked_to;
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
