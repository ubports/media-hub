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
#ifndef CORE_UBUNTU_MEDIA_POWER_STATE_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_POWER_STATE_OBSERVER_H_

#include <QObject>
#include <QScopedPointer>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace power
{
// Enumerates known power levels.
enum class Level
{
    unknown,
    ok,
    low,
    very_low,
    critical
};

class BatteryObserverPrivate;

// Interface that enables observation of the system power state.
class BatteryObserver: public QObject
{
    Q_OBJECT

public:
    BatteryObserver(QObject *parent = nullptr);
    virtual ~BatteryObserver();

    Level level() const;
    bool isWarningActive() const;

Q_SIGNALS:
    void levelChanged();
    void isWarningActiveChanged();

private:
    Q_DECLARE_PRIVATE(BatteryObserver);
    QScopedPointer<BatteryObserverPrivate> d_ptr;
};

}
}
}
}
#endif // CORE_UBUNTU_MEDIA_POWER_STATE_OBSERVER_H_
