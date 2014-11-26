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

#include "audio/output_observer.h"
#include "client_death_observer.h"
#include "player_configuration.h"
#include "player_implementation.h"
#include "power/battery_observer.h"
#include "power/state_controller.h"
#include "recorder_observer.h"
#include "telephony/call_monitor.h"

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
          audio_output_observer(media::audio::make_platform_default_output_observer()),
          call_monitor(media::telephony::make_platform_default_call_monitor())
    {
    }

    void media_recording_state_changed(media::RecordingState state)
    {
        if (state == media::RecordingState::started)
            display_state_lock->request_acquire(media::power::DisplayState::on);
        else if (state == media::RecordingState::stopped)
            display_state_lock->request_release(media::power::DisplayState::off);
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
    media::audio::OutputObserver::Ptr audio_output_observer;

    media::telephony::CallMonitor::Ptr call_monitor;
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

    d->audio_output_observer->external_output_state().changed().connect([this](audio::OutputState state)
    {
        switch (state)
        {
        case audio::OutputState::connected:
            break;
        case audio::OutputState::disconnected:
            pause_all_multimedia_sessions();
            break;
        }
    });

    d->call_monitor->on_call_state_changed().connect([this](media::telephony::CallMonitor::State state)
    {
        switch (state) {
        case media::telephony::CallMonitor::State::OffHook:
            pause_all_multimedia_sessions();
            break;
        case media::telephony::CallMonitor::State::OnHook:
            resume_paused_multimedia_sessions();
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
            remove_player_for_key(key);
        });
    });

    return player;
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
                              player->pause();
                          }
                      });
}

void media::ServiceImplementation::resume_paused_multimedia_sessions()
{
    std::for_each(d->paused_sessions.begin(), d->paused_sessions.end(), [this](const media::Player::PlayerKey& key) {
            player_for_key(key)->play();
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
