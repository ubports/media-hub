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

#ifndef XESAM_H_
#define XESAM_H_

#include <cstdint>

#include <string>
#include <vector>

#define DATUM(Type, Name, VType) \
    struct Type \
    {\
        static constexpr const char* name{#Name};\
        typedef VType ValueType;\
    };

namespace xesam
{
DATUM(Album, xesam:album, std::string)
DATUM(AlbumArtist, xesam:albumArtist, std::vector<std::string>)
DATUM(Artist, xesam:artist, std::vector<std::string>)
DATUM(AsText, xesam:asText, std::string)
DATUM(AudioBpm, xesam:audioBpm, std::int32_t)
DATUM(AutoRating, xesam:autoRating, double)
DATUM(Comment, xesam:comment, std::vector<std::string>)
DATUM(Composer, xesam:composer, std::vector<std::string>)
DATUM(ContentCreated, xesam:comment, std::string)
DATUM(DiscNumber, xesam:discNumber, std::int32_t)
DATUM(FirstUsed, xesam:firstUsed, std::string)
DATUM(Genre, xesam:genre, std::vector<std::string>)
DATUM(LastUsed, xesam:lastUsed, std::string)
DATUM(Lyricist, xesam:lyricist, std::vector<std::string>)
DATUM(Title, xesam:title, std::string)
DATUM(TrackNumber, xesam:trackNumber, std::int32_t)
DATUM(Url, xesam:url, std::string)
DATUM(UserRating, xesam:userRating, double)
}

namespace tags
{
// Does the track contain album art?
DATUM(Image, tag:image, bool)
// Does the track contain a small album art preview image?
DATUM(PreviewImage, tag::previewImage, bool)
}

#endif // XESAM_H_
