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

#ifndef COM_UBUNTU_MUSIC_SERVICE_IMPLEMENTATION_H_
#define COM_UBUNTU_MUSIC_SERVICE_IMPLEMENTATION_H_

#include "service_skeleton.h"

namespace com
{
namespace ubuntu
{
namespace music
{
class ServiceImplementation : public ServiceSkeleton
{
  public:
    ServiceImplementation ();
    ~ServiceImplementation ();

    std::shared_ptr<Player> create_session(const Player::Configuration&);

  private:
    struct Private;
    std::shared_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_SERVICE_IMPLEMENTATION_H_
