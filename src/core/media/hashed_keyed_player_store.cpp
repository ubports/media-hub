/*
 * Copyright © 2014 Canonical Ltd.
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

#include <core/media/hashed_keyed_player_store.h>

namespace media = core::ubuntu::media;

media::HashedKeyedPlayerStore::HashedKeyedPlayerStore()
{
}

const core::Property<std::shared_ptr<media::Player>>& media::HashedKeyedPlayerStore::current_player() const
{
    return prop_current_player;
}

bool media::HashedKeyedPlayerStore::has_player_for_key(const media::Player::PlayerKey& key) const
{
    std::lock_guard<std::recursive_mutex> lg{guard};
    return map.count(key) > 0;
}

std::shared_ptr<media::Player> media::HashedKeyedPlayerStore::player_for_key(const media::Player::PlayerKey& key) const
{
    std::lock_guard<std::recursive_mutex> lg{guard};
    auto it = map.find(key);

    if (it == map.end()) throw std::out_of_range
    {
        "HashedKeyedPlayerStore::player_for_key: No player known for " + std::to_string(key)
    };

    return it->second;
}

void media::HashedKeyedPlayerStore::enumerate_players(const media::KeyedPlayerStore::PlayerEnumerator& enumerator) const
{
    std::lock_guard<std::recursive_mutex> lg{guard};
    for (const auto& pair : map)
        enumerator(pair.first, pair.second);
}

void media::HashedKeyedPlayerStore::add_player_for_key(const media::Player::PlayerKey& key, const std::shared_ptr<media::Player>& player)
{
    std::lock_guard<std::recursive_mutex> lg{guard};
    map[key] = player;
}

void media::HashedKeyedPlayerStore::remove_player_for_key(const media::Player::PlayerKey& key)
{
    std::lock_guard<std::recursive_mutex> lg{guard};
    auto it = map.find(key);
    if (it != map.end())
    {
        if (prop_current_player == it->second)
            prop_current_player = nullptr;

        map.erase(it);
    }
}

size_t media::HashedKeyedPlayerStore::number_of_players() const
{
    std::lock_guard<std::recursive_mutex> lg{guard};
    return map.size();
}

void media::HashedKeyedPlayerStore::set_current_player_for_key(const media::Player::PlayerKey& key)
{
    prop_current_player = player_for_key(key);
}
