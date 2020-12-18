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

#ifndef CORE_UBUNTU_MEDIA_SERVICE_IMPLEMENTATION_H_
#define CORE_UBUNTU_MEDIA_SERVICE_IMPLEMENTATION_H_

#include "service_skeleton.h"
#include "external_services.h"

namespace core
{
namespace ubuntu
{
namespace media
{
class Player;

class ServiceImplementation : public Service
{
public:
    // All creation time arguments go here.
    struct Configuration
    {
        KeyedPlayerStore::Ptr player_store;
        helper::ExternalServices& external_services;
    };

    ServiceImplementation (const Configuration& configuration);
    ~ServiceImplementation ();

    std::shared_ptr<Player> create_session(const Player::Configuration&);
    void detach_session(const std::string&, const Player::Configuration&);
    std::shared_ptr<Player> reattach_session(const std::string&, const Player::Configuration&);
    void destroy_session(const std::string&, const Player::Configuration&);
    std::shared_ptr<Player> create_fixed_session(const std::string& name, const Player::Configuration&);
    std::shared_ptr<Player> resume_session(Player::PlayerKey key);
    void pause_other_sessions(Player::PlayerKey key);
    void equalizer_set_band(int band, double gain);

    const core::Signal<void>& service_disconnected() const;
    const core::Signal<void>& service_reconnected() const;

private:
    void pause_all_multimedia_sessions(bool resume_play_after_phonecall);
    void resume_paused_multimedia_sessions(bool resume_video_sessions = true);
    void resume_multimedia_session();

    struct Private;
    std::shared_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_SERVICE_IMPLEMENTATION_H_
