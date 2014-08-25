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

#include "player_configuration.h"
#include "player_implementation.h"

#include <map>

namespace media = core::ubuntu::media;

using namespace std;

struct media::ServiceImplementation::Private
{
    Private()
        : key_(0)
    {
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
            conf.bus, conf.session, shared_from_this(), d->key());
    d->track_player(player);
    return player;
}

void media::ServiceImplementation::pause_other_sessions(media::Player::PlayerKey key)
{
    d->pause_other_sessions(key);
}
