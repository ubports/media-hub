/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include <core/media/player.h>

#include "apparmor/ubuntu.h"
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

template<typename Parent>
class PlayerImplementation : public Parent
{
public:
    // All creation time arguments go here
    struct Configuration
    {
        // All creation time configuration options of the Parent class.
        typename Parent::Configuration parent;
        // The unique key identifying the player instance.
        Player::PlayerKey key;
        // Functional dependencies
        ClientDeathObserver::Ptr client_death_observer;
        power::StateController::Ptr power_state_controller;
    };

    PlayerImplementation(const Configuration& configuration);
    ~PlayerImplementation();

    virtual std::string uuid() const;
    virtual void reconnect();
    virtual void abandon();

    virtual std::shared_ptr<TrackList> track_list();
    virtual Player::PlayerKey key() const;

    virtual video::Sink::Ptr create_gl_texture_video_sink(std::uint32_t texture_id);

    virtual bool open_uri(const Track::UriType& uri);
    virtual bool open_uri(const Track::UriType& uri, const Player::HeadersType& headers);
    virtual void next();
    virtual void previous();
    virtual void play();
    virtual void pause();
    virtual void stop();
    virtual void seek_to(const std::chrono::microseconds& offset);
    virtual void equalizer_set_band(int band, double gain);

    const core::Signal<>& on_client_disconnected() const;

protected:
    void emit_playback_status_changed(const Player::PlaybackStatus &status);

private:
    struct Private;
    std::shared_ptr<Private> d;
};

}
}
}
#endif // CORE_UBUNTU_MEDIA_PLAYER_IMPLEMENTATION_H_
