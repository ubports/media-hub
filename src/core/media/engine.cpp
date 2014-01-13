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

#include "engine.h"

#include <exception>

namespace music = com::ubuntu::music;

const std::string& music::Engine::Xesam::album()
{
    static const std::string s{"xesam:album"};
    return s;
}

const std::string& music::Engine::Xesam::album_artist()
{
    static const std::string s{"xesam:album_artist"};
    return s;
}

const std::string& music::Engine::Xesam::artist()
{
    static const std::string s{"xesam:artist"};
    return s;
}

const std::string& music::Engine::Xesam::as_text()
{
    static const std::string s{"xesam:as_text"};
    return s;
}

const std::string& music::Engine::Xesam::audio_bpm()
{
    static const std::string s{"xesam:audio_bmp"};
    return s;
}

const std::string& music::Engine::Xesam::auto_rating()
{
    static const std::string s{"xesam:autoRating"};
    return s;
}

const std::string& music::Engine::Xesam::comment()
{
    static const std::string s{"xesam:comment"};
    return s;
}

const std::string& music::Engine::Xesam::composer()
{
    static const std::string s{"xesam:composer"};
    return s;
}

const std::string& music::Engine::Xesam::content_created()
{
    static const std::string s{"xesam:contentCreated"};
    return s;
}

const std::string& music::Engine::Xesam::disc_number()
{
    static const std::string s{"xesam:discNumber"};
    return s;
}

const std::string& music::Engine::Xesam::first_used()
{
    static const std::string s{"xesam:firstUsed"};
    return s;
}

const std::string& music::Engine::Xesam::genre()
{
    static const std::string s{"xesam:genre"};
    return s;
}

const std::string& music::Engine::Xesam::last_used()
{
    static const std::string s{"xesam:lastUsed"};
    return s;
}

const std::string& music::Engine::Xesam::lyricist()
{
    static const std::string s{"xesam:lyricist"};
    return s;
}

const std::string& music::Engine::Xesam::title()
{
    static const std::string s{"xesam:title"};
    return s;
}

const std::string& music::Engine::Xesam::track_number()
{
    static const std::string s{"xesam:trackNumber"};
    return s;
}

const std::string& music::Engine::Xesam::url()
{
    static const std::string s{"xesam:url"};
    return s;
}

const std::string& music::Engine::Xesam::use_count()
{
    static const std::string s{"xesam:useCount"};
    return s;
}

const std::string& music::Engine::Xesam::user_rating()
{
    static const std::string s{"xesam:userRating"};
    return s;
}

double music::Engine::Volume::min()
{
    return 0.;
}

double music::Engine::Volume::max()
{
    return 1.;
}

music::Engine::Volume::Volume(double v) : value(v)
{
    if (value < min() || value > max())
        throw std::runtime_error("Value exceeds limits");
}

bool music::Engine::Volume::operator!=(const music::Engine::Volume& rhs) const
{
    return value != rhs.value;
}

bool music::Engine::Volume::operator==(const music::Engine::Volume& rhs) const
{
    return value == rhs.value;
}
