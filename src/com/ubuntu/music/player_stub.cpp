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

#include <com/ubuntu/music/service.h>
#include <com/ubuntu/music/track_list.h>

#include "player_stub.h"

#include "mpris/player.h"

#include <org/freedesktop/dbus/types/object_path.h>

#include <limits>

namespace dbus = org::freedesktop::dbus;
namespace music = com::ubuntu::music;

namespace
{
dbus::Bus::Ptr session_bus()
{
    static dbus::Bus::Ptr bus = std::make_shared<dbus::Bus>(dbus::WellKnownBus::session);
    return bus;
}
}

struct music::PlayerStub::Private
{
    Private(const std::shared_ptr<Service>& parent)
            : parent(parent),
              bus(session_bus())
    {
    }

    std::shared_ptr<Service> parent;
    std::shared_ptr<TrackList> track_list;

    dbus::Bus::Ptr bus;
    dbus::Object::Ptr object;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::PlaybackStatus>> playback_status;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::LoopStatus>> loop_status;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::PlaybackRate>> playback_rate;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::Rate>> rate;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::Shuffle>> is_shuffle;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::MetaData>> meta_data;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::Volume>> volume;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::Position>> position;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::MinimumRate>> minimum_rate;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::MaximumRate>> maximum_rate;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::CanGoNext>> can_go_next;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::CanGoPrevious>> can_go_previous;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::CanPlay>> can_play;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::CanPause>> can_pause;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::CanSeek>> can_seek;
    std::shared_ptr<dbus::Property<mpris::Player::Properties::CanControl>> can_control;    
};

music::PlayerStub::PlayerStub(
    const std::shared_ptr<Service>& parent,
    const dbus::types::ObjectPath& object) : dbus::Stub<Player>(session_bus()),
                                             d(new Private{parent})
{
    d->object = access_service()->object_for_path(object);
}

music::PlayerStub::~PlayerStub()
{
}

std::shared_ptr<music::TrackList> music::PlayerStub::track_list()
{
    return d->track_list;
}

bool music::PlayerStub::can_go_next()
{
    return d->can_go_next->value();
}

void music::PlayerStub::next()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Next, void>();

    if (op.is_error())
        throw std::runtime_error("Problem switching to next track on remote object");
}

bool music::PlayerStub::can_go_previous()
{
    return d->can_go_next->value();
}

void music::PlayerStub::previous()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Previous, void>();

    if (op.is_error())
        throw std::runtime_error("Problem switching to previous track on remote object");
}

bool music::PlayerStub::can_play()
{
    return d->can_play->value();
}

void music::PlayerStub::play()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Play, void>();

    if (op.is_error())
        throw std::runtime_error("Problem starting playback on remote object");
}

bool music::PlayerStub::can_pause()
{
    return d->can_play->value();
}

void music::PlayerStub::pause()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Pause, void>();

    if (op.is_error())
        throw std::runtime_error("Problem pausing playback on remote object");
}

bool music::PlayerStub::can_seek()
{
    return d->can_seek->value();
}

void music::PlayerStub::seek_to(const std::chrono::microseconds& offset)
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::SeekTo, void, uint64_t>(offset.ticks());

    if (op.is_error())
        throw std::runtime_error("Problem seeking on remote object");
}

void music::PlayerStub::stop()
{
    auto op = d->object->invoke_method_synchronously<mpris::Player::Stop, void>()

    if (op.is_error())
        throw std::runtime_error("Problem stopping playback on remote object");
}

music::Player::PlaybackStatus music::PlayerStub::playback_status() const
{
    return Player::stopped;
}

music::Connection music::PlayerStub::on_playback_status_changed(const std::function<void(music::Player::PlaybackStatus)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::PlayerStub::LoopStatus music::PlayerStub::loop_status() const
{
    return Player::none;
}

void music::PlayerStub::set_loop_status(music::Player::LoopStatus new_status)
{
    (void) new_status;
}

music::Connection music::PlayerStub::on_loop_status_changed(const std::function<void(music::Player::LoopStatus)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::PlayerStub::PlaybackRate music::PlayerStub::playback_rate() const
{
    return d->playback_rate->value();
}

void music::PlayerStub::set_playback_rate(music::Player::PlaybackRate rate)
{
    d->playback_rate->value(rate);
}

music::Connection music::PlayerStub::on_playback_rate_changed(const std::function<void(music::Player::PlaybackRate)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

bool music::PlayerStub::is_shuffle() const
{
    return d->is_shuffle->value();
}

void music::PlayerStub::set_shuffle(bool b)
{
    d->is_shuffle->value(b);
}

music::Connection music::PlayerStub::on_shuffle_changed(const std::function<void(bool)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::Track::MetaData music::PlayerStub::meta_data_for_current_track() const
{
    return Track::MetaData();
}

music::Connection music::PlayerStub::on_meta_data_for_current_track_changed(const std::function<void(const music::Track::MetaData&)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::Player::Volume music::PlayerStub::volume() const
{
    return d->volume->value();
}

void music::PlayerStub::set_volume(music::Player::Volume new_volume)
{
    d->volume->value(new_volume);
}

music::Connection music::PlayerStub::on_volume_changed(const std::function<void(music::Player::Volume)>& handler)
{
    (void) handler;
    return Connection(nullptr);
}

music::Player::PlaybackRate music::PlayerStub::minimum_playback_rate() const
{
    return d->minimum_rate->value();
}

music::Player::PlaybackRate music::PlayerStub::maximum_playback_rate() const
{
    return d->maximum_rate->value();
}
