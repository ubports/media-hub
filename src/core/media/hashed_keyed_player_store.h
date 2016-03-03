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

#ifndef CORE_UBUNTU_MEDIA_HASHED_KEYED_PLAYER_STORE_H_
#define CORE_UBUNTU_MEDIA_HASHED_KEYED_PLAYER_STORE_H_

#include <core/media/keyed_player_store.h>

#include <mutex>
#include <unordered_map>

namespace core
{
namespace ubuntu
{
namespace media
{
// Implements KeyedPlayerStore using a std::unordered_map.
class HashedKeyedPlayerStore : public KeyedPlayerStore
{
public:
    HashedKeyedPlayerStore();
    // We keep track of the "current" player, that is, the one
    // that has been created most recently, or has been explicitly foregrounded, or has been enabled for
    // background playback. We provide a getable/observable access to that designated instance.
    const core::Property<std::shared_ptr<media::Player>>& current_player() const override;

    // We keep track of all known player sessions here and render them accessible via
    // the key. All of these functions are thread-safe but not reentrant.
    // Returns true iff a player is known for the given key.
    bool has_player_for_key(const Player::PlayerKey& key) const override;

    // Returns the player for the given key or throws std::out_of_range if no player is known
    // for the given key.
    // Throws std::out_of_range if no player is known for the key.
    std::shared_ptr<Player> player_for_key(const Player::PlayerKey& key) const override;

    // Enumerates all known players and invokes the given enumerator for each
    // (key, player) pair.
    void enumerate_players(const PlayerEnumerator& enumerator) const override;

    // Adds the given player with the given key.
    void add_player_for_key(const Player::PlayerKey& key, const std::shared_ptr<Player>& player) override;

    // Removes the player for the given key, and unsets it if it is the current one.
    void remove_player_for_key(const Player::PlayerKey& key) override;

    // Returns the number of players in the store.
    size_t number_of_players() const;

    // Makes the player known under the given key current.
    // Throws std::out_of_range if no player is known for the key.
    void set_current_player_for_key(const Player::PlayerKey& key) override;

private:
    core::Property<std::shared_ptr<Player>> prop_current_player;
    mutable std::recursive_mutex guard;
    std::unordered_map<Player::PlayerKey, std::shared_ptr<Player>> map;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_KEYED_PLAYER_STORE_H_
