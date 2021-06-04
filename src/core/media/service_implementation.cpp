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

#include "apparmor/ubuntu.h"
#include "audio/output_observer.h"
#include "client_death_observer.h"
#include "logging.h"
#include "player_implementation.h"
#include "power/battery_observer.h"
#include "power/state_controller.h"
#include "recorder_observer.h"
#include "telephony/call_monitor.h"

#include <string>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <thread>
#include <utility>

#include <pulse/pulseaudio.h>

namespace media = core::ubuntu::media;

using namespace media;

namespace core {
namespace ubuntu {
namespace media {

class ServiceImplementationPrivate
{
    Q_DECLARE_PUBLIC(ServiceImplementation)

public:
    // Create all of the appropriate observers and helper class instances to be
    // passed to the PlayerImplementation
    ServiceImplementationPrivate(ServiceImplementation *q);

    void pause_all_multimedia_sessions(bool resume_play_after_phonecall);
    void resume_paused_multimedia_sessions(bool resume_video_sessions = true);
    void resume_multimedia_session();

    void setCurrentPlayer(Player::PlayerKey key);
    bool pause_other_sessions(Player::PlayerKey key);
    void onPlaybackRequested(Player::PlayerKey key);

private:
    // This holds the key of the multimedia role Player instance that was paused
    // when the battery level reached 10% or 5%
    media::Player::PlayerKey resume_key;
    media::power::BatteryObserver battery_observer;
    media::power::StateController::Ptr power_state_controller;
    media::ClientDeathObserver::Ptr client_death_observer;
    media::RecorderObserver recorder_observer;
    media::audio::OutputObserver audio_output_observer;
    media::audio::OutputState audio_output_state;

    media::telephony::CallMonitor call_monitor;
    // Holds a pair of a Player key denoting what player to resume playback, and a bool
    // for if it should be resumed after a phone call is hung up
    std::list<std::pair<media::Player::PlayerKey, bool>> paused_sessions;
    Player::PlayerKey m_currentPlayer;
    QHash<Player::PlayerKey, PlayerImplementation *> m_players;
    ServiceImplementation *q_ptr;
};

}}} // namespace

ServiceImplementationPrivate::ServiceImplementationPrivate(
        ServiceImplementation *q):
    resume_key(Player::invalidKey),
    power_state_controller(media::power::StateController::instance()),
    client_death_observer(ClientDeathObserver::Ptr::create()),
    audio_output_state(media::audio::OutputState::Speaker),
    m_currentPlayer(Player::invalidKey),
    q_ptr(q)
{
    QObject::connect(&battery_observer,
                     &power::BatteryObserver::levelChanged,
                     q, [this]()
    {
        const bool resume_play_after_phonecall = false;
        // When the battery level hits 10% or 5%, pause all multimedia sessions.
        // Playback will resume when the user clears the presented notification.
        switch (battery_observer.level())
        {
        case media::power::Level::low:
        case media::power::Level::very_low:
            // Whatever player session is currently playing, make sure it is NOT resumed after
            // a phonecall is hung up
            pause_all_multimedia_sessions(resume_play_after_phonecall);
            break;
        default:
            break;
        }
    });

    QObject::connect(&battery_observer,
                     &power::BatteryObserver::isWarningActiveChanged,
                     q, [this]()
    {
        // If the low battery level notification is no longer being displayed,
        // resume what the user was previously playing
        if (!battery_observer.isWarningActive())
            resume_multimedia_session();
    });

    QObject::connect(&audio_output_observer,
                     &audio::OutputObserver::outputStateChanged,
                     q, [this]()
    {
        audio::OutputState state = audio_output_observer.outputState();
        const bool resume_play_after_phonecall = false;
        switch (state)
        {
        case audio::OutputState::Earpiece:
            MH_INFO("AudioOutputObserver reports that output is now Headphones/Headset.");
            break;
        case audio::OutputState::Speaker:
            MH_INFO("AudioOutputObserver reports that output is now Speaker.");
            // Whatever player session is currently playing, make sure it is NOT resumed after
            // a phonecall is hung up
            pause_all_multimedia_sessions(resume_play_after_phonecall);
            break;
        case audio::OutputState::External:
            MH_INFO("AudioOutputObserver reports that output is now External.");
            break;
        }
        audio_output_state = state;
    });

    QObject::connect(&call_monitor,
                     &telephony::CallMonitor::callStateChanged,
                     q, [this]()
    {
        const bool resume_play_after_phonecall = true;
        switch (call_monitor.callState()) {
        case media::telephony::CallMonitor::State::OffHook:
            MH_INFO("Got call started signal, pausing all multimedia sessions");
            // Whatever player session is currently playing, make sure it gets resumed after
            // a phonecall is hung up
            pause_all_multimedia_sessions(resume_play_after_phonecall);
            break;
        case media::telephony::CallMonitor::State::OnHook:
            MH_INFO("Got call ended signal, resuming paused multimedia sessions");
            resume_paused_multimedia_sessions(false);
            break;
        }
    });

    QObject::connect(&recorder_observer,
                     &RecorderObserver::recordingStateChanged,
                     q, [this]()
    {
        RecordingState state = recorder_observer.recordingState();
        if (state == media::RecordingState::started)
        {
            power_state_controller->requestDisplayOn();
            // Whatever player session is currently playing, make sure it is NOT resumed after
            // a phonecall is hung up
            const bool resume_play_after_phonecall = false;
            pause_all_multimedia_sessions(resume_play_after_phonecall);
        }
        else if (state == media::RecordingState::stopped)
        {
            power_state_controller->releaseDisplayOn();
        }
    });
}

void ServiceImplementationPrivate::pause_all_multimedia_sessions(bool resume_play_after_phonecall)
{
    for (auto i = m_players.begin(); i != m_players.end(); i++) {
        const Player::PlayerKey key = i.key();
        PlayerImplementation *player = i.value();

        if (player->playbackStatus() == Player::playing &&
            player->audioStreamRole() == media::Player::multimedia)
        {
            auto paused_player_pair = std::make_pair(key, resume_play_after_phonecall);
            paused_sessions.push_back(paused_player_pair);
            MH_INFO("Pausing Player with key: %d, resuming after phone call? %s", key,
                (resume_play_after_phonecall ? "yes" : "no"));
            player->pause();
        }
    }
}

void ServiceImplementationPrivate::resume_paused_multimedia_sessions(bool resume_video_sessions)
{
    std::for_each(paused_sessions.begin(), paused_sessions.end(),
        [this, resume_video_sessions](const std::pair<media::Player::PlayerKey, bool> &paused_player_pair) {
            const media::Player::PlayerKey key = paused_player_pair.first;
            const bool resume_play_after_phonecall = paused_player_pair.second;
            PlayerImplementation *player = m_players.value(key);
            if (not player) {
                MH_WARNING("Failed to look up Player instance for key %d"
                    ", no valid Player instance for that key value and cannot automatically resume"
                    " paused players. This most likely means that media-hub-server has crashed and"
                    " restarted.", key);
                return;
            }
            // Only resume video playback if explicitly desired
            if ((resume_video_sessions || player->isAudioSource()) && resume_play_after_phonecall)
                player->play();
            else
                MH_INFO("Not auto-resuming video player session or other type of player session.");
        });

    paused_sessions.clear();
}

void ServiceImplementationPrivate::resume_multimedia_session()
{
    PlayerImplementation *player = m_players.value(resume_key);
    if (not player) {
        MH_WARNING("Failed to look up Player instance for key %d"
            ", no valid Player instance for that key value and cannot automatically resume"
            " paused Player. This most likely means that media-hub-server has crashed and"
            " restarted.", resume_key);
        return;
    }

    if (player->playbackStatus() == Player::paused)
    {
        MH_INFO("Resuming playback of Player with key: %d", resume_key);
        player->play();
        resume_key = Player::invalidKey;
    }
}

void ServiceImplementationPrivate::setCurrentPlayer(Player::PlayerKey key)
{
    Q_Q(ServiceImplementation);
    if (key == m_currentPlayer) return;
    m_currentPlayer = key;
    Q_EMIT q->currentPlayerChanged();
}

bool ServiceImplementationPrivate::pause_other_sessions(media::Player::PlayerKey key)
{
    MH_TRACE("");

    PlayerImplementation *current_player = m_players.value(key);
    if (not current_player)
    {
        MH_WARNING("Could not find Player by key: %d", key);
        return false;
    }

    for (auto i = m_players.begin(); i != m_players.end(); i++) {
        const Player::PlayerKey other_key = i.key();
        PlayerImplementation *other_player = i.value();
        // Only pause a Player if all of the following criteria are met:
        // 1) currently playing
        // 2) not the same player as the one passed in my key
        // 3) new Player has an audio stream role set to multimedia
        // 4) has an audio stream role set to multimedia
        if (other_player->playbackStatus() == Player::playing &&
            other_key != key &&
            other_player->client().name != current_player->client().name &&
            current_player->audioStreamRole() == media::Player::multimedia &&
            other_player->audioStreamRole() == media::Player::multimedia)
        {
            MH_INFO("Pausing Player with key: %d", other_key);
            other_player->pause();
        }
    }
    return true;
}

void ServiceImplementationPrivate::onPlaybackRequested(Player::PlayerKey key)
{
    MH_DEBUG("==== Pausing all other multimedia player sessions");
    if (not pause_other_sessions(key)) {
        MH_WARNING("Failed to pause other player sessions");
    }

    MH_DEBUG("==== Updating the current player");
    setCurrentPlayer(key);
}

ServiceImplementation::ServiceImplementation(QObject *parent):
    QObject(parent),
    d_ptr(new ServiceImplementationPrivate(this))
{
}

ServiceImplementation::~ServiceImplementation()
{
}

PlayerImplementation *ServiceImplementation::create_session(
        const Player::Client &client)
{
    Q_D(ServiceImplementation);

    // Create a new Player
    auto player = new PlayerImplementation({
        client,
        d->client_death_observer,
    }, this);

    QObject::connect(player, &PlayerImplementation::clientDisconnected,
                     this, [this, key=client.key]()
    {
        Q_D(ServiceImplementation);
        /* Update the current player if needed */
        PlayerImplementation *player = d->m_players.value(key);
        if (player &&
            player->audioStreamRole() == Player::AudioStreamRole::multimedia &&
            key == currentPlayer())
        {
            MH_DEBUG("==== Resetting current player");
            d->setCurrentPlayer(Player::invalidKey);
        }

        if (player->lifetime() == Player::Lifetime::normal) {
            d->m_players.remove(key);
            player->deleteLater();
        }
    });

    QObject::connect(player, &PlayerImplementation::playbackRequested,
                     this, [d, key=client.key]() {
        d->onPlaybackRequested(key);
    });

    d->m_players[client.key] = player;
    return player;
}

PlayerImplementation *ServiceImplementation::playerByKey(Player::PlayerKey key) const
{
    Q_D(const ServiceImplementation);
    return d->m_players.value(key);
}

void media::ServiceImplementation::pause_other_sessions(media::Player::PlayerKey key)
{
    Q_D(ServiceImplementation);
    MH_TRACE("");
    d->pause_other_sessions(key);
}

Player::PlayerKey ServiceImplementation::currentPlayer() const
{
    Q_D(const ServiceImplementation);
    return d->m_currentPlayer;
}
