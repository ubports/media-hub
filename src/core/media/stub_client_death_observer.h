/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#ifndef CORE_UBUNTU_MEDIA_STUB_CLIENT_DEATH_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_STUB_CLIENT_DEATH_OBSERVER_H_

#include <core/media/client_death_observer_p.h>

namespace core
{
namespace ubuntu
{
namespace media
{
// Models functionality to be notified whenever a client
// of the service goes away, and thus allows us to clean
// up in that case.
// Generic empty implementation.
class StubClientDeathObserver : public ClientDeathObserverPrivate
{
public:
    StubClientDeathObserver(ClientDeathObserver *q);
    ~StubClientDeathObserver();

    void registerForDeathNotificationsWithKey(const Player::PlayerKey &) override;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_STUB_CLIENT_DEATH_OBSERVER_H_
