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

#include <core/dbus/message.h>
#include <core/dbus/object.h>
#include <core/dbus/types/object_path.h>

#include <map>
#include <sstream>

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

namespace
{
std::map<dbus::types::ObjectPath, std::shared_ptr<media::Player>> session_store;
}

struct media::ServiceSkeleton::Private
{
    Private(media::ServiceSkeleton* impl)
        : impl(impl),
          object(impl->access_service()->add_object_for_path(
                     dbus::traits::Service<media::Service>::object_path()))
    {
        object->install_method_handler<mpris::Service::CreateSession>(
                    std::bind(
                        &Private::handle_create_session,
                        this,
                        std::placeholders::_1));
    }

    void handle_create_session(const core::dbus::Message::Ptr& msg)
    {
        static unsigned int session_counter = 0;

        std::stringstream ss;
        ss << "/core/ubuntu/media/Service/sessions/" << session_counter++;

        dbus::types::ObjectPath op{ss.str()};
        media::Player::Configuration config{op};

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

            impl->access_bus()->send(reply);
        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::CreatingSession::name(),
                        e.what());
            impl->access_bus()->send(reply);
        }
    }

    media::ServiceSkeleton* impl;
    dbus::Object::Ptr object;

};

media::ServiceSkeleton::ServiceSkeleton()
    : dbus::Skeleton<media::Service>(the_session_bus()),
      d(new Private(this))
{
}

media::ServiceSkeleton::~ServiceSkeleton()
{
}

void media::ServiceSkeleton::run()
{
    access_bus()->run();
}

void media::ServiceSkeleton::stop()
{
    access_bus()->stop();
}
