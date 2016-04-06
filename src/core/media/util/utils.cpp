/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <memory>
#include <fstream>
#include <sstream>

#include <cstring>
#include <cstdarg>

#include "utils.h"

namespace media = core::ubuntu::media;

uint64_t media::Utils::GetNowNs() {
   struct timespec ts;
   memset(&ts, 0, sizeof(ts));
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

uint64_t media::Utils::GetNowUs() {
    return GetNowNs() / 1000;
}
