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

#include <core/dbus/macros.h>

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
struct TrackList
{
    static const std::string& name()
    {
        static const std::string s{"core.ubuntu.media.Service.Player.TrackList"};
        return s;
    }

    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(GetTracksMetadata, TrackList, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(AddTrack, TrackList, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(RemoveTrack, TrackList, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(GoTo, TrackList, 1000)

    struct Signals
    {
        DBUS_CPP_SIGNAL_DEF
        (
            TrackListReplaced,
            TrackList,
            BOOST_IDENTITY_TYPE((std::tuple<std::vector<dbus::types::ObjectPath>, dbus::types::ObjectPath>))
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackAdded,
            TrackList,
            BOOST_IDENTITY_TYPE((std::tuple<std::map<std::string, dbus::types::Variant>, dbus::types::ObjectPath>))
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackRemoved,
            TrackList,
            dbus::types::ObjectPath
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackMetadataChanged,
            TrackList,
            BOOST_IDENTITY_TYPE((std::tuple<std::map<std::string, dbus::types::Variant>, dbus::types::ObjectPath>))
        )
    };

    struct Properties
    {
        DBUS_CPP_READABLE_PROPERTY_DEF(Tracks, TrackList, std::vector<std::string>)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanEditTracks, TrackList, bool)
    };
};
}

#endif // MPRIS_TRACK_LIST_H_
