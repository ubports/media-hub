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

#include "service_skeleton.h"

#include "mpris/service.h"
#include "player_configuration.h"
#include "the_session_bus.h"

#include <org/freedesktop/dbus/message.h>
#include <org/freedesktop/dbus/types/object_path.h>

#include <map>
#include <sstream>

namespace dbus = org::freedesktop::dbus;
namespace music = com::ubuntu::music;

namespace
{
std::map<dbus::types::ObjectPath, std::shared_ptr<music::Player>> session_store;
}

struct music::ServiceSkeleton::Private
{
    Private(music::ServiceSkeleton* impl)
        : impl(impl),
          object(impl->access_service()->add_object_for_path(
                     dbus::traits::Service<music::Service>::object_path()))
    {
        object->install_method_handler<mpris::Service::CreateSession>(
                    std::bind(
                        &Private::handle_create_session,
                        this,
                        std::placeholders::_1));
    }

    void handle_create_session(DBusMessage* msg)
    {
        static unsigned int session_counter = 0;

        std::stringstream ss;
        ss << "/com/ubuntu/music/Service/sessions/" << session_counter++;

        dbus::types::ObjectPath op{ss.str()};
        music::Player::Configuration config{op};

        try
        {
            auto session = impl->create_session(config);

            bool inserted = false;
            std::tie(std::ignore, inserted)
                    = session_store.insert(std::make_pair(op, session));

            if (!inserted)
                throw std::runtime_error("Problem persisting session in session store.");

            auto reply = dbus::Message::make_method_return(msg);
            reply->writer() << op;

            impl->access_bus()->send(reply->get());
        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::CreatingSession::name(),
                        e.what());
            impl->access_bus()->send(reply->get());
        }
    }

    music::ServiceSkeleton* impl;
    dbus::Object::Ptr object;

};

music::ServiceSkeleton::ServiceSkeleton()
    : dbus::Skeleton<music::Service>(the_session_bus()),
      d(new Private(this))
{
}

music::ServiceSkeleton::~ServiceSkeleton()
{
}

void music::ServiceSkeleton::run()
{
    access_bus()->run();
}

void music::ServiceSkeleton::stop()
{
    access_bus()->stop();
}
