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
 *              Ricardo Mendoza <ricardo.mendoza@canonical.com>
 *
 * Note: Some of the PulseAudio code was adapted from telepathy-ofono
 */

#include "service_implementation.h"

#include "client_death_observer.h"
#include "call-monitor/call_monitor.h"
#include "player_configuration.h"
#include "player_implementation.h"
#include "power/battery_observer.h"
#include "power/state_controller.h"
#include "recorder_observer.h"

#include <boost/asio.hpp>

#include <string>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <thread>

#include <pulse/pulseaudio.h>

#include "util/timeout.h"

namespace media = core::ubuntu::media;

using namespace std;

struct media::ServiceImplementation::Private
{
    Private(const ServiceImplementation::Configuration& configuration)
        : configuration(configuration),
          resume_key(std::numeric_limits<std::uint32_t>::max()),
          battery_observer(media::power::make_platform_default_battery_observer(configuration.external_services)),
          power_state_controller(media::power::make_platform_default_state_controller(configuration.external_services)),
          display_state_lock(power_state_controller->display_state_lock()),
          client_death_observer(media::platform_default_client_death_observer()),
          recorder_observer(media::make_platform_default_recorder_observer()),
          pulse_mainloop_api(nullptr),
          pulse_context(nullptr),
          headphones_connected(false),
          a2dp_connected(false),
          primary_idx(-1),
          call_monitor(new CallMonitor)
    {
        // Spawn pulse watchdog
        pulse_mainloop = nullptr;
        pulse_worker = std::move(std::thread([this]()
        {
            std::unique_lock<std::mutex> lk(pulse_mutex);
            pcv.wait(lk,
                [this]{
                    if (pulse_mainloop != nullptr || pulse_context != nullptr)
                    {
                        // We come from instance death, destroy and create.
                        if (pulse_context != nullptr)
                        {
                            pa_threaded_mainloop_lock(pulse_mainloop);
                            pa_operation *o;
                            o = pa_context_drain(pulse_context,
                                [](pa_context *context, void *userdata)
                                {
                                    (void) context;

                                    Private *p = reinterpret_cast<Private*>(userdata);
                                    pa_threaded_mainloop_signal(p->mainloop(), 0);
                                }, this);

                            if (o)
                            {
                                while (pa_operation_get_state(o) == PA_OPERATION_RUNNING)
                                    pa_threaded_mainloop_wait(pulse_mainloop);

                                pa_operation_unref(o);
                            }

                            pa_context_set_state_callback(pulse_context, NULL, NULL);
                            pa_context_set_subscribe_callback(pulse_context, NULL, NULL);
                            pa_context_disconnect(pulse_context);
                            pa_context_unref(pulse_context);
                            pulse_context = nullptr;
                            pa_threaded_mainloop_unlock(pulse_mainloop);
                        }
                    }

                    if (pulse_mainloop == nullptr)
                    {
                        pulse_mainloop = pa_threaded_mainloop_new();

                        if (pa_threaded_mainloop_start(pulse_mainloop) != 0)
                        {
                            std::cerr << "Unable to start pulseaudio mainloop, audio output detection will not function" << std::endl;
                            pa_threaded_mainloop_free(pulse_mainloop);
                            pulse_mainloop = nullptr;
                        }
                    }

                    do {
                        create_pulse_context();
                    } while (pulse_context == nullptr);

                    // Wait for next instance death.
                    return false;
                });
        }));
        
        recorder_observer->recording_state().changed().connect([this](media::RecordingState state)
        {
            media_recording_state_changed(state);
        });
    }

    ~Private()
    {
        release_pulse_context();

        if (pulse_mainloop != nullptr)
        {
            pa_threaded_mainloop_stop(pulse_mainloop);
            pa_threaded_mainloop_free(pulse_mainloop);
            pulse_mainloop = nullptr;
        }

        if (pulse_worker.joinable())
            pulse_worker.join();
    }

    void media_recording_state_changed(media::RecordingState state)
    {
        if (state == media::RecordingState::started)
        {
            display_state_lock->request_acquire(media::power::DisplayState::on);
            pause_playback();
        }
        else if (state == media::RecordingState::stopped)
        {
            display_state_lock->request_release(media::power::DisplayState::off);
        }
    }

    pa_threaded_mainloop *mainloop()
    {
        return pulse_mainloop;
    }

    bool is_port_available(pa_card_port_info **ports, uint32_t n_ports, const char *name)
    {
        bool ret = false;

        if (ports != nullptr && n_ports > 0 && name != nullptr)
        {
            for (uint32_t i=0; i<n_ports; i++)
            {
                if (strstr(ports[i]->name, name) != nullptr && ports[i]->available != PA_PORT_AVAILABLE_NO)
                {
                    ret = true;
                    break;
                }
            }
        }

        return ret;
    }

    void update_wired_output()
    {
        const pa_operation *o = pa_context_get_card_info_by_index(pulse_context, primary_idx,
                [](pa_context *context, const pa_card_info *info, int eol, void *userdata)
                {
                    (void) context;
                    (void) eol;

                    if (info == nullptr || userdata == nullptr)
                        return;

                    Private *p = reinterpret_cast<Private*>(userdata);
                    if (p->is_port_available(info->ports, info->n_ports, "output-wired"))
                    {
                        if (!p->headphones_connected)
                            std::cout << "Wired headphones connected" << std::endl;
                        p->headphones_connected = true;
                    }
                    else if (p->headphones_connected == true)
                    {
                        std::cout << "Wired headphones disconnected" << std::endl;
                        p->headphones_connected = false;
                        p->pause_playback_if_necessary(std::get<0>(p->active_sink));
                    }
                }, this);
        (void) o;
    }

    void pause_playback_if_necessary(int index)
    {
        // Catch uninitialized case (active index == -1)
        if (std::get<0>(active_sink) == -1)
            return;

        if (headphones_connected)
            return;

        // No headphones/fallback on primary sink? Pause.
        if (index == primary_idx)
            pause_playback();
    }

    void set_active_sink(const char *name)
    {
        const pa_operation *o = pa_context_get_sink_info_by_name(pulse_context, name,
                [](pa_context *context, const pa_sink_info *i, int eol, void *userdata)
                {
                    (void) context;

                    if (eol)
                        return;

                    Private *p = reinterpret_cast<Private*>(userdata); 
                    std::tuple<uint32_t, uint32_t, std::string> new_sink(std::make_tuple(i->index, i->card, i->name));

                    printf("pulsesink: active_sink=('%s',%d,%d) -> ('%s',%d,%d)\n",
                        std::get<2>(p->active_sink).c_str(), std::get<0>(p->active_sink),
                        std::get<1>(p->active_sink), i->name, i->index, i->card);

                    p->pause_playback_if_necessary(i->index);
                    p->active_sink = new_sink;
                }, this);

        (void) o;
    }

    void update_active_sink()
    {
        const pa_operation *o = pa_context_get_server_info(pulse_context,
                [](pa_context *context, const pa_server_info *i, void *userdata)
                {
                    (void) context;

                    Private *p = reinterpret_cast<Private*>(userdata);
                    if (i->default_sink_name != std::get<2>(p->active_sink))
                        p->set_active_sink(i->default_sink_name);
                    p->update_wired_output();
                }, this);

        (void) o;
    }

    void create_pulse_context()
    {
        if (pulse_context != nullptr)
            return;

        active_sink = std::make_tuple(-1, -1, "");

        bool keep_going = true, ok = true;

        pulse_mainloop_api = pa_threaded_mainloop_get_api(pulse_mainloop);
        pa_threaded_mainloop_lock(pulse_mainloop);

        pulse_context = pa_context_new(pulse_mainloop_api, "MediaHubPulseContext");
        pa_context_set_state_callback(pulse_context,
                [](pa_context *context, void *userdata)
                {
                    (void) context;
                    Private *p = reinterpret_cast<Private*>(userdata);
                    // Signals the pa_threaded_mainloop_wait below to proceed
                    pa_threaded_mainloop_signal(p->mainloop(), 0);
                }, this);

        if (pulse_context == nullptr)
        {
            std::cerr << "Unable to create new pulseaudio context" << std::endl;
            pa_threaded_mainloop_unlock(pulse_mainloop);
            return;
        }

        pa_context_connect(pulse_context, nullptr, pa_context_flags_t((int) PA_CONTEXT_NOAUTOSPAWN | (int) PA_CONTEXT_NOFAIL), nullptr); 
        pa_threaded_mainloop_wait(pulse_mainloop);

        while (keep_going)
        {
            switch (pa_context_get_state(pulse_context))
            {
                case PA_CONTEXT_CONNECTING: // Wait for service to be available
                case PA_CONTEXT_AUTHORIZING:
                case PA_CONTEXT_SETTING_NAME:
                    break;

                case PA_CONTEXT_READY:
                    std::cout << "Pulseaudio connection established." << std::endl;
                    keep_going = false;
                    break;

                case PA_CONTEXT_FAILED:
                case PA_CONTEXT_TERMINATED:
                    keep_going = false;
                    ok = false;
                    break;

                default:
                    std::cerr << "Pulseaudio connection failure: " << pa_strerror(pa_context_errno(pulse_context));
                    keep_going = false;
                    ok = false;
            }

            if (keep_going)
                pa_threaded_mainloop_wait(pulse_mainloop);
        }

        if (ok)
        {
            pa_context_set_state_callback(pulse_context,
                    [](pa_context *context, void *userdata)
                    {
                        (void) context;
                        (void) userdata;
                        Private *p = reinterpret_cast<Private*>(userdata);
                        std::unique_lock<std::mutex> lk(p->pulse_mutex);
                        switch (pa_context_get_state(context))
                        {
                            case PA_CONTEXT_FAILED:
                            case PA_CONTEXT_TERMINATED:
                                p->pcv.notify_all();
                                break;
                            default:
                                break;
                        }
                    }, this);

            //FIXME: Get index for "sink.primary", the default onboard card on Touch.
            pa_context_get_sink_info_by_name(pulse_context, "sink.primary",
                    [](pa_context *context, const pa_sink_info *i, int eol, void *userdata)
                    {
                        (void) context;

                        if (eol)
                            return;

                        Private *p = reinterpret_cast<Private*>(userdata);
                        p->primary_idx = i->index;
                        p->update_wired_output();
                    }, this);

            update_active_sink();

            pa_context_set_subscribe_callback(pulse_context,
                    [](pa_context *context, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
                    {
                        (void) context;
                        (void) idx;

                        if (userdata == nullptr)
                            return;

                        Private *p = reinterpret_cast<Private*>(userdata);
                        if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK)
                        {
                            p->update_active_sink(); 
                        }
                    }, this);
            pa_context_subscribe(pulse_context, PA_SUBSCRIPTION_MASK_SINK, nullptr, this);
        }
        else
        {
            std::cerr << "Connection to pulseaudio failed or was dropped." << std::endl;
            pa_context_unref(pulse_context);
            pulse_context = nullptr;
        }

        pa_threaded_mainloop_unlock(pulse_mainloop);
    }

    void release_pulse_context()
    {
        if (pulse_context != nullptr)
        {
            pa_threaded_mainloop_lock(pulse_mainloop);
            pa_context_disconnect(pulse_context);
            pa_context_unref(pulse_context);
            pa_threaded_mainloop_unlock(pulse_mainloop);
            pulse_context = nullptr;
        }
    }

    media::ServiceImplementation::Configuration configuration;
    // This holds the key of the multimedia role Player instance that was paused
    // when the battery level reached 10% or 5%
    media::Player::PlayerKey resume_key;    
    media::power::BatteryObserver::Ptr battery_observer;
    media::power::StateController::Ptr power_state_controller;
    media::power::StateController::Lock<media::power::DisplayState>::Ptr display_state_lock;
    media::ClientDeathObserver::Ptr client_death_observer;
    media::RecorderObserver::Ptr recorder_observer;
    // Pulse-specific
    pa_mainloop_api *pulse_mainloop_api;
    pa_threaded_mainloop *pulse_mainloop;
    pa_context *pulse_context;
    std::thread pulse_worker;
    std::mutex pulse_mutex;
    std::condition_variable pcv;
    bool headphones_connected;
    bool a2dp_connected;
    std::tuple<int, int, std::string> active_sink;
    int primary_idx;

    // Gets signaled when both the headphone jack is removed or an A2DP device is
    // disconnected and playback needs pausing. Also gets signaled when recording
    // begins.
    core::Signal<void> pause_playback;
    std::unique_ptr<CallMonitor> call_monitor;
    std::list<media::Player::PlayerKey> paused_sessions;
};

media::ServiceImplementation::ServiceImplementation(const Configuration& configuration) : d(new Private(configuration))
{
    d->battery_observer->level().changed().connect([this](const media::power::Level& level)
    {
        // When the battery level hits 10% or 5%, pause all multimedia sessions.
        // Playback will resume when the user clears the presented notification.
        switch (level)
        {
        case media::power::Level::low:
        case media::power::Level::very_low:
            pause_all_multimedia_sessions();
            break;
        default:
            break;
        }
    });

    d->battery_observer->is_warning_active().changed().connect([this](bool active)
    {
        // If the low battery level notification is no longer being displayed,
        // resume what the user was previously playing
        if (!active)
            resume_multimedia_session();
    });

    d->pause_playback.connect([this]()
    {
        std::cout << "Got pause_playback signal, pausing all multimedia sessions" << std::endl;
        pause_all_multimedia_sessions();
    });

    d->call_monitor->on_change([this](CallMonitor::State state) {
        switch (state) {
        case CallMonitor::OffHook:
            std::cout << "Got call started signal, pausing all multimedia sessions" << std::endl;
            pause_all_multimedia_sessions();
            break;
        case CallMonitor::OnHook:
            std::cout << "Got call ended signal, resuming paused multimedia sessions" << std::endl;
            // Don't auto-resume any paused video playback sessions
            resume_paused_multimedia_sessions(false);
            break;
        }
    });
}

media::ServiceImplementation::~ServiceImplementation()
{
}

std::shared_ptr<media::Player> media::ServiceImplementation::create_session(
        const media::Player::Configuration& conf)
{
    auto player = std::make_shared<media::PlayerImplementation>(media::PlayerImplementation::Configuration
    {
        conf.identity,
        conf.bus,
        conf.session,
        shared_from_this(),
        conf.key,
        d->client_death_observer,
        d->power_state_controller
    });

    auto key = conf.key;
    player->on_client_disconnected().connect([this, key]()
    {
        // Call remove_player_for_key asynchronously otherwise deadlock can occur
        // if called within this dispatcher context.
        // remove_player_for_key can destroy the player instance which in turn
        // destroys the "on_client_disconnected" signal whose destructor will wait
        // until all dispatches are done
        d->configuration.external_services.io_service.post([this, key]()
        {
            if (!has_player_for_key(key))
                return;

            if (player_for_key(key)->lifetime() == Player::Lifetime::normal)
                remove_player_for_key(key);
        });
    });

    return player;
}

std::shared_ptr<media::Player> media::ServiceImplementation::create_fixed_session(const std::string&, const media::Player::Configuration&)
{
  // no impl
  return std::shared_ptr<media::Player>();
}

std::shared_ptr<media::Player> media::ServiceImplementation::resume_session(media::Player::PlayerKey)
{
  // no impl
  return std::shared_ptr<media::Player>();
}

void media::ServiceImplementation::pause_other_sessions(media::Player::PlayerKey key)
{
    if (not has_player_for_key(key))
    {
        cerr << "Could not find Player by key: " << key << endl;
        return;
    }

    auto current_player = player_for_key(key);

    // We immediately make the player known as new current player.
    if (current_player->audio_stream_role() == media::Player::multimedia)
        set_current_player_for_key(key);

    enumerate_players([current_player, key](const media::Player::PlayerKey& other_key, const std::shared_ptr<media::Player>& other_player)
    {
        // Only pause a Player if all of the following criteria are met:
        // 1) currently playing
        // 2) not the same player as the one passed in my key
        // 3) new Player has an audio stream role set to multimedia
        // 4) has an audio stream role set to multimedia
        if (other_player->playback_status() == Player::playing &&
            other_key != key &&
            current_player->audio_stream_role() == media::Player::multimedia &&
            other_player->audio_stream_role() == media::Player::multimedia)
        {
            cout << "Pausing Player with key: " << other_key << endl;
            other_player->pause();
        }
    });
}

void media::ServiceImplementation::pause_all_multimedia_sessions()
{
    enumerate_players([this](const media::Player::PlayerKey& key, const std::shared_ptr<media::Player>& player)
                      {
                          if (player->playback_status() == Player::playing
                              && player->audio_stream_role() == media::Player::multimedia)
                          {
                              d->paused_sessions.push_back(key);
                              std::cout << "Pausing Player with key: " << key << std::endl;
                              player->pause();
                          }
                      });
}

void media::ServiceImplementation::resume_paused_multimedia_sessions(bool resume_video_sessions)
{
    std::for_each(d->paused_sessions.begin(), d->paused_sessions.end(), [this, resume_video_sessions](const media::Player::PlayerKey& key) {
            auto player = player_for_key(key);
            // Only resume video playback if explicitly desired
            if (resume_video_sessions || player->is_audio_source())
                player->play();
            else
                std::cout << "Not auto-resuming video playback session." << std::endl;
        });

    d->paused_sessions.clear();
}

void media::ServiceImplementation::resume_multimedia_session()
{
    if (not has_player_for_key(d->resume_key))
        return;

    auto player = player_for_key(d->resume_key);

    if (player->playback_status() == Player::paused)
    {
        cout << "Resuming playback of Player with key: " << d->resume_key << endl;
        player->play();
        d->resume_key = std::numeric_limits<std::uint32_t>::max();
    }
}
