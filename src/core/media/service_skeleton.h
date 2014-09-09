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
 */

#ifndef CORE_UBUNTU_MEDIA_SERVICE_SKELETON_H_
#define CORE_UBUNTU_MEDIA_SERVICE_SKELETON_H_

#include <core/media/service.h>

#include "cover_art_resolver.h"
#include "service_traits.h"

#include <core/dbus/skeleton.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class ServiceSkeleton : public core::dbus::Skeleton<core::ubuntu::media::Service>
{
public:
    // Functor for enumerating all known (key, player) pairs.
    typedef std::function
    <
        void(
            // The key of the player.
            const core::ubuntu::media::Player::PlayerKey&,
            // The actual player instance.
            const std::shared_ptr<core::ubuntu::media::Player>&
        )
    > PlayerEnumerator;

    ServiceSkeleton(const CoverArtResolver& cover_art_resolver = always_missing_cover_art_resolver());
    ~ServiceSkeleton();

    // We keep track of all known player sessions here and render them accessible via
    // the key. All of these functions are thread-safe but not reentrant.
    // Returns true iff a player is known for the given key.
    bool has_player_for_key(const Player::PlayerKey& key) const;
    // Returns the player for the given key or throws std::out_of_range if no player is known
    // for the given key.
    std::shared_ptr<Player> player_for_key(const Player::PlayerKey& key) const;
    // Enumerates all known players and invokes the given enumerator for each
    // (key, player) pair.
    void enumerate_players(const PlayerEnumerator& enumerator) const;
    // Removes the player for the given key, and unsets it if it is the current one.
    void remove_player_for_key(const Player::PlayerKey& key);
    // Makes the player known under the given key current.
    void set_current_player_for_key(const Player::PlayerKey& key);

    void run();
    void stop();

  private:
    struct Private;
    std::shared_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_SKELETON_H_
