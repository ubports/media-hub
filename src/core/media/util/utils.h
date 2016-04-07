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
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <boost/format.hpp>

#include <string>
#include <vector>

#define MCS_STR_VALUE(str) #str

namespace core {
namespace ubuntu {
namespace media {
typedef int64_t TimestampNs;
typedef int64_t TimestampUs;
struct Utils
{
    // Merely used as a namespace.
    Utils() = delete;

    // Sprintf - much like what you would expect :)
    template<typename... Types>
    static std::string Sprintf(const std::string& fmt_str, Types&&... args);
    // GetEnv - returns a variable value from the environment
    static uint64_t GetNowNs();
    // GetNowUs - get a timestamp in microseconds
    static uint64_t GetNowUs();
};

namespace impl {
// Base case, just return the passed in boost::format instance.
inline boost::format& Sprintf(boost::format& f)
{
    return f;
}
// Sprintf recursively walks the parameter pack at compile time.
template <typename Head, typename... Tail>
inline boost::format& Sprintf(boost::format& f, Head const& head, Tail&&... tail) {
    return Sprintf(f % head, std::forward<Tail>(tail)...);
}
} // namespace impl
} // namespace media
} // namespace ubuntu
} // namespace core

template <typename... Types>
inline std::string core::ubuntu::media::Utils::Sprintf(const std::string& format, Types&&... args) {
    boost::format f(format);
    return core::ubuntu::media::impl::Sprintf(f, std::forward<Types>(args)...).str();
}

#endif
