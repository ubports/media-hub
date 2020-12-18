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

#ifndef CORE_UBUNTU_MEDIA_SERVICE_STUB_H_
#define CORE_UBUNTU_MEDIA_SERVICE_STUB_H_

#include <core/media/service.h>

#include "service_traits.h"

#include <core/dbus/bus.h>
#include <core/dbus/service_watcher.h>
#include <core/dbus/stub.h>

#include <memory>
#include <string>

namespace core
{
namespace ubuntu
{
namespace media
{
class ServiceStub : public core::dbus::Stub<core::ubuntu::media::Service>
{
  public:
    ServiceStub();
    ~ServiceStub();

    std::shared_ptr<Player> create_session(const Player::Configuration&);
    void detach_session(const std::string& uuid, const Player::Configuration&);
    std::shared_ptr<Player> reattach_session(const std::string& uuid, const Player::Configuration&);
    void destroy_session(const std::string& uuid, const Player::Configuration&);
    std::shared_ptr<Player> create_fixed_session(const std::string& name, const Player::Configuration&);
    std::shared_ptr<Player> resume_session(Player::PlayerKey key);
    void pause_other_sessions(Player::PlayerKey key);
    void equalizer_set_band(int band, double gain);

    virtual const core::Signal<void>& service_disconnected() const;
    virtual const core::Signal<void>& service_reconnected() const;

  private:
    dbus::Object::Ptr object;
    dbus::DBus daemon;
    dbus::ServiceWatcher::Ptr service_watcher_reg;
    dbus::ServiceWatcher::Ptr service_watcher_unreg;
    std::thread worker;

    struct Signals
    {
        core::Signal<void> service_disconnected;
        core::Signal<void> service_reconnected;
    } signals;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_STUB_H_
