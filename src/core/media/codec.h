/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef CODEC_H_
#define CODEC_H_

#include <core/media/track.h>

#include <org/freedesktop/dbus/codec.h>

namespace org
{
namespace freedesktop
{
namespace dbus
{
namespace helper
{
template<>
struct TypeMapper<core::ubuntu::media::Track::MetaData>
{
    constexpr static ArgumentType type_value()
    {
        return ArgumentType::floating_point;
    }
    constexpr static bool is_basic_type()
    {
        return false;
    }
    constexpr static bool requires_signature()
    {
        return false;
    }

    static std::string signature()
    {
        static const std::string s = TypeMapper<double>::signature();
        return s;
    }
};
}

template<>
struct Codec<core::ubuntu::media::Track::MetaData>
{
    static void encode_argument(DBusMessageIter* out, const core::ubuntu::media::Track::MetaData& in)
    {
        Codec<typename core::ubuntu::location::units::Quantity<T>::value_type>::encode_argument(out, in.value());
    }

    static void decode_argument(DBusMessageIter* out, com::ubuntu::location::units::Quantity<T>& in)
    {
        typename core::ubuntu::location::units::Quantity<T>::value_type value;
        Codec<typename com::ubuntu::location::units::Quantity<T>::value_type>::decode_argument(out, value);
        in = core::ubuntu::location::units::Quantity<T>::from_value(value);
        dbus_message_iter_next(out);
    }
};
}
}
}

#endif // CODEC_H_
