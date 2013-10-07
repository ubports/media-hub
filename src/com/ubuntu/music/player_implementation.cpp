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
#include "track_list_implementation.h"

#include <com/ubuntu/music/property.h>

namespace music = com::ubuntu::music;

struct music::PlayerImplementation::Private
{

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
      d(new Private
        {
            service,
            engine,
            session_path,
            std::make_shared<music::TrackListImplementation>(session_path.as_string() + "/TrackList")
        })
{
    // Initializing default values for properties
    can_play().set(true);
    can_pause().set(true);
    can_seek().set(true);
    can_go_previous().set(true);
    can_go_next().set(true);
    is_shuffle().set(true);
    playback_rate().set(1.f);
    playback_status().set(Player::PlaybackStatus::stopped);
    loop_status().set(Player::LoopStatus::none);
}

music::PlayerImplementation::~PlayerImplementation()
{
}

std::shared_ptr<music::TrackList> music::PlayerImplementation::track_list()
{
    return d->track_list;
}

bool music::PlayerImplementation::open_uri(const Track::UriType&)
{
    return false;
}

void music::PlayerImplementation::next()
{
}

void music::PlayerImplementation::previous()
{
}

void music::PlayerImplementation::play()
{

}

void music::PlayerImplementation::pause()
{
}

void music::PlayerImplementation::stop()
{
}

void music::PlayerImplementation::seek_to(const std::chrono::microseconds&)
{
}
