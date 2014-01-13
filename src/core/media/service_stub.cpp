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

#include "service_stub.h"
#include "service_traits.h"

#include "player_stub.h"
#include "the_session_bus.h"

#include "mpris/service.h"

namespace dbus = org::freedesktop::dbus;
namespace media = core::ubuntu::media;

struct media::ServiceStub::Private
{
    dbus::Object::Ptr object;
};

media::ServiceStub::ServiceStub()
    : org::freedesktop::dbus::Stub<media::Service>(the_session_bus()),
      d(new Private{
        access_service()->object_for_path(
            dbus::types::ObjectPath(
                dbus::traits::Service<media::Service>::object_path()))})
{
}

media::ServiceStub::~ServiceStub()
{
}

std::shared_ptr<media::Player> media::ServiceStub::create_session(const media::Player::Configuration&)
{
    auto op
            = d->object->invoke_method_synchronously<
                mpris::Service::CreateSession,
                dbus::types::ObjectPath>();

    if (op.is_error())
        throw std::runtime_error("Problem creating session: " + op.error());

    return std::shared_ptr<media::Player>(new media::PlayerStub(shared_from_this(), op.value()));
}
