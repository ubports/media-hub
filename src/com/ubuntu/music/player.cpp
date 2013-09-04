/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "com/ubuntu/music/player.h"
#include "com/ubuntu/music/service.h"

#include <limits>

namespace music = com::ubuntu::music;

struct music::Player::Private
{
    std::shared_ptr<Service> parent;
};

music::Player::Player(const std::shared_ptr<Service>& parent) : d(new Private{parent})
{
}

music::Player::~Player()
{
}

bool music::Player::can_go_next()
{
    return true;
}

void music::Player::next()
{    
}

bool music::Player::can_go_previous()
{
    return true;
}

void music::Player::previous()
{
}

bool music::Player::can_play()
{
    return true;
}

void music::Player::play()
{
}

bool music::Player::can_pause()
{
    return true;
}

void music::Player::pause()
{
}

bool music::Player::can_seek()
{
    return true;
}

void music::Player::seek_to(const std::chrono::microseconds& offset)
{
    (void) offset;
}

void music::Player::stop()
{
}

music::Player::PlaybackStatus music::Player::playback_status() const
{
    return Player::stopped;
}

music::Connection music::Player::on_playback_status_changed(const std::function<void(music::Player::PlaybackStatus)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::Player::LoopStatus music::Player::loop_status() const
{
    return Player::none;
}

void music::Player::set_loop_status(music::Player::LoopStatus new_status)
{
    (void) new_status;
}

music::Connection music::Player::on_loop_status_changed(const std::function<void(music::Player::LoopStatus)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::Player::PlaybackRate music::Player::playback_rate() const
{
    return 1.0;
}

void music::Player::set_playback_rate(music::Player::PlaybackRate rate)
{
    (void) rate;
}

music::Connection music::Player::on_playback_rate_changed(const std::function<void(music::Player::PlaybackRate)>& handler)
{
    (void) handler;

    return Connection(nullptr);
}

bool music::Player::is_shuffle() const
{
    return false;
}

void music::Player::set_shuffle(bool b)
{
    (void) b;
}

music::Connection music::Player::on_shuffle_changed(const std::function<void(bool)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::Track::MetaData music::Player::meta_data_for_current_track() const
{
    return Track::MetaData();
}

music::Connection music::Player::on_meta_data_for_current_track_changed(const std::function<void(const music::Track::MetaData&)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::Player::Volume music::Player::volume() const
{
    return 0.f;
}

void music::Player::set_volume(music::Player::Volume new_volume)
{
    (void) new_volume;
}

music::Connection music::Player::on_volume_changed(const std::function<void(music::Player::Volume)>& handler) 
{
    (void) handler;
    return Connection(nullptr);
}

music::Player::PlaybackRate music::Player::minimum_playback_rate() const
{    
    return std::numeric_limits<double>::min();
}

music::Player::PlaybackRate music::Player::maximum_playback_rate() const
{
    return std::numeric_limits<double>::max();
}

