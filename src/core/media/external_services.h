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
#ifndef CORE_UBUNTU_MEDIA_EXTERNAL_SERVICES_H_
#define CORE_UBUNTU_MEDIA_EXTERNAL_SERVICES_H_

#include <core/dbus/bus.h>

#include <core/dbus/asio/executor.h>

#include <boost/asio.hpp>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace helper
{
// A helper struct that bundles:
//   * a dispatcher, i.e., the io_service
//   * access to the system and session bus
//
// In addtion, it allows us to mock out services and
// for acceptance testing purposes.
struct ExternalServices
{
    ExternalServices(const core::dbus::Bus::Ptr& session, const core::dbus::Bus::Ptr& system)
        : keep_alive{io_service},
          session{session},
          system{system}
    {
    }

    ExternalServices()
        : ExternalServices
          {
              core::dbus::Bus::Ptr{new core::dbus::Bus{core::dbus::WellKnownBus::session}},
              core::dbus::Bus::Ptr{new core::dbus::Bus{core::dbus::WellKnownBus::system}}
          }
    {
        session->install_executor(core::dbus::asio::make_executor(session, io_service));
        system->install_executor(core::dbus::asio::make_executor(session, io_service));
    }


    void run()
    {
        io_service.run();
    }

    void stop()
    {
        io_service.stop();
    }

    boost::asio::io_service io_service;
    boost::asio::io_service::work keep_alive;

    core::dbus::Bus::Ptr session;
    core::dbus::Bus::Ptr system;
};
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_EXTERNAL_SERVICES_H_
