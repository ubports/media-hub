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

#ifndef CORE_UBUNTU_MEDIA_DBUS_CLIENT_DEATH_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_DBUS_CLIENT_DEATH_OBSERVER_H_

#include <core/media/client_death_observer_p.h>

#include <QDBusServiceWatcher>
#include <QObject>
#include <QVector>

namespace core {
namespace ubuntu {
namespace media {

class DBusClientDeathObserver: public QObject,
                               public ClientDeathObserverPrivate
{
    Q_OBJECT

public:
    DBusClientDeathObserver(ClientDeathObserver *q);
    ~DBusClientDeathObserver();

    // Registers the given client for death notifications.
    void registerForDeathNotifications(const Player::Client &) override;

protected:
    void onServiceDied(const QString &serviceName);

private:
    QVector<Player::Client> m_clients;
    QDBusServiceWatcher m_watcher;
};

}
}
}

#endif // CORE_UBUNTU_MEDIA_DBUS_CLIENT_DEATH_OBSERVER_H_
