/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MPRIS_METADATA_H_
#define MPRIS_METADATA_H_

#include <core/dbus/types/object_path.h>

#include <cstdint>

#include <string>

namespace mpris
{
namespace metadata
{
// D-Bus path: A unique identity for this track within the context of an MPRIS object (eg: tracklist).
struct TrackId
{
    static constexpr const char* name{"mpris:trackid"};
    typedef core::dbus::types::ObjectPath ValueType;
};
// 64-bit integer: The duration of the track in microseconds.
struct Length
{
    static constexpr const char* name{"mpris:length"};
    typedef std::int64_t ValueType;
};
// URI: The location of an image representing the track or album.
// Clients should not assume this will continue to exist when the media player
// stops giving out the URL.
struct ArtUrl
{
    static constexpr const char* name{"mpris:artUrl"};
    typedef std::string ValueType;
};
}
}

#endif // MPRIS_METADATA_H_
