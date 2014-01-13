/*
 * Copyright © 2013 Canonical Ltd.
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

#include <org/freedesktop/dbus/stub.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class ServiceStub : public org::freedesktop::dbus::Stub<core::ubuntu::media::Service>
{
  public:
    ServiceStub();
    ~ServiceStub();

    std::shared_ptr<Player> create_session(const Player::Configuration&);

  private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_STUB_H_
