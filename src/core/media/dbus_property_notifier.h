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

#ifndef DBUS_PROPERTY_NOTIFIER_H
#define DBUS_PROPERTY_NOTIFIER_H

#include <QObject>
#include <QStringList>

class QDBusConnection;

class DBusPropertyNotifierPrivate;
class DBusPropertyNotifier: public QObject
{
    Q_OBJECT

public:
    DBusPropertyNotifier(const QDBusConnection &connection,
                         const QString &objectPath,
                         QObject *target);
    virtual ~DBusPropertyNotifier();

    void notify(const QStringList &propertyFilter = {});

private:
    Q_DECLARE_PRIVATE(DBusPropertyNotifier)
    QScopedPointer<DBusPropertyNotifierPrivate> d_ptr;
};

#endif // DBUS_PROPERTY_NOTIFIER_H
