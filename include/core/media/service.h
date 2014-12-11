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
#ifndef CORE_UBUNTU_MEDIA_SERVICE_H_
#define CORE_UBUNTU_MEDIA_SERVICE_H_

#include <core/media/player.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class Service : public std::enable_shared_from_this<Service>
{
  public:
    struct Client
    {
        static const std::shared_ptr<Service> instance();
    };

    Service(const Service&) = delete;
    virtual ~Service() = default;

    Service& operator=(const Service&) = delete;
    bool operator==(const Service&) const = delete;

    virtual std::shared_ptr<Player> create_session(const Player::Configuration&) = 0;
    virtual std::shared_ptr<Player> create_fixed_session(const std::string& name, const Player::Configuration&) = 0;
    virtual std::shared_ptr<Player> resume_session(Player::PlayerKey) = 0;
    virtual void pause_other_sessions(Player::PlayerKey) = 0;

  protected:
    Service() = default;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_H_
