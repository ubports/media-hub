/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
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
 */

#ifndef CORE_UBUNTU_MEDIA_CLIENT_DEATH_OBSERVER_PRIVATE_H_
#define CORE_UBUNTU_MEDIA_CLIENT_DEATH_OBSERVER_PRIVATE_H_

#include <core/media/client_death_observer.h>

namespace core {
namespace ubuntu {
namespace media {

class ClientDeathObserverPrivate
{
    Q_DECLARE_PUBLIC(ClientDeathObserver)

public:
    ClientDeathObserverPrivate(ClientDeathObserver *q):
        q_ptr(q)
    {
    }
    virtual ~ClientDeathObserverPrivate() = default;

    virtual void registerForDeathNotificationsWithKey(const Player::PlayerKey &key) = 0;

protected:
    void notifyClientDeath(Player::PlayerKey key) {
        Q_Q(ClientDeathObserver);
        Q_EMIT q->clientWithKeyDied(key);
    }

private:
    ClientDeathObserver *q_ptr;
};

}}} // namespace

#endif
