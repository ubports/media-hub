/*
 * Copyright © 2013 Canonical Ltd.
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
        std::cout << "Installing power level signal connection" << std::endl;
        auto stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::IndicatorPower>::interface_name());
        indicator_power_session = stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/indicator/power/Battery"));
        power_level = indicator_power_session->get_property<core::IndicatorPower::PowerLevel>();
        power_level->changed().connect([](const core::IndicatorPower::PowerLevel::ValueType &level)
        {
            std::cout << "Power level property changed: " << level << std::endl;
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
        cout << __PRETTY_FUNCTION__ << endl;
        cout << "key: " << key_ << endl;
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
        cout << __PRETTY_FUNCTION__ << endl;
        cout << "key: " << key << endl;

        // TODO: Add a field to Player that is the type of player so that certain
        // types of playback aren't paused below. E.g. a camera click sound shouldn't
        // pause, nor should it pause background music sessions
        for (auto& player_pair : player_map)
        {
            if (player_pair.second->playback_status() == Player::playing
                    && player_pair.first != key)
            {
                cout << "Pausing player with key: " << player_pair.first << endl;
                player_pair.second->pause();
            }
        }
    }

    // Used for Player instance management
    std::map<media::Player::PlayerKey, std::shared_ptr<media::Player>> player_map;
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
