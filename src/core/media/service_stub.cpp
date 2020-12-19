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

#include "service_stub.h"
#include "service_traits.h"

#include "player_stub.h"
#include "the_session_bus.h"

#include "mpris/service.h"

#include "core/media/logger/logger.h"

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

media::ServiceStub::ServiceStub()
    : core::dbus::Stub<media::Service>(the_session_bus()),
      object(
        access_service()->object_for_path(
            dbus::types::ObjectPath(
                dbus::traits::Service<media::Service>::object_path()))
        ),
      daemon(the_session_bus()),
      service_watcher_reg(
          daemon.make_service_watcher(dbus::traits::Service<media::Service>::interface_name(),
                        dbus::DBus::WatchMode::registration)
          ),
      service_watcher_unreg(
          daemon.make_service_watcher(dbus::traits::Service<media::Service>::interface_name(),
                        dbus::DBus::WatchMode::unregistration)
          )
{
    auto bus = the_session_bus();
    worker = std::move(std::thread([bus]()
    {
        bus->run();
    }));

    service_watcher_reg->service_registered().connect(
        [&]()
        {
            MH_DEBUG("media-hub service registered");
            signals.service_reconnected();
        });
    service_watcher_unreg->service_unregistered().connect(
        [&]()
        {
            MH_DEBUG("media-hub service unregistered");
            signals.service_disconnected();
        });
}

media::ServiceStub::~ServiceStub()
{
    auto bus = the_session_bus();
    bus->stop();

    if (worker.joinable())
        worker.join();
}

std::shared_ptr<media::Player> media::ServiceStub::create_session(const media::Player::Configuration&)
{
    const auto op = object->invoke_method_synchronously<mpris::Service::CreateSession,
         std::tuple<dbus::types::ObjectPath, std::string>>();

    if (op.is_error())
        throw std::runtime_error("Problem creating session: " + op.error());

    return std::shared_ptr<media::Player>(new media::PlayerStub
    {
        shared_from_this(),
        access_service(),
        access_service()->object_for_path(std::get<0>(op.value())),
        std::get<1>(op.value())
    });
}

void media::ServiceStub::detach_session(const std::string& uuid,
        const media::Player::Configuration&)
{
    const auto op = object->invoke_method_synchronously<mpris::Service::DetachSession,
         void>(uuid);

    if (op.is_error())
        throw std::runtime_error("Problem detaching session: " + op.error());
}

std::shared_ptr<media::Player> media::ServiceStub::reattach_session(const std::string& uuid,
        const media::Player::Configuration&)
{
    const auto op = object->invoke_method_synchronously<mpris::Service::ReattachSession,
         dbus::types::ObjectPath>(uuid);

    if (op.is_error())
        throw std::runtime_error("Problem reattaching session: " + op.error());

    return std::shared_ptr<media::Player>(new media::PlayerStub
    {
        shared_from_this(),
        access_service(),
        access_service()->object_for_path(op.value()),
        uuid
    });
}

void media::ServiceStub::destroy_session(const std::string& uuid,
        const media::Player::Configuration&)
{
    const auto op = object->invoke_method_synchronously<mpris::Service::DestroySession,
         void>(uuid);

    if (op.is_error())
        throw std::runtime_error("Problem destroying session: " + op.error());
}

std::shared_ptr<media::Player> media::ServiceStub::create_fixed_session(const std::string& name,
        const media::Player::Configuration&)
{
    const auto op = object->invoke_method_synchronously<mpris::Service::CreateFixedSession,
         dbus::types::ObjectPath>(name);

    if (op.is_error())
        throw std::runtime_error("Problem creating session: " + op.error());

    return std::shared_ptr<media::Player>(new media::PlayerStub
    {
        shared_from_this(),
        access_service(),
        access_service()->object_for_path(op.value())
    });
}

std::shared_ptr<media::Player> media::ServiceStub::resume_session(media::Player::PlayerKey key)
{
    const auto op = object->invoke_method_synchronously<mpris::Service::ResumeSession,
         dbus::types::ObjectPath>(key);

    if (op.is_error())
        throw std::runtime_error("Problem resuming session: " + op.error());

    return std::shared_ptr<media::Player>(new media::PlayerStub
    {
        shared_from_this(),
        access_service(),
        access_service()->object_for_path(op.value())
    });
}

void media::ServiceStub::pause_other_sessions(media::Player::PlayerKey key)
{
    const auto op = object->invoke_method_synchronously<mpris::Service::PauseOtherSessions,
         void>(key);

    if (op.is_error())
        throw std::runtime_error("Problem pausing other sessions: " + op.error());
}

std::map<int, double>& media::ServiceStub::equalizer_get_bands() {
  MH_INFO("not implemented");
}

void media::ServiceStub::equalizer_set_band(int band, double gain) {
  MH_INFO("not implemented");
}

const core::Signal<void>& media::ServiceStub::service_disconnected() const
{
    return signals.service_disconnected;
}

const core::Signal<void>& media::ServiceStub::service_reconnected() const
{
    return signals.service_reconnected;
}
