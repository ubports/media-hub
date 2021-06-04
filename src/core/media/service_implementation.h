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

#ifndef CORE_UBUNTU_MEDIA_SERVICE_IMPLEMENTATION_H_
#define CORE_UBUNTU_MEDIA_SERVICE_IMPLEMENTATION_H_

#include "player.h"

#include <QObject>
#include <QScopedPointer>

namespace core
{
namespace ubuntu
{
namespace media
{
class PlayerImplementation;

class ServiceImplementationPrivate;
class ServiceImplementation: public QObject
{
    Q_OBJECT

public:
    ServiceImplementation(QObject *parent = nullptr);
    ~ServiceImplementation();

    Player::PlayerKey currentPlayer() const;

    PlayerImplementation *create_session(const Player::Client &client);
    PlayerImplementation *playerByKey(Player::PlayerKey key) const;
    void pause_other_sessions(Player::PlayerKey key);

Q_SIGNALS:
    void currentPlayerChanged();

private:
    Q_DECLARE_PRIVATE(ServiceImplementation)
    QScopedPointer<ServiceImplementationPrivate> d_ptr;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_IMPLEMENTATION_H_
