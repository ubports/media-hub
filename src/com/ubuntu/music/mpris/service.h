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

#ifndef MPRIS_SERVICE_H_
#define MPRIS_SERVICE_H_

#include "macros.h"

#include <chrono>
#include <string>

namespace mpris
{
struct Service
{
    static const std::string& name()
    {
        static const std::string s{"org.mpris.MediaPlayer2.Ubuntu"};
        return s;
    }

    METHOD(CreateSession, Service, std::chrono::seconds(1))
};
}

#endif // MPRIS_SERVICE_H_
