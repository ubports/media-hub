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

#include "service_skeleton.h"

#include "apparmor.h"

#include "mpris/service.h"
#include "player_configuration.h"
#include "the_session_bus.h"

#include <core/dbus/message.h>
#include <core/dbus/object.h>
#include <core/dbus/types/object_path.h>

#include <map>
#include <regex>
#include <sstream>

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

namespace
{
std::map<dbus::types::ObjectPath, std::shared_ptr<media::Player>> session_store;

// The sound indicator is not able to handle .desktop files contained within click
// packages. For that, we translate the short app id to a desktop entry that we know
// is present in /usr/share. See https://bugs.launchpad.net/indicator-sound/+bug/1364241
std::string translate_short_app_id(const std::string& app_id)
{
    static const std::map<std::string, std::string> lut
    {
        {"com.ubuntu.music_music", "mediaplayer-app"}
    };

    auto it = lut.find(app_id);

    if (it == lut.end())
        return app_id;

    return it->second;
}
}

struct media::ServiceSkeleton::Private
{
    Private(media::ServiceSkeleton* impl)
        : impl(impl),
          object(impl->access_service()->add_object_for_path(
                     dbus::traits::Service<media::Service>::object_path())),
          dbus_stub(impl->access_bus())
    {
        object->install_method_handler<mpris::Service::CreateSession>(
                    std::bind(
                        &Private::handle_create_session,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::PauseOtherSessions>(
                    std::bind(
                        &Private::handle_pause_other_sessions,
                        this,
                        std::placeholders::_1));
    }

    void handle_create_session(const core::dbus::Message::Ptr& msg)
    {
        static unsigned int session_counter = 0;

        std::stringstream ss;
        ss << "/core/ubuntu/media/Service/sessions/" << session_counter++;

        dbus::types::ObjectPath op{ss.str()};

        dbus_stub.get_connection_app_armor_security_async(msg->sender(), [this, msg, op](const std::string& profile)
        {
            // We post-process the profile name and try to extract the short app id.
            // Please see https://wiki.ubuntu.com/AppStore/Interfaces/ApplicationId.
            static const std::regex regex{"(.*)_(.*)_(.*)"};
            static constexpr std::size_t index_package{1};
            static constexpr std::size_t index_app{2};

            std::string identity{profile};

            // We store our matches here.
            std::smatch match;

            // See if the application id matches the pattern described in
            // https://wiki.ubuntu.com/AppStore/Interfaces/ApplicationId
            if (std::regex_match(identity, match, regex))
            {
                identity = std::string{match[index_package]} + "_" + std::string{match[index_app]};
                identity = translate_short_app_id(identity);
            }

            media::Player::Configuration config
            {
                identity,
                impl->access_bus(),
                impl->access_service()->add_object_for_path(op)
            };

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
        });
    }

    void handle_pause_other_sessions(const core::dbus::Message::Ptr& msg)
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        Player::PlayerKey key;
        msg->reader() >> key;
        impl->pause_other_sessions(key);

        auto reply = dbus::Message::make_method_return(msg);
        impl->access_bus()->send(reply);
    }

    media::ServiceSkeleton* impl;
    dbus::Object::Ptr object;

    // We query the apparmor profile to obtain an identity for players.
    org::freedesktop::dbus::DBus::Stub dbus_stub;
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
