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
 */

#include "service_implementation.h"

#include "indicator_power_service.h"
#include "player_configuration.h"
#include "player_implementation.h"

#include <boost/asio.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <thread>

#include "util/timeout.h"
#include "unity_screen_service.h"
#include <hybris/media/media_recorder_layer.h>

namespace media = core::ubuntu::media;

using namespace std;

struct media::ServiceImplementation::Private
{
    Private()
        : resume_key(std::numeric_limits<std::uint32_t>::max()),
          keep_alive(io_service),
          disp_cookie(0)
    {
        bus = std::shared_ptr<dbus::Bus>(new dbus::Bus(core::dbus::WellKnownBus::session));
        bus->install_executor(dbus::asio::make_executor(bus, io_service));
        worker = std::move(std::thread([this]()
        {
            bus->run();
        }));

        // Connect the property change signal that will allow media-hub to take appropriate action
        // when the battery level reaches critical
        auto stub_service = dbus::Service::use_service(bus, "com.canonical.indicator.power");
        indicator_power_session = stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/indicator/power/Battery"));
        power_level = indicator_power_session->get_property<core::IndicatorPower::PowerLevel>();
        is_warning = indicator_power_session->get_property<core::IndicatorPower::IsWarning>();

        // Obtain session with Unity.Screen so that we request state when doing recording
        auto bus = std::shared_ptr<dbus::Bus>(new dbus::Bus(core::dbus::WellKnownBus::system));
        bus->install_executor(dbus::asio::make_executor(bus));

        auto uscreen_stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::UScreen>::interface_name());
        uscreen_session = uscreen_stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/Unity/Screen"));

        observer = android_media_recorder_observer_new();
        android_media_recorder_observer_set_cb(observer, &Private::media_recording_started_callback, this);
    }

    ~Private()
    {
        bus->stop();

        if (worker.joinable())
            worker.join();
    }

    void media_recording_started(bool started)
    {
        if (uscreen_session == nullptr)
            return;

        if (started)
        {
            if (disp_cookie > 0)
                return;

            auto result = uscreen_session->invoke_method_synchronously<core::UScreen::keepDisplayOn, int>();
            if (result.is_error())
                throw std::runtime_error(result.error().print());
            disp_cookie = result.value();
        }
        else
        {
            if (disp_cookie != -1)
            {
                timeout(4000, true, [this](){
                    this->uscreen_session->invoke_method_synchronously<core::UScreen::removeDisplayOnRequest, void>(this->disp_cookie);
                    this->disp_cookie = -1;
                });
            }
        }
    }

    static void media_recording_started_callback(bool started, void *context)
    {
        if (context == nullptr)
            return;

        auto p = static_cast<Private*>(context);
        p->media_recording_started(started);
    }

    // This holds the key of the multimedia role Player instance that was paused
    // when the battery level reached 10% or 5%
    media::Player::PlayerKey resume_key;
    std::thread worker;
    dbus::Bus::Ptr bus;
    boost::asio::io_service io_service;
    boost::asio::io_service::work keep_alive;
    std::shared_ptr<dbus::Object> indicator_power_session;
    std::shared_ptr<core::dbus::Property<core::IndicatorPower::PowerLevel>> power_level;
    std::shared_ptr<core::dbus::Property<core::IndicatorPower::IsWarning>> is_warning;
    int disp_cookie;
    std::shared_ptr<dbus::Object> uscreen_session;
    MediaRecorderObserver *observer;
};

media::ServiceImplementation::ServiceImplementation() : d(new Private())
{
    cout << __PRETTY_FUNCTION__ << endl;

    d->power_level->changed().connect([this](const core::IndicatorPower::PowerLevel::ValueType &level)
    {
        // When the battery level hits 10% or 5%, pause all multimedia sessions.
        // Playback will resume when the user clears the presented notification.
        if (level == "low" || level == "very_low")
            pause_all_multimedia_sessions();
    });

    d->is_warning->changed().connect([this](const core::IndicatorPower::IsWarning::ValueType &notifying)
    {
        // If the low battery level notification is no longer being displayed,
        // resume what the user was previously playing
        if (!notifying)
            resume_multimedia_session();
    });
}

media::ServiceImplementation::~ServiceImplementation()
{
}

std::shared_ptr<media::Player> media::ServiceImplementation::create_session(
        const media::Player::Configuration& conf)
{
    auto player = std::make_shared<media::PlayerImplementation>(
            conf.identity, conf.bus, conf.session, shared_from_this(), conf.key);

    auto key = conf.key;
    player->on_client_disconnected().connect([this, key]()
    {
        // Call remove_player_for_key asynchronously otherwise deadlock can occur
        // if called within this dispatcher context.
        // remove_player_for_key can destroy the player instance which in turn
        // destroys the "on_client_disconnected" signal whose destructor will wait
        // until all dispatches are done
        d->io_service.post([this, key]()
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
                              d->resume_key = key;
                              cout << "Will resume playback of Player with key: " << d->resume_key << endl;
                              player->pause();
                          }
                      });
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
