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

namespace media = core::ubuntu::media;

struct media::PlayerImplementation::Private
{
    Private(PlayerImplementation* parent,
            const dbus::types::ObjectPath& session_path,
            const std::shared_ptr<media::Service>& service,
            const std::shared_ptr<media::Engine>& engine)
        : parent(parent),
          service(service),
          engine(engine),
          session_path(session_path),
          track_list(
              new media::TrackListImplementation(
                  session_path.as_string() + "/TrackList",
                  engine->meta_data_extractor()))
    {
        engine->state().changed().connect(
                    [parent](const Engine::State& state)
                    {
            switch(state)
            {
            case Engine::State::ready: parent->playback_status().set(media::Player::ready); break;
            case Engine::State::playing: parent->playback_status().set(media::Player::playing); break;
            case Engine::State::stopped: parent->playback_status().set(media::Player::stopped); break;
            case Engine::State::paused: parent->playback_status().set(media::Player::paused); break;
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

media::PlayerImplementation::PlayerImplementation(
        const dbus::types::ObjectPath& session_path,
        const std::shared_ptr<Service>& service,
        const std::shared_ptr<Engine>& engine)
    : media::PlayerSkeleton(session_path),
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

media::PlayerImplementation::~PlayerImplementation()
{
}

std::shared_ptr<media::TrackList> media::PlayerImplementation::track_list()
{
    return d->track_list;
}

bool media::PlayerImplementation::open_uri(const Track::UriType& uri)
{
    std::cout << __PRETTY_FUNCTION__ << ": " << uri << std::endl;
    return d->engine->open_resource_for_uri(uri);
}

void media::PlayerImplementation::next()
{
}

void media::PlayerImplementation::previous()
{
}

void media::PlayerImplementation::play()
{
    /*if (playback_status() == media::Player::null)
    {
        if (d->track_list->has_next())
            if (open_uri(d->track_list->next()->))
    }*/
    d->engine->play();
}

void media::PlayerImplementation::pause()
{
    d->engine->pause();
}

void media::PlayerImplementation::stop()
{
    d->engine->stop();
}

void media::PlayerImplementation::seek_to(const std::chrono::microseconds& ms)
{
    d->engine->seek_to(ms);
}
