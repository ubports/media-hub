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

#ifndef CORE_UBUNTU_MEDIA_KEYED_PLAYER_STORE_H_
#define CORE_UBUNTU_MEDIA_KEYED_PLAYER_STORE_H_

#include <core/media/player.h>

#include <core/property.h>

#include <functional>
#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
// An interface abstracting keyed lookups of known Player instances.
class KeyedPlayerStore
{
public:
    // Save us some typing.
    typedef std::shared_ptr<KeyedPlayerStore> Ptr;
    // Functor for enumerating all known (key, player) pairs.
    typedef std::function
    <
        void(
            // The key of the player.
            const Player::PlayerKey&,
            // The actual player instance.
            const std::shared_ptr<Player>&
        )
    > PlayerEnumerator;
    // We keep track of the "current" player, that is, the one
    // that has been created most recently and provide a getable/observable
    // access to that designated instance.
    virtual const core::Property<std::shared_ptr<Player>>& current_player() const = 0;

    // We keep track of all known player sessions here and render them accessible via
    // the key. All of these functions are thread-safe but not reentrant.
    // Returns true iff a player is known for the given key.
    virtual bool has_player_for_key(const Player::PlayerKey& key) const = 0;
    // Returns the player for the given key or throws std::out_of_range if no player is known
    // for the given key.
    virtual std::shared_ptr<Player> player_for_key(const Player::PlayerKey& key) const = 0;
    // Enumerates all known players and invokes the given enumerator for each
    // (key, player) pair.
    virtual void enumerate_players(const PlayerEnumerator& enumerator) const = 0;
    // Adds the given player with the given key.
    virtual void add_player_for_key(const Player::PlayerKey& key, const std::shared_ptr<Player>& player) = 0;
    // Removes the player for the given key, and unsets it if it is the current one.
    virtual void remove_player_for_key(const Player::PlayerKey& key) = 0;
    // Makes the player known under the given key current.
    virtual void set_current_player_for_key(const Player::PlayerKey& key) = 0;

protected:
    KeyedPlayerStore() = default;
    KeyedPlayerStore(const KeyedPlayerStore&) = delete;
    virtual ~KeyedPlayerStore() = default;
    KeyedPlayerStore& operator=(const KeyedPlayerStore&) = delete;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_KEYED_PLAYER_STORE_H_
