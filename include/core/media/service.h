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

    /** @brief Creates a session with the media-hub service. */
    virtual std::shared_ptr<Player> create_session(const Player::Configuration&) = 0;

    /** @brief Detaches a UUID-identified session for later resuming. */
    virtual void detach_session(const std::string& uuid, const Player::Configuration&) = 0;

    /** @brief Reattaches to a UUID-identified session that is in detached state. */
    virtual std::shared_ptr<Player> reattach_session(const std::string& uuid, const Player::Configuration&) = 0;

    /** @brief Asks the service to destroy a session. The session is destroyed when the client exits. */
    virtual void destroy_session(const std::string& uuid, const Player::Configuration&) = 0;

    /** @brief Creates a fixed-named session with the media-hub service. */
    virtual std::shared_ptr<Player> create_fixed_session(const std::string& name, const Player::Configuration&) = 0;

    /** @brief Resumes a fixed-name session directly by player key. */
    virtual std::shared_ptr<Player> resume_session(Player::PlayerKey) = 0;

    /** @brief Pauses sessions other than the supplied one. */
    virtual void pause_other_sessions(Player::PlayerKey) = 0;

    /** @brief Set equalizer band for all multimedia players. */
    virtual void equalizer_set_band(int band, double gain) = 0;

    /** @brief Signals when the media-hub server disappears from the bus **/
    virtual const core::Signal<void>& service_disconnected() const = 0;
    /** @brief Signals when the media-hub server reappears from the bus **/
    virtual const core::Signal<void>& service_reconnected() const = 0;

  protected:
    Service() = default;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_H_
