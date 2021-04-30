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

#include <core/media/apparmor/context.h>

namespace apparmor = core::ubuntu::media::apparmor;

apparmor::Context::Context(const QString &name) : name{name}
{
    if (name.isEmpty()) throw std::runtime_error
    {
        "apparmor::Context cannot be created for empty name."
    };
}

const QString &apparmor::Context::str() const
{
    return name;
}
