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

#ifndef CORE_UBUNTU_MEDIA_CLIENT_DEATH_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_CLIENT_DEATH_OBSERVER_H_

#include <core/media/player.h>

#include <core/signal.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
// Models functionality to be notified whenever a client
// of the service goes away, and thus allows us to clean
// up in that case.
struct ClientDeathObserver
{
    // To save us some typing.
    typedef std::shared_ptr<ClientDeathObserver> Ptr;

    ClientDeathObserver() = default;
    ClientDeathObserver(const ClientDeathObserver&) = delete;
    virtual ~ClientDeathObserver() = default;

    ClientDeathObserver& operator=(const ClientDeathObserver&) = delete;

    // Registers the client with the given key for death notifications.
    virtual void register_for_death_notifications_with_key(const Player::PlayerKey&) = 0;

    // Emitted whenever a client dies, reporting the key under which the
    // respective client was known.
    virtual const core::Signal<Player::PlayerKey>& on_client_with_key_died() const = 0;
};

// Accesses the default client death observer implementation for the platform.
ClientDeathObserver::Ptr platform_default_client_death_observer();
}
}
}

#endif // CORE_UBUNTU_MEDIA_CLIENT_DEATH_OBSERVER_H_
