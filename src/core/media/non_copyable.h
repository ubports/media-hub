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

#ifndef NON_COPYABLE_H_
#define NON_COPYABLE_H_

namespace core {
namespace ubuntu {
namespace media {
// The alert reader might wonder why we don't use boost::noncopyable. The reason
// is simple: We would like to have a convenient virtual d'tor available.
struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    virtual ~NonCopyable() = default;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
}
}
}

#endif
