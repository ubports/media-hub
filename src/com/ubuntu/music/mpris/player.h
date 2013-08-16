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

#ifndef MPRIS_PLAYER_H_
#define MPRIS_PLAYER_H_

#include "player.h"

#include <org/freedesktop/dbus/types/any.h>
#include <org/freedesktop/dbus/types/object_path.h>
#include <org/freedesktop/dbus/types/variant.h>

#include <string>
#include <tuple>
#include <vector>

namespace dbus = org::freedesktop::dbus;

namespace mpris
{
struct Player
{
    METHOD(Next, Player, SECONDS(1));
    METHOD(Previous, Player, SECONDS(1));
    METHOD(Pause, Player, SECONDS(1));
    METHOD(PlayPause, Player, SECONDS(1));
    METHOD(Stop, Player, SECONDS(1));
    METHOD(Play, Player, SECONDS(1));
    METHOD(Seek, Player, SECONDS(1));
    METHOD(SetPosition, Player, SECONDS(1));
    METHOD(OpenUri, Player, SECONDS(1));

    struct Signals
    {
        SIGNAL(Seeked, Player, uint64_t);
    };

    struct Properties
    {
        READABLE_PROPERTY(PlaybackStatus, Player, std::string);
        WRITABLE_PROPERTY(LoopStatus, Player, std::string);
        WRITABLE_PROPERTY(PlaybackRate, Player, double);
        WRITABLE_PROPERTY(Rate, Player, double);
        WRITABLE_PROPERTY(Shuffle, Player, bool);
        READABLE_PROPERTY(MetaData, Player, std::map<std::string, dbus::types::Variant<dbus::types::Any>>);
        WRITABLE_PROPERTY(Volume, Player, double);
        READABLE_PROPERTY(Position, Player, uint64_t);
        READABLE_PROPERTY(MinimumRate, Player, double);
        READABLE_PROPERTY(MaximumRate, Player, double);
        READABLE_PROPERTY(CanGoNext, Player, bool);
        READABLE_PROPERTY(CanGoPrevious, Player, bool);
        READABLE_PROPERTY(CanPlay, Player, bool);
        READABLE_PROPERTY(CanPause, Player, bool);
        READABLE_PROPERTY(CanSeek, Player, bool);
        READABLE_PROPERTY(CanControl, Player, bool);
    };
};
}

#endif // MPRIS_PLAYER_H_
