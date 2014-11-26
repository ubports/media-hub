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

#include <core/dbus/macros.h>
#include <core/dbus/object.h>
#include <core/dbus/property.h>

namespace media = core::ubuntu::media;

namespace com { namespace canonical { namespace indicator { namespace power {
struct Battery
{
    static std::string& name()
    {
        static std::string s = "com.canonical.indicator.power.Battery";
        return s;
    }

    static const core::dbus::types::ObjectPath& path()
    {
        static core::dbus::types::ObjectPath p{"/com/canonical/indicator/power/Battery"};
        return p;
    }

    // Possible values: "ok", "low", "very_low", "critical"
    DBUS_CPP_READABLE_PROPERTY_DEF(PowerLevel, Battery, std::string)
    DBUS_CPP_READABLE_PROPERTY_DEF(IsWarning, Battery, bool)
}; // IndicatorPower
}}}}

namespace
{
namespace impl
{
struct BatteryObserver : public media::power::BatteryObserver
{
    static core::ubuntu::media::power::Level power_level_from_string(const std::string& s)
    {
        static const std::map<std::string, core::ubuntu::media::power::Level> lut =
        {
            {"ok", core::ubuntu::media::power::Level::ok},
            {"low", core::ubuntu::media::power::Level::low},
            {"very_low", core::ubuntu::media::power::Level::very_low},
            {"critical", core::ubuntu::media::power::Level::critical}
        };

        if (lut.count(s) == 0)
            return core::ubuntu::media::power::Level::unknown;

        return lut.at(s);
    }

    BatteryObserver(const core::dbus::Object::Ptr& object)
        : object{object},
          properties
          {
              core::Property<core::ubuntu::media::power::Level>{core::ubuntu::media::power::Level::unknown},
              object->get_property<com::canonical::indicator::power::Battery::PowerLevel>(),
              object->get_property<com::canonical::indicator::power::Battery::IsWarning>(),
          },
          connections
          {
              properties.power_level->changed().connect([this](const std::string& value)
              {
                  properties.typed_power_level = BatteryObserver::power_level_from_string(value);
              })
          }
    {
    }

    const core::Property<core::ubuntu::media::power::Level>& level() const override
    {
        return properties.typed_power_level;
    }

    const core::Property<bool>& is_warning_active() const override
    {
        return *properties.is_warning;
    }

    // The object representing the remote indicator instance.
    core::dbus::Object::Ptr object;
    // All properties go here.
    struct
    {
        // We have to translate from the raw strings coming in via the bus to
        // the strongly typed enumeration exposed via the interface.
        core::Property<core::ubuntu::media::power::Level> typed_power_level;
        std::shared_ptr<core::dbus::Property<com::canonical::indicator::power::Battery::PowerLevel>> power_level;

        std::shared_ptr<core::dbus::Property<com::canonical::indicator::power::Battery::IsWarning>> is_warning;
    } properties;
    // Our event connections
    struct
    {
        core::ScopedConnection power_level;
    } connections;

};
}
}

media::power::BatteryObserver::Ptr media::power::make_platform_default_battery_observer(media::helper::ExternalServices& es)
{
    auto service = core::dbus::Service::use_service<com::canonical::indicator::power::Battery>(es.session);
    auto object = service->object_for_path(com::canonical::indicator::power::Battery::path());

    return std::make_shared<impl::BatteryObserver>(object);
}

