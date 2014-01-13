/*
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

#include "player_implementation.h"

#include "engine.h"
#include "track_list_implementation.h"

#include <core/media/property.h>

namespace music = com::ubuntu::music;

struct music::PlayerImplementation::Private
{
    Private(PlayerImplementation* parent,
            const dbus::types::ObjectPath& session_path,
            const std::shared_ptr<music::Service>& service,
            const std::shared_ptr<music::Engine>& engine)
        : parent(parent),
          service(service),
          engine(engine),
          session_path(session_path),
          track_list(
              new music::TrackListImplementation(
                  session_path.as_string() + "/TrackList",
                  engine->meta_data_extractor()))
    {
        engine->state().changed().connect(
                    [parent](const Engine::State& state)
                    {
            switch(state)
            {
            case Engine::State::ready: parent->playback_status().set(music::Player::ready); break;
            case Engine::State::playing: parent->playback_status().set(music::Player::playing); break;
            case Engine::State::stopped: parent->playback_status().set(music::Player::stopped); break;
            case Engine::State::paused: parent->playback_status().set(music::Player::paused); break;
            default:
                break;
            };
                    });
    }

    PlayerImplementation* parent;
    std::shared_ptr<Service> service;
    std::shared_ptr<Engine> engine;
    dbus::types::ObjectPath session_path;
    std::shared_ptr<TrackListImplementation> track_list;
};

music::PlayerImplementation::PlayerImplementation(
        const dbus::types::ObjectPath& session_path,
        const std::shared_ptr<Service>& service,
        const std::shared_ptr<Engine>& engine)
    : music::PlayerSkeleton(session_path),
      d(new Private(
            this,
            session_path,
            service,
            engine))
{
    // Initializing default values for properties
    can_play().set(true);
    can_pause().set(true);
    can_seek().set(true);
    can_go_previous().set(true);
    can_go_next().set(true);
    is_shuffle().set(true);
    playback_rate().set(1.f);
    playback_status().set(Player::PlaybackStatus::null);
    loop_status().set(Player::LoopStatus::none);
}

music::PlayerImplementation::~PlayerImplementation()
{
}

std::shared_ptr<music::TrackList> music::PlayerImplementation::track_list()
{
    return d->track_list;
}

bool music::PlayerImplementation::open_uri(const Track::UriType& uri)
{
    std::cout << __PRETTY_FUNCTION__ << ": " << uri << std::endl;
    return d->engine->open_resource_for_uri(uri);
}

void music::PlayerImplementation::next()
{
}

void music::PlayerImplementation::previous()
{
}

void music::PlayerImplementation::play()
{
    /*if (playback_status() == music::Player::null)
    {
        if (d->track_list->has_next())
            if (open_uri(d->track_list->next()->))
    }*/
    d->engine->play();
}

void music::PlayerImplementation::pause()
{
    d->engine->pause();
}

void music::PlayerImplementation::stop()
{
    d->engine->stop();
}

void music::PlayerImplementation::seek_to(const std::chrono::microseconds& ms)
{
    d->engine->seek_to(ms);
}
