/*
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

#include "player_implementation.h"

#include <unistd.h>

#include "engine.h"
#include "track_list_implementation.h"

#include "powerd_service.h"
#include "unity_screen_service.h"
#include "gstreamer/engine.h"

#include <exception>
#include <iostream>

#define UNUSED __attribute__((unused))

namespace media = core::ubuntu::media;
namespace dbus = core::dbus;

using namespace std;

struct media::PlayerImplementation::Private
{
    Private(PlayerImplementation* parent,
            const dbus::types::ObjectPath& session_path,
            const std::shared_ptr<media::Service>& service,
            PlayerImplementation::PlayerKey key)
        : parent(parent),
          service(service),
          engine(std::make_shared<gstreamer::Engine>()),
          session_path(session_path),
          track_list(
              new media::TrackListImplementation(
                  session_path.as_string() + "/TrackList",
                  engine->meta_data_extractor())),
          sys_lock_name("media-hub-music-playback"),
          active_display_on_request(false),
          key(key)
    {
        auto bus = std::shared_ptr<dbus::Bus>(new dbus::Bus(core::dbus::WellKnownBus::system));
        bus->install_executor(dbus::asio::make_executor(bus));

        auto stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::Powerd>::interface_name());
        powerd_session = stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/powerd"));

        auto uscreen_stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::UScreen>::interface_name());
        uscreen_session = uscreen_stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/Unity/Screen"));

        engine->state().changed().connect(
                    [parent, this](const Engine::State& state)
        {
            switch(state)
            {
            case Engine::State::ready:
            {
                parent->playback_status().set(media::Player::ready);
                clear_power_state();
                break;
            }
            case Engine::State::playing:
            {
                parent->playback_status().set(media::Player::playing);
                request_power_state();
                break;
            }
            case Engine::State::stopped:
            {
                parent->playback_status().set(media::Player::stopped);
                clear_power_state();
                break;
            }
            case Engine::State::paused:
            {
                parent->playback_status().set(media::Player::paused);
                clear_power_state();
                break;
            }
            default:
                break;
            };
        });

    }

    void request_power_state()
    {
        try
        {
            if (parent->is_video_source())
            {
                if (!active_display_on_request)
                {
                    auto result = uscreen_session->invoke_method_synchronously<core::UScreen::keepDisplayOn, int>();
                    if (result.is_error())
                        throw std::runtime_error(result.error().print());

                    disp_cookie = result.value();
                    active_display_on_request = true;
                }
            }
            else
            {
                if (sys_cookie.empty())
                {
                    auto result = powerd_session->invoke_method_synchronously<core::Powerd::requestSysState, std::string>(sys_lock_name, static_cast<int>(1));
                    if (result.is_error())
                        throw std::runtime_error(result.error().print());

                    sys_cookie = result.value();
                }
            }
        }
        catch(std::exception& e)
        {
            std::cerr << "Warning: failed to request power state: ";
            std::cerr << e.what() << std::endl;
        }
    }

    void clear_power_state()
    {
        try
        {
            if (parent->is_video_source())
            {
                if (active_display_on_request)
                {
                    uscreen_session->invoke_method_synchronously<core::UScreen::removeDisplayOnRequest, void>(disp_cookie);
                    active_display_on_request = false;
                }
            }
            else
            {
                if (!sys_cookie.empty())
                {
                    powerd_session->invoke_method_synchronously<core::Powerd::clearSysState, void>(sys_cookie);
                    sys_cookie.clear();
                }
            }
        }
        catch(std::exception& e)
        {
            std::cerr << "Warning: failed to clear power state: ";
            std::cerr << e.what() << std::endl;
        }
    }

    PlayerImplementation* parent;
    std::shared_ptr<Service> service;
    std::shared_ptr<Engine> engine;
    dbus::types::ObjectPath session_path;
    std::shared_ptr<TrackListImplementation> track_list;
    std::shared_ptr<dbus::Object> powerd_session;
    std::shared_ptr<dbus::Object> uscreen_session;
    std::string sys_lock_name;
    int disp_cookie;
    bool active_display_on_request;
    std::string sys_cookie;
    PlayerImplementation::PlayerKey key;
};

media::PlayerImplementation::PlayerImplementation(
        const dbus::types::ObjectPath& session_path,
        const std::shared_ptr<Service>& service,
        PlayerKey key)
    : media::PlayerSkeleton(session_path),
      d(new Private(
            this,
            session_path,
            service,
            key))
{
    // Initializing default values for properties
    can_play().set(true);
    can_pause().set(true);
    can_seek().set(true);
    can_go_previous().set(true);
    can_go_next().set(true);
    is_video_source().set(false);
    is_audio_source().set(false);
    is_shuffle().set(true);
    playback_rate().set(1.f);
    playback_status().set(Player::PlaybackStatus::null);
    loop_status().set(Player::LoopStatus::none);
    position().set(0);
    duration().set(0);

    // Make sure that the Position property gets updated from the Engine
    // every time the client requests position
    std::function<uint64_t()> position_getter = [this]()
    {
        return d->engine->position().get();
    };
    position().install(position_getter);

    // Make sure that the Duration property gets updated from the Engine
    // every time the client requests duration
    std::function<uint64_t()> duration_getter = [this]()
    {
        return d->engine->duration().get();
    };
    duration().install(duration_getter);

    std::function<bool()> video_type_getter = [this]()
    {
        return d->engine->is_video_source().get();
    };
    is_video_source().install(video_type_getter);

    std::function<bool()> audio_type_getter = [this]()
    {
        return d->engine->is_audio_source().get();
    };
    is_audio_source().install(audio_type_getter);

    d->engine->about_to_finish_signal().connect([this]()
    {
        if (d->track_list->has_next())
        {
            Track::UriType uri = d->track_list->query_uri_for_track(d->track_list->next());
            if (!uri.empty())
                d->parent->open_uri(uri);
        }
    });

    d->engine->seeked_to_signal().connect([this](uint64_t value)
    {
        seeked_to()(value);
    });

    d->engine->end_of_stream_signal().connect([this]()
    {
        end_of_stream()();
    });

    d->engine->playback_status_changed_signal().connect([this](const Player::PlaybackStatus& status)
    {
        playback_status_changed()(status);
    });
}

media::PlayerImplementation::~PlayerImplementation()
{
}

std::shared_ptr<media::TrackList> media::PlayerImplementation::track_list()
{
    return d->track_list;
}

// TODO: Convert this to be a property instead of sync call
media::Player::PlayerKey media::PlayerImplementation::key() const
{
    return d->key;
}

bool media::PlayerImplementation::open_uri(const Track::UriType& uri)
{
    return d->engine->open_resource_for_uri(uri);
}

bool media::PlayerImplementation::open_uri(const Track::UriType& uri, const Track::UriType& cookies,
        const std::string& user_agent)
{
    // TODO: Implement this for proper Oxide media playback support
    (void)uri;
    (void)cookies;
    (void)user_agent;
    return true;
}

void media::PlayerImplementation::create_video_sink(uint32_t texture_id)
{
    d->engine->create_video_sink(texture_id);
}

media::Player::GLConsumerWrapperHybris media::PlayerImplementation::gl_consumer() const
{
    // This method is client-side only and is simply a no-op for the service side
    return NULL;
}

void media::PlayerImplementation::next()
{
}

void media::PlayerImplementation::previous()
{
}

void media::PlayerImplementation::play()
{
    d->engine->play();
}

void media::PlayerImplementation::pause()
{
    d->engine->pause();
}

void media::PlayerImplementation::stop()
{
    d->engine->stop();
}

void media::PlayerImplementation::set_frame_available_callback(
    UNUSED FrameAvailableCb cb, UNUSED void *context)
{
    // This method is client-side only and is simply a no-op for the service side
}

void media::PlayerImplementation::set_playback_complete_callback(
    UNUSED PlaybackCompleteCb cb, UNUSED void *context)
{
    // This method is client-side only and is simply a no-op for the service side
}

void media::PlayerImplementation::seek_to(const std::chrono::microseconds& ms)
{
    d->engine->seek_to(ms);
}
