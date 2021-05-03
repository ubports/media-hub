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

#include "dbus_property_notifier.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class DBusPropertyNotifierPrivate: public QObject
{
    Q_OBJECT

public:
    DBusPropertyNotifierPrivate(const QDBusConnection &connection,
                                const QString &objectPath,
                                QObject *target);

    void findInterfaceName(const QMetaObject *mo);
    void connectAllProperties(const QMetaObject *mo);
    void deliverNotifySignal();
    void notify(const QStringList &propertyFilter);

private Q_SLOTS:
    void onChanged(int propertyIndex);
    void onPropertyChanged0() { onChanged(0); }
    void onPropertyChanged1() { onChanged(1); }
    void onPropertyChanged2() { onChanged(2); }
    void onPropertyChanged3() { onChanged(3); }
    void onPropertyChanged4() { onChanged(4); }
    void onPropertyChanged5() { onChanged(5); }
    void onPropertyChanged6() { onChanged(6); }
    void onPropertyChanged7() { onChanged(7); }
    void onPropertyChanged8() { onChanged(8); }
    void onPropertyChanged9() { onChanged(9); }
    void onPropertyChanged10() { onChanged(10); }
    void onPropertyChanged11() { onChanged(11); }
    void onPropertyChanged12() { onChanged(12); }
    void onPropertyChanged13() { onChanged(13); }
    void onPropertyChanged14() { onChanged(14); }
    void onPropertyChanged15() { onChanged(15); }
    void onPropertyChanged16() { onChanged(16); }
    void onPropertyChanged17() { onChanged(17); }
    void onPropertyChanged18() { onChanged(18); }
    void onPropertyChanged19() { onChanged(19); }

private:
    QDBusConnection m_connection;
    QString m_objectPath;
    QString m_interface;
    QObject *m_target;
    QVector<int> m_changedPropertyIndexes;
    QVector<QMetaProperty> m_properties;
    QVariantMap m_lastValues;
};

DBusPropertyNotifierPrivate::DBusPropertyNotifierPrivate(
        const QDBusConnection &connection,
        const QString &objectPath,
        QObject *target):
    m_connection(connection),
    m_objectPath(objectPath),
    m_target(target)
{
    const QMetaObject *mo = target->metaObject();
    findInterfaceName(mo);
    connectAllProperties(mo);
}

void DBusPropertyNotifierPrivate::findInterfaceName(const QMetaObject *mo)
{
    int index = mo->indexOfClassInfo("D-Bus Interface");
    m_interface = QString::fromUtf8(mo->classInfo(index).value());
}

void DBusPropertyNotifierPrivate::connectAllProperties(const QMetaObject *mo)
{
    QStringList properties;
    int nextFreeSlot = 0;
    int indexOfFirstSlot = -1;
    const QMetaObject *ourMo = metaObject();
    for (int i = ourMo->methodOffset(); i < ourMo->methodCount(); i++) {
        if (ourMo->method(i).name().startsWith("onPropertyChanged")) {
            indexOfFirstSlot = i;
            break;
        }
    }

    for (int i = mo->propertyOffset(); i < mo->propertyCount(); i++) {
        const QMetaProperty p = mo->property(i);
        if (!p.hasNotifySignal()) continue;
        const QMetaMethod signalMethod = p.notifySignal();
        const QMetaMethod slotMethod =
            ourMo->method(indexOfFirstSlot + nextFreeSlot);
        if (Q_UNLIKELY(!slotMethod.isValid())) {
            qWarning() << "No more slots available!";
            break;
        }
        QObject::connect(m_target, signalMethod, this, slotMethod);
        m_properties.insert(nextFreeSlot, p);
        nextFreeSlot++;
    }
}

void DBusPropertyNotifierPrivate::onChanged(int propertyIndex)
{
    if (m_changedPropertyIndexes.contains(propertyIndex)) return;

    /* If the list of properties is empty it means we haven't queued the
     * emission of the notify signal.
     */
    if (m_changedPropertyIndexes.isEmpty()) {
        QMetaObject::invokeMethod(this,
                &DBusPropertyNotifierPrivate::deliverNotifySignal,
                Qt::QueuedConnection);
    }
    m_changedPropertyIndexes.append(propertyIndex);
}

void DBusPropertyNotifierPrivate::deliverNotifySignal()
{
    QVariantMap changedProperties;
    for (int propertyIndex: m_changedPropertyIndexes) {
        const QMetaProperty &p = m_properties[propertyIndex];
        const QVariant value = p.read(m_target);
        changedProperties.insert(p.name(), value);
    }

    QDBusMessage msg = QDBusMessage::createSignal(m_objectPath,
            QStringLiteral("org.freedesktop.DBus.Properties"),
            QStringLiteral("PropertiesChanged"));
    msg.setArguments({
        m_interface,
        changedProperties,
        QStringList {},
    });
    m_connection.send(msg);

    m_changedPropertyIndexes.clear();
}

void DBusPropertyNotifierPrivate::notify(const QStringList &propertyFilter)
{
    QVariantMap changedProperties;
    const QMetaObject *mo = m_target->metaObject();
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); i++) {
        const QMetaProperty p = mo->property(i);
        const QString propertyName = p.name();
        if (!propertyFilter.isEmpty() &&
            !propertyFilter.contains(propertyName)) {
            continue;
        }
        const QVariant value = p.read(m_target);
        if (m_lastValues.value(propertyName) != value) {
            changedProperties.insert(propertyName, value);
            m_lastValues.insert(propertyName, value);
        }
    }

    if (!changedProperties.isEmpty()) {
        QDBusMessage msg = QDBusMessage::createSignal(m_objectPath,
                QStringLiteral("org.freedesktop.DBus.Properties"),
                QStringLiteral("PropertiesChanged"));
        msg.setArguments({
            m_interface,
            changedProperties,
            QStringList {},
        });
        m_connection.send(msg);
    }
}

DBusPropertyNotifier::DBusPropertyNotifier(const QDBusConnection &connection,
                                           const QString &objectPath,
                                           QObject *target):
    QObject(target),
    d_ptr(new DBusPropertyNotifierPrivate(connection, objectPath, target))
{
}

DBusPropertyNotifier::~DBusPropertyNotifier() = default;

void DBusPropertyNotifier::notify(const QStringList &propertyFilter)
{
    Q_D(DBusPropertyNotifier);
    d->notify(propertyFilter);
}

#include "dbus_property_notifier.moc"
