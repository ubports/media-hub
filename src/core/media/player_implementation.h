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

#ifndef CORE_UBUNTU_MEDIA_PLAYER_IMPLEMENTATION_H_
#define CORE_UBUNTU_MEDIA_PLAYER_IMPLEMENTATION_H_

#include "player_skeleton.h"

#include "client_death_observer.h"
#include "power/state_controller.h"

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class Engine;
class Service;

class PlayerImplementation : public PlayerSkeleton
{
public:
    // All creation time arguments go here
    struct Configuration
    {
        std::string identity;
        std::shared_ptr<core::dbus::Bus> bus;
        std::shared_ptr<core::dbus::Object> session;
        std::shared_ptr<Service> service;
        PlayerKey key;

        // Functional dependencies
        ClientDeathObserver::Ptr client_death_observer;
        power::StateController::Ptr power_state_controller;
    };

    PlayerImplementation(const Configuration& configuration);
    ~PlayerImplementation();

    virtual std::shared_ptr<TrackList> track_list();
    virtual PlayerKey key() const;

    virtual video::Sink::Ptr create_gl_texture_video_sink(std::uint32_t texture_id);

    virtual bool open_uri(const Track::UriType& uri);
    virtual bool open_uri(const Track::UriType& uri, const Player::HeadersType& headers);
    virtual void next();
    virtual void previous();
    virtual void play();
    virtual void pause();
    virtual void stop();
    virtual void seek_to(const std::chrono::microseconds& offset);

    const core::Signal<>& on_client_disconnected() const;
private:
    struct Private;
    std::shared_ptr<Private> d;
};
}
}
}
#endif // CORE_UBUNTU_MEDIA_PLAYER_IMPLEMENTATION_H_

