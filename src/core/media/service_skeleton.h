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

#ifndef CORE_UBUNTU_MEDIA_SERVICE_SKELETON_H_
#define CORE_UBUNTU_MEDIA_SERVICE_SKELETON_H_

#include <core/media/service.h>

#include "cover_art_resolver.h"
#include "keyed_player_store.h"
#include "service_traits.h"

#include <core/dbus/skeleton.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class ServiceSkeleton : public core::dbus::Skeleton<core::ubuntu::media::Service>
{
public:
    // Creation time arguments go here.
    struct Configuration
    {
        std::shared_ptr<Service> impl;
        KeyedPlayerStore::Ptr player_store;
        CoverArtResolver cover_art_resolver;
    };

    ServiceSkeleton(const Configuration& configuration);
    ~ServiceSkeleton();

    // From media::Service
    std::shared_ptr<Player> create_session(const Player::Configuration&);
    std::shared_ptr<Player> create_fixed_session(const std::string& name, const Player::Configuration&);
    std::shared_ptr<Player> resume_session(Player::PlayerKey);
    void pause_other_sessions(Player::PlayerKey key);

    void run();
    void stop();

  private:
    struct Private;
    std::shared_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_SKELETON_H_
