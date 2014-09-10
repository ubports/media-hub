/**
 * Copyright (C) 2013-2014 Canonical Ltd
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

#ifndef CORE_UBUNTU_MEDIA_COVER_ART_RESOLVER_H_
#define CORE_UBUNTU_MEDIA_COVER_ART_RESOLVER_H_

#include <functional>
#include <string>

namespace core
{
namespace ubuntu
{
namespace media
{
// Functional modelling a helper to resolve artist/album names to
// cover art.
typedef std::function
<
    std::string             // Returns a URL pointing to the album art
    (
        const std::string&, // The title of the track
        const std::string&, // The name of the album
        const std::string&  // The name of the artist
    )
> CoverArtResolver;

// Return a CoverArtResolver that always resolves to
// file:///usr/share/unity/icons/album_missing.png
CoverArtResolver always_missing_cover_art_resolver();
}
}
}

#endif // CORE_UBUNTU_MEDIA_COVER_ART_RESOLVER_H_

