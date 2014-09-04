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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */

#ifndef MPRIS_PLAYER_H_
#define MPRIS_PLAYER_H_

#include <core/media/player.h>
#include <core/media/track.h>

#include "macros.h"

#include <core/dbus/types/any.h>
#include <core/dbus/types/object_path.h>
#include <core/dbus/types/variant.h>

#include <boost/utility/identity_type.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace dbus = core::dbus;

namespace mpris
{
struct Player
{
    static const std::string& name()
    {
        static const std::string s{"core.ubuntu.media.Service.Player"};
        return s;
    }

    METHOD(Next, Player, std::chrono::seconds(1))
    METHOD(Previous, Player, std::chrono::seconds(1))
    METHOD_WITH_TIMEOUT_MS(Pause, Player, 1000)
    METHOD(PlayPause, Player, std::chrono::seconds(1))
    METHOD(Stop, Player, std::chrono::seconds(1))
    METHOD(Play, Player, std::chrono::seconds(1))
    METHOD(Seek, Player, std::chrono::seconds(1))
    METHOD(SetPosition, Player, std::chrono::seconds(1))
    METHOD(CreateVideoSink, Player, std::chrono::seconds(1))
    METHOD(Key, Player, std::chrono::seconds(1))
    METHOD(OpenUri, Player, std::chrono::seconds(1))

    struct Signals
    {
        SIGNAL(Seeked, Player, uint64_t)
        SIGNAL(EndOfStream, Player, void)
        SIGNAL(PlaybackStatusChanged, Player, core::ubuntu::media::Player::PlaybackStatus)
    };

    struct Properties
    {
        READABLE_PROPERTY(PlaybackStatus, Player, core::ubuntu::media::Player::PlaybackStatus)
        WRITABLE_PROPERTY(LoopStatus, Player, core::ubuntu::media::Player::LoopStatus)
        WRITABLE_PROPERTY(PlaybackRate, Player, core::ubuntu::media::Player::PlaybackRate)
        WRITABLE_PROPERTY(Rate, Player, double)
        WRITABLE_PROPERTY(Shuffle, Player, bool)
        READABLE_PROPERTY(MetaData, Player, core::ubuntu::media::Track::MetaData)
        WRITABLE_PROPERTY(Volume, Player, double)
        READABLE_PROPERTY(Position, Player, uint64_t)
        READABLE_PROPERTY(Duration, Player, uint64_t)
        WRITABLE_PROPERTY(AudioStreamRole, Player, core::ubuntu::media::Player::AudioStreamRole)
        READABLE_PROPERTY(MinimumRate, Player, double)
        READABLE_PROPERTY(MaximumRate, Player, double)
        READABLE_PROPERTY(IsVideoSource, Player, bool)
        READABLE_PROPERTY(IsAudioSource, Player, bool)
        READABLE_PROPERTY(CanGoNext, Player, bool)
        READABLE_PROPERTY(CanGoPrevious, Player, bool)
        READABLE_PROPERTY(CanPlay, Player, bool)
        READABLE_PROPERTY(CanPause, Player, bool)
        READABLE_PROPERTY(CanSeek, Player, bool)
        READABLE_PROPERTY(CanControl, Player, bool)
    };
};
}

#endif // MPRIS_PLAYER_H_
