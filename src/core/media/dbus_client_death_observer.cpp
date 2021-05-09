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

#include "dbus_client_death_observer.h"

#include "core/media/logging.h"

#include <QDBusConnection>
#include <QPair>
#include <QPointer>

namespace media = core::ubuntu::media;

media::DBusClientDeathObserver::DBusClientDeathObserver(ClientDeathObserver *q):
    ClientDeathObserverPrivate(q)
{
    QObject::connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered,
                     this, &DBusClientDeathObserver::onServiceDied);
    m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    m_watcher.setConnection(QDBusConnection::sessionBus());
}

media::DBusClientDeathObserver::~DBusClientDeathObserver()
{
}

void media::DBusClientDeathObserver::onServiceDied(const QString &serviceName)
{
    MH_DEBUG() << "Client died:" << serviceName;
    for (int i = 0; i < m_clients.count(); i++) {
        const media::Player::Client &client = m_clients[i];
        if (serviceName == client.name) {
            notifyClientDeath(client);
            m_clients.removeAt(i);
            break;
        }
    }
}

void media::DBusClientDeathObserver::registerForDeathNotifications(
                                                                            const media::Player::Client &client)
{
    MH_DEBUG() << "Watching for client name" << client.name;
    for (const media::Player::Client &c: m_clients) {
        if (client == c) return; // nothing to do
    }
    m_clients.append(client);

    m_watcher.addWatchedService(client.name);
}
