/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "engine.h"

#include <exception>
#include <stdexcept>

namespace media = core::ubuntu::media;

const std::string& media::Engine::Xesam::album()
{
    static const std::string s{"xesam:album"};
    return s;
}

const std::string& media::Engine::Xesam::album_artist()
{
    static const std::string s{"xesam:album_artist"};
    return s;
}

const std::string& media::Engine::Xesam::artist()
{
    static const std::string s{"xesam:artist"};
    return s;
}

const std::string& media::Engine::Xesam::as_text()
{
    static const std::string s{"xesam:as_text"};
    return s;
}

const std::string& media::Engine::Xesam::audio_bpm()
{
    static const std::string s{"xesam:audio_bmp"};
    return s;
}

const std::string& media::Engine::Xesam::auto_rating()
{
    static const std::string s{"xesam:autoRating"};
    return s;
}

const std::string& media::Engine::Xesam::comment()
{
    static const std::string s{"xesam:comment"};
    return s;
}

const std::string& media::Engine::Xesam::composer()
{
    static const std::string s{"xesam:composer"};
    return s;
}

const std::string& media::Engine::Xesam::content_created()
{
    static const std::string s{"xesam:contentCreated"};
    return s;
}

const std::string& media::Engine::Xesam::disc_number()
{
    static const std::string s{"xesam:discNumber"};
    return s;
}

const std::string& media::Engine::Xesam::first_used()
{
    static const std::string s{"xesam:firstUsed"};
    return s;
}

const std::string& media::Engine::Xesam::genre()
{
    static const std::string s{"xesam:genre"};
    return s;
}

const std::string& media::Engine::Xesam::last_used()
{
    static const std::string s{"xesam:lastUsed"};
    return s;
}

const std::string& media::Engine::Xesam::lyricist()
{
    static const std::string s{"xesam:lyricist"};
    return s;
}

const std::string& media::Engine::Xesam::title()
{
    static const std::string s{"xesam:title"};
    return s;
}

const std::string& media::Engine::Xesam::track_number()
{
    static const std::string s{"xesam:trackNumber"};
    return s;
}

const std::string& media::Engine::Xesam::url()
{
    static const std::string s{"xesam:url"};
    return s;
}

const std::string& media::Engine::Xesam::use_count()
{
    static const std::string s{"xesam:useCount"};
    return s;
}

const std::string& media::Engine::Xesam::user_rating()
{
    static const std::string s{"xesam:userRating"};
    return s;
}

double media::Engine::Volume::min()
{
    return 0.;
}

double media::Engine::Volume::max()
{
    return 1.;
}

media::Engine::Volume::Volume(double v) : value(v)
{
    if (value < min() || value > max())
        throw std::runtime_error("Value exceeds limits");
}

bool media::Engine::Volume::operator!=(const media::Engine::Volume& rhs) const
{
    return value != rhs.value;
}

bool media::Engine::Volume::operator==(const media::Engine::Volume& rhs) const
{
    return value == rhs.value;
}
