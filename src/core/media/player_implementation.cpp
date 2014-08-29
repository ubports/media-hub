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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */

#include "player_implementation.h"
#include "util/timeout.h"

#include <unistd.h>

#include "engine.h"
#include "track_list_implementation.h"

#include "powerd_service.h"
#include "unity_screen_service.h"
#include "gstreamer/engine.h"

#include <exception>
#include <iostream>
#include <mutex>

#define UNUSED __attribute__((unused))

namespace media = core::ubuntu::media;
namespace dbus = core::dbus;

using namespace std;

struct media::PlayerImplementation::Private
{
    enum class wakelock_clear_t
    {
        WAKELOCK_CLEAR_INACTIVE,
        WAKELOCK_CLEAR_DISPLAY,
        WAKELOCK_CLEAR_SYSTEM,
        WAKELOCK_CLEAR_INVALID
    };    

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
          disp_cookie(-1),
          system_wakelock_count(0),
          display_wakelock_count(0),
          previous_state(Engine::State::stopped),
          key(key)
    {
        auto bus = std::shared_ptr<dbus::Bus>(new dbus::Bus(core::dbus::WellKnownBus::system));
        bus->install_executor(dbus::asio::make_executor(bus));

        auto stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::Powerd>::interface_name());
        powerd_session = stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/powerd"));

        auto uscreen_stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::UScreen>::interface_name());
        uscreen_session = uscreen_stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/Unity/Screen"));

        /*
         * Wakelock state logic:
         *
         * PLAYING->READY: delay 4 seconds and try to clear current wakelock type
         * PLAYING->PAUSED or PLAYING->STOPPED: delay 4 seconds and try to clear current wakelock type
         * READY->PAUSED: request a new wakelock (system or display)
         * PLAYING->PAUSED: delay 4 seconds and try to clear current wakelock type
         */
        engine->state().changed().connect(
                    [parent, this](const Engine::State& state)
        {
            switch(state)
            {
            case Engine::State::ready:
            {
                parent->playback_status().set(media::Player::ready);
                if (previous_state == Engine::State::playing)
                {
                    wakelock_timeout.reset(new timeout(4000, true, std::bind(&Private::clear_wakelock,
                                    this, std::placeholders::_1), current_wakelock_type()));
                }
                break;
            }
            case Engine::State::playing:
            {
                // We update the track meta data prior to updating the playback status.
                // Some MPRIS clients expect this order of events.
                parent->meta_data_for_current_track().set(std::get<1>(engine->track_meta_data().get()));
                // And update our playback status.
                parent->playback_status().set(media::Player::playing);
                if (previous_state == Engine::State::stopped || previous_state == Engine::State::paused)
                {
                    request_power_state();
                }
                break;
            }
            case Engine::State::stopped:
            {
                parent->playback_status().set(media::Player::stopped);
                break;
            }
            case Engine::State::paused:
            {
                parent->playback_status().set(media::Player::paused);
                if (previous_state == Engine::State::ready)
                {
                    request_power_state();
                }
                else if (previous_state == Engine::State::playing)
                {
                    wakelock_timeout.reset(new timeout(4000, true, std::bind(&Private::clear_wakelock,
                                    this, std::placeholders::_1), current_wakelock_type()));
                }
                break;
            }
            default:
                break;
            };

            // Keep track of the previous Engine playback state:
            previous_state = state;
        });
    }

    ~Private()
    {
        // Make sure that we don't hold on to the wakelocks if media-hub-server
        // ever gets restarted manually or automatically
        clear_wakelocks();
    }

    void request_power_state()
    {
        try
        {
            if (parent->is_video_source())
            {
                if (display_wakelock_count == 0)
                {
                    auto result = uscreen_session->invoke_method_synchronously<core::UScreen::keepDisplayOn, int>();
                    if (result.is_error())
                        throw std::runtime_error(result.error().print());
                    disp_cookie = result.value();
                    cout << "Requested new display wakelock" << endl;
                }

                {
                    // Keep track of how many display wakelocks have been requested
                    std::lock_guard<std::mutex> lock(wakelock_mutex);
                    ++display_wakelock_count;
                }
            }
            else
            {
                if (system_wakelock_count == 0)
                {
                    auto result = powerd_session->invoke_method_synchronously<core::Powerd::requestSysState, std::string>(sys_lock_name, static_cast<int>(1));
                    if (result.is_error())
                        throw std::runtime_error(result.error().print());
                    sys_cookie = result.value();
                    cout << "Requested new system wakelock" << endl;
                }

                {
                    // Keep track of how many system wakelocks have been requested
                    std::lock_guard<std::mutex> lock(wakelock_mutex);
                    ++system_wakelock_count;
                }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << "Warning: failed to request power state: ";
            std::cerr << e.what() << std::endl;
        }
    }

    void clear_wakelock(const wakelock_clear_t &wakelock)
    {
        cout << __PRETTY_FUNCTION__ << endl;
        try
        {
            switch (wakelock)
            {
                case wakelock_clear_t::WAKELOCK_CLEAR_INACTIVE:
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM:
                    {
                        std::lock_guard<std::mutex> lock(wakelock_mutex);
                        --system_wakelock_count;
                    }
                    // Only actually clear the system wakelock once the count reaches zero
                    if (system_wakelock_count == 0)
                    {
                        cout << "Clearing system wakelock" << endl;
                        powerd_session->invoke_method_synchronously<core::Powerd::clearSysState, void>(sys_cookie);
                        sys_cookie.clear();
                    }
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY:
                    {
                        std::lock_guard<std::mutex> lock(wakelock_mutex);
                        --display_wakelock_count;
                    }
                    // Only actually clear the display wakelock once the count reaches zero
                    if (display_wakelock_count == 0)
                    {
                        cout << "Clearing display wakelock" << endl;
                        uscreen_session->invoke_method_synchronously<core::UScreen::removeDisplayOnRequest, void>(disp_cookie);
                        disp_cookie = -1;
                    }
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_INVALID:
                default:
                    cerr << "Can't clear invalid wakelock type" << endl;
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << "Warning: failed to clear power state: ";
            std::cerr << e.what() << std::endl;
        }
    }

    wakelock_clear_t current_wakelock_type() const
    {
        return (parent->is_video_source()) ?
            wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY : wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM;
    }

    void clear_wakelocks()
    {
        // Clear both types of wakelocks (display and system)
        if (system_wakelock_count > 0)
        {
            {
                std::lock_guard<std::mutex> lock(wakelock_mutex);
                system_wakelock_count = 1;
            }
            clear_wakelock(wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM);
        }
        if (display_wakelock_count > 0)
        {
            {
                std::lock_guard<std::mutex> lock(wakelock_mutex);
                display_wakelock_count = 1;
            }
            clear_wakelock(wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY);
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
    std::string sys_cookie;
    std::mutex wakelock_mutex;
    uint8_t system_wakelock_count;
    uint8_t display_wakelock_count;
    std::unique_ptr<timeout> wakelock_timeout;
    Engine::State previous_state;
    PlayerImplementation::PlayerKey key;
};

media::PlayerImplementation::PlayerImplementation(
        const std::string& identity,
        const std::shared_ptr<core::dbus::Bus>& bus,
        const std::shared_ptr<core::dbus::Object>& session,
        const std::shared_ptr<Service>& service,
        PlayerKey key)
    : media::PlayerSkeleton
      {
          media::PlayerSkeleton::Configuration
          {
              bus,
              session,
              media::PlayerSkeleton::always_missing_cover_art_resolver(),
              identity
          }
      },
      d(new Private(
            this,
            session->path(),
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

    d->engine->client_disconnected_signal().connect([this]()
    {
        // If the client disconnects, make sure both wakelock types
        // are cleared
        d->clear_wakelocks();
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
    std::cout << __PRETTY_FUNCTION__ << std::endl;
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
