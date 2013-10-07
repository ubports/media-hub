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
#ifndef COM_UBUNTU_MUSIC_CONNECTION_H_
#define COM_UBUNTU_MUSIC_CONNECTION_H_

#include <functional>
#include <memory>

namespace com
{
namespace ubuntu
{
namespace music
{
class Connection
{
  public:
    typedef std::function<void()> Disconnector;

    Connection(const Disconnector& disconnector);
    ~Connection();

    bool is_connected() const;
    void disconnect();
    
  private:
    struct Private;
    std::shared_ptr<Private> d;
};

class ScopedConnection
{
  public:
    ScopedConnection(const Connection& c) : connection(c)
    {
    }

    ScopedConnection(const ScopedConnection&) = delete;

    ~ScopedConnection()
    {
        connection.disconnect();
    }

    ScopedConnection& operator=(const ScopedConnection&) = delete;
    bool operator==(const ScopedConnection&) = delete;

  private:
    Connection connection;
};

}
}
}

#endif // COM_UBUNTU_MUSIC_CONNECTION_H_
