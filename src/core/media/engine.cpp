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
#include <stdexcept>

namespace media = core::ubuntu::media;

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
