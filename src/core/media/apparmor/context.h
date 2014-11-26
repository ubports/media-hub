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
#ifndef CORE_UBUNTU_MEDIA_APPARMOR_AUTHENTICATOR_H_
#define CORE_UBUNTU_MEDIA_APPARMOR_AUTHENTICATOR_H_

#include <memory>
#include <string>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace apparmor
{
// Models an apparmor context name, and provides convenience functionality
// on top of it.
class Context
{
public:
    // Constructs a new Context instance for the given raw name.
    // Throws std::logic_error for empty names.
    explicit Context(const std::string& name);
    virtual ~Context() = default;
    // Returns the raw string describing the context.
    const std::string& str() const;

private:
    const std::string name;
};
}
}
}
}
#endif // CORE_UBUNTU_MEDIA_APPARMOR_AUTHENTICATOR_H_
