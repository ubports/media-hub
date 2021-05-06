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

#include "apparmor/ubuntu.h"
#include "player.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>
#include <QScopedPointer>

namespace core
{
namespace ubuntu
{
namespace media
{
class ServiceImplementation;

class ServiceSkeletonPrivate;
class ServiceSkeleton: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "core.ubuntu.media.Service")

public:
    // Creation time arguments go here.
    struct Configuration
    {
        QDBusConnection connection;
    };

    ServiceSkeleton(const Configuration &configuration,
                    ServiceImplementation *impl,
                    QObject *parent = nullptr);
    ~ServiceSkeleton();

public Q_SLOTS:
    void CreateSession(QDBusObjectPath &op, QString &uuid);
    void DetachSession(const QString &uuid);
    void ReattachSession(const QString &uuid);
    void DestroySession(const QString &uuid);
    QDBusObjectPath CreateFixedSession(const QString &name);
    QDBusObjectPath ResumeSession(Player::PlayerKey key);
    void PauseOtherSessions(Player::PlayerKey key);

private:
    Q_DECLARE_PRIVATE(ServiceSkeleton)
    QScopedPointer<ServiceSkeletonPrivate> d_ptr;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_SKELETON_H_
