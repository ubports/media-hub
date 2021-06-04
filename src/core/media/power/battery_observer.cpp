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

#include <core/media/power/battery_observer.h>

#include "core/media/logging.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingReply>

using namespace core::ubuntu::media::power;

namespace core {
namespace ubuntu {
namespace media {
namespace power {

class BatteryObserverPrivate: public QObject
{
    Q_OBJECT

    static core::ubuntu::media::power::Level powerLevelFromString(const QString &s)
    {
        using Level = core::ubuntu::media::power::Level;
        if (s == "ok") return Level::ok;
        if (s == "low") return Level::low;
        if (s == "very_low") return Level::very_low;
        if (s == "critical") return Level::critical;
        return Level::unknown;
    }

public:

    BatteryObserverPrivate(BatteryObserver *q):
        QObject(q),
        m_powerLevel(Level::ok),
        m_isWarningActive(false),
        q_ptr(q)
    {
        QDBusConnection connection = QDBusConnection::sessionBus();
        auto iface = new QDBusInterface(QStringLiteral("com.canonical.indicator.power"),
                                        QStringLiteral("/com/canonical/indicator/power/Battery"),
                                        QStringLiteral("org.freedesktop.DBus.Properties"),
                                        connection, this);
        iface->connection().connect(
            iface->service(),
            iface->path(),
            iface->interface(),
            QStringLiteral("PropertiesChanged"),
            this,
            SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

        QDBusPendingCall call =
            iface->asyncCall(QStringLiteral("GetAll"),
                             QStringLiteral("com.canonical.indicator.power.Battery"));
        auto *watcher = new QDBusPendingCallWatcher(call);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         this, [this](QDBusPendingCallWatcher *watcher) {
            QDBusPendingReply<QVariantMap> reply = *watcher;
            updateProperties(reply.value());
            watcher->deleteLater();
        });
    }

    Q_INVOKABLE
    void onPropertiesChanged(const QString &interface,
                             const QVariantMap &changed,
                             const QStringList &invalid)
    {
        Q_UNUSED(interface);
        Q_UNUSED(invalid);

        updateProperties(changed);
    }

    void updateProperties(const QVariantMap &properties)
    {
        Q_Q(BatteryObserver);

        auto i = properties.find(QStringLiteral("PowerLevel"));
        if (i != properties.end()) {
            auto oldPowerLevel = m_powerLevel;
            m_powerLevel = powerLevelFromString(i->toString());
            if (m_powerLevel != oldPowerLevel) {
                Q_EMIT q->levelChanged();
            }
        }

        i = properties.find(QStringLiteral("IsWarning"));
        if (i != properties.end()) {
            bool oldIsWarningActive = m_isWarningActive;
            m_isWarningActive = i->toBool();
            if (m_isWarningActive != oldIsWarningActive) {
                Q_EMIT q->isWarningActiveChanged();
            }
        }
    }

private:
    Q_DECLARE_PUBLIC(BatteryObserver)
    core::ubuntu::media::power::Level m_powerLevel;
    bool m_isWarningActive;
    BatteryObserver *q_ptr;
};

}}}} // namespace

BatteryObserver::BatteryObserver(QObject *parent):
    QObject(parent),
    d_ptr(new BatteryObserverPrivate(this))
{
}

BatteryObserver::~BatteryObserver() = default;

Level BatteryObserver::level() const
{
    Q_D(const BatteryObserver);
    return d->m_powerLevel;
}

bool BatteryObserver::isWarningActive() const
{
    Q_D(const BatteryObserver);
    return d->m_isWarningActive;
}

#include "battery_observer.moc"
