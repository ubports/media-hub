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

#include <map>
#include <memory>
#include <thread>

namespace media = core::ubuntu::media;

using namespace std;

struct media::ServiceImplementation::Private
{
    typedef map<media::Player::PlayerKey, std::shared_ptr<media::Player>> player_map_t;

    Private()
        : key_(0)
    {
        bus = std::shared_ptr<dbus::Bus>(new dbus::Bus(core::dbus::WellKnownBus::session));
        bus->install_executor(dbus::asio::make_executor(bus));
        worker = std::thread([this]()
        {
            bus->run();
        });

        // Connect the property change signal that will allow media-hub to take appropriate action
        // when the battery level reaches critical
        auto stub_service = dbus::Service::use_service(bus, "com.canonical.indicator.power");
        indicator_power_session = stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/indicator/power/Battery"));
        power_level = indicator_power_session->get_property<core::IndicatorPower::PowerLevel>();
        power_level->changed().connect([this](const core::IndicatorPower::PowerLevel::ValueType &level)
        {
            // When the battery level hits 2%, pause all multimedia sessions
            if (level == "critical")
                pause_all_multimedia_sessions();
        });
    }

    ~Private()
    {
        bus->stop();

        if (worker.joinable())
            worker.join();
    }

    void track_player(const std::shared_ptr<media::Player>& player)
    {
        player_map.insert(
                std::pair<media::Player::PlayerKey,
                std::shared_ptr<media::Player>>(key_, player));

        ++key_;
    }

    inline media::Player::PlayerKey key() const
    {
        return key_;
    }

    void pause_other_sessions(media::Player::PlayerKey key)
    {
        auto player_it = player_map.find(key);
        if (player_it != player_map.end())
        {
            auto &current_player = (*player_it).second;
            for (auto& player_pair : player_map)
            {
                // Only pause a Player if all of the following criteria are met:
                // 1) currently playing
                // 2) not the same player as the one passed in my key
                // 3) new Player has an audio stream role set to multimedia
                // 4) has an audio stream role set to multimedia
                if (player_pair.second->playback_status() == Player::playing
                        && player_pair.first != key
                        && current_player->audio_stream_role() == media::Player::multimedia
                        && player_pair.second->audio_stream_role() == media::Player::multimedia)
                {
                    cout << "Pausing Player with key: " << player_pair.first << endl;
                    player_pair.second->pause();
                }
            }
        }
        else
            cerr << "Could not find Player by key: " << key << endl;
    }

    // Pauses all multimedia audio stream role type Players
    void pause_all_multimedia_sessions()
    {
        for (auto& player_pair : player_map)
        {
            if (player_pair.second->playback_status() == Player::playing
                    && player_pair.second->audio_stream_role() == media::Player::multimedia)
                player_pair.second->pause();
        }
    }

    // Used for Player instance management
    player_map_t player_map;
    media::Player::PlayerKey key_;
    std::thread worker;
    dbus::Bus::Ptr bus;
    std::shared_ptr<dbus::Object> indicator_power_session;
    std::shared_ptr<core::dbus::Property<core::IndicatorPower::PowerLevel>> power_level;

};

media::ServiceImplementation::ServiceImplementation() : d(new Private())
{
    cout << __PRETTY_FUNCTION__ << endl;
}

media::ServiceImplementation::~ServiceImplementation()
{
}

std::shared_ptr<media::Player> media::ServiceImplementation::create_session(
        const media::Player::Configuration& conf)
{
    std::shared_ptr<media::Player> player = std::make_shared<media::PlayerImplementation>(
            conf.object_path, shared_from_this(), d->key());
    d->track_player(player);
    return player;
}

void media::ServiceImplementation::pause_other_sessions(media::Player::PlayerKey key)
{
    d->pause_other_sessions(key);
}
