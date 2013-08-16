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

#ifndef MPRIS_TRACK_LIST_H_
#define MPRIS_TRACK_LIST_H_

#include "macros.h"

#include <org/freedesktop/dbus/types/any.h>
#include <org/freedesktop/dbus/types/object_path.h>
#include <org/freedesktop/dbus/types/variant.h>

#include <string>
#include <tuple>
#include <vector>

namespace dbus = org::freedesktop::dbus;

namespace mpris
{
struct TrackList
{
    METHOD(GetTracksMetadata, TrackList, SECONDS(1));
    METHOD(AddTrack, TrackList, SECONDS(1));
    METHOD(RemoveTrack, TrackList, SECONDS(1));
    METHOD(GoTo, TrackList, SECONDS(1));

    struct Signals
    {
        SIGNAL(TrackListReplaced, TrackList, std::tuple<std::vector<dbus::types::ObjectPath>, dbus::types::ObjectPath>);
        SIGNAL(TrackAdded, TrackList, std::tuple<std::map<std::string, dbus::types::Variant<types::Any>>, dbus::types::ObjectPath>);
        SIGNAL(TrackRemoved, TrackList, dbus::types::ObjectPath);
        SIGNAL(TrackMetadataChanged, TrackList, std::tuple<std::map<std::string, dbus::types::Variant<dbus::types::Any>>, dbus::types::ObjectPath>);
    };

    struct Properties
    {
        READABLE_PROPERTY(Tracks, TrackList, std::vector<dbus::types::ObjectPath>);
        READABLE_PROPERTY(CanEditTracks, TrackList, bool);
    };
};
}

#endif // MPRIS_TRACK_LIST_H_
