/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#include "xesam.h"

#include <core/media/track.h>

#include <glib.h>

namespace media = core::ubuntu::media;

std::string media::Track::MetaData::encode(const std::string& key) const
{
    if (not is_set(key))
        return std::string{};

    char* escaped {g_uri_escape_string(map.at(key).c_str(),
                    "!$&'()*+,;=:/?[]@", // Reserved chars
                    true)};
    if (!escaped)
    {
        return std::string{};
    }
    else
    {
        std::string s{escaped};
        g_free(escaped);
        return s;
    }
}

const std::string& media::Track::MetaData::album() const
{
    return map.at(xesam::Album::name);
}

const std::string& media::Track::MetaData::artist() const
{
    return map.at(xesam::Artist::name);
}

const std::string& media::Track::MetaData::title() const
{
    return map.at(xesam::Title::name);
}

const std::string& media::Track::MetaData::track_id() const
{
    return map.at(media::Track::MetaData::TrackIdKey);
}

const std::string& media::Track::MetaData::track_length() const
{
    return map.at(media::Track::MetaData::TrackLengthKey);
}

const std::string& media::Track::MetaData::art_url() const
{
    return map.at(media::Track::MetaData::TrackArtlUrlKey);
}

const std::string& media::Track::MetaData::last_used() const
{
    return map.at(xesam::LastUsed::name);
}

void media::Track::MetaData::set_album(const std::string& album)
{
    map[xesam::Album::name] = album;
}

void media::Track::MetaData::set_artist(const std::string& artist)
{
    map[xesam::Artist::name] = artist;
}

void media::Track::MetaData::set_title(const std::string& title)
{
    map[xesam::Title::name] = title;
}

void media::Track::MetaData::set_track_id(const std::string& id)
{
    map[media::Track::MetaData::TrackIdKey] = id;
}

void media::Track::MetaData::set_track_length(const std::string& length)
{
    map[media::Track::MetaData::TrackLengthKey] = length;
}

void media::Track::MetaData::set_art_url(const std::string& url)
{
    map[media::Track::MetaData::TrackArtlUrlKey] = url;
}

void media::Track::MetaData::set_last_used(const std::string& datetime)
{
    map[xesam::LastUsed::name] = datetime;
}
