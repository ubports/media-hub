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

#include "player.h"

#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>

namespace core
{
namespace ubuntu
{
namespace media
{
// Models functionality to be notified whenever a client
// of the service goes away, and thus allows us to clean
// up in that case.
class ClientDeathObserverPrivate;
class ClientDeathObserver: public QObject
{
    Q_OBJECT

public:
    // To save us some typing.
    typedef QSharedPointer<ClientDeathObserver> Ptr;

    ClientDeathObserver(QObject *parent = nullptr);
    ClientDeathObserver(const ClientDeathObserver&) = delete;
    virtual ~ClientDeathObserver();

    ClientDeathObserver& operator=(const ClientDeathObserver&) = delete;

    // Registers the given client for death notifications.
    void registerForDeathNotifications(const Player::Client &client);

Q_SIGNALS:
    // Emitted whenever a client dies, reporting the key under which the
    // respective client was known.
    void clientDied(const Player::Client &client);

private:
    Q_DECLARE_PRIVATE(ClientDeathObserver);
    QScopedPointer<ClientDeathObserverPrivate> d_ptr;
};

}
}
}

#endif // CORE_UBUNTU_MEDIA_CLIENT_DEATH_OBSERVER_H_
