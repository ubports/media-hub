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
#ifndef COM_UBUNTU_MUSIC_SERVICE_H_
#define COM_UBUNTU_MUSIC_SERVICE_H_

#include <memory>

namespace com
{
namespace ubuntu
{
namespace music
{
class Player;

class Service : public std::enable_shared_from_this<Service>
{
  public:
    struct Client
    {
        static const std::shared_ptr<Service> instance();
    };

    Service(const Service&) = delete;
    ~Service();

    Service& operator=(const Service&) = delete;
    bool operator==(const Service&) const = delete;

    std::shared_ptr<Player> create_session();

  private:
    friend struct Client;
    Service();
};
}
}
}

#endif // COM_UBUNTU_MUSIC_SERVICE_H_
