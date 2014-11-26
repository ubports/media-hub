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

#include <core/media/external_services.h>

#include <core/property.h>

#include <memory>

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

// Interface that enables observation of the system power state.
struct BatteryObserver
{
    // To safe us some typing.
    typedef std::shared_ptr<BatteryObserver> Ptr;

    BatteryObserver() = default;
    virtual ~BatteryObserver() = default;

    // A getable/observable property reporting the current power-level
    // of the system.
    virtual const core::Property<Level>& level() const = 0;
    // A getable/observable property indicating whether a power-level
    // warning is currently presented to the user.
    virtual const core::Property<bool>& is_warning_active() const = 0;
};

// Creates a BatteryObserver instance that connects to the platform default
// services to observe battery levels.
core::ubuntu::media::power::BatteryObserver::Ptr make_platform_default_battery_observer(core::ubuntu::media::helper::ExternalServices&);
}
}
}
}
#endif // CORE_UBUNTU_MEDIA_POWER_STATE_OBSERVER_H_
