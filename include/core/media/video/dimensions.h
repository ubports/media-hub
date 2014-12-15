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
#ifndef CORE_UBUNTU_MEDIA_VIDEO_DIMENSIONS_H_
#define CORE_UBUNTU_MEDIA_VIDEO_DIMENSIONS_H_

#include <cstdint>

#include <tuple>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace video
{
namespace detail
{
enum class DimensionTag { width, height };

/**
 * @brief IntWrapper is a type-safe integer that allows for encoding/enforcing semantics by means of tags.
 * @tparam Tag Hint for the compiler on the semantics of the underlying integer.
 * @tparam IntegerType The underlying integer type.
 *
 * Handling dimensions like width and height with raw integer values is tedious and error prone
 * as the compiler has no way of distinguishing a width from a height, or an x coordinate from a
 * y coordinate. The problem is solvable with the tagged integer type presented here. Consider
 * the following example:
 *
 * @code{.cpp}
 * typedef IntWrapper<DimensionTag::width, std::uint32_t> Width;
 * typedef IntWrapper<DimensionTag::height, std::uint32_t> Height;
 *
 * void an_unsafe_function_expecting_width_and_height(std::uint32_t width, std::uint32_t height);
 * void a_safe_function_expecting_width_and_height(Width width, Height height);
 *
 * int main()
 * {
 *     std::uint unsafe_width{640}, unsafe_height{480};
 *     Width width{640}; Height height{480};
 *     // This will compile but is still wrong
 *     an_unsafe_function_expecting_width_and_height(unsafe_height, unsafe_width);
 *
 *    // This will not compile
 *    a_safe_function_expecting_width_and_height(height, width);
 *
 * }
 * @endcode
 */
template<DimensionTag Tag, typename IntegerType>
class IntWrapper
{
public:
    static_assert(std::is_integral<IntegerType>::value, "IntWrapper<> only supports integral types.");
    typedef IntegerType ValueType;

    IntWrapper() : value{0} {}
    template<typename AnyInteger>
    explicit IntWrapper(AnyInteger value) : value{static_cast<ValueType>(value)} {}

    template<typename T = IntegerType>
    T as() const
    {
        static_assert(std::is_arithmetic<T>::value, "as() only supports arithmetic types.");
        return static_cast<T>(value);
    }

private:
    ValueType value;
};

template<DimensionTag Tag, typename IntegerType>
std::ostream& operator<<(std::ostream& out, IntWrapper<Tag, IntegerType> const& value)
{
    out << value.template as<>();
    return out;
}

template<DimensionTag Tag, typename IntegerType>
inline bool operator == (IntWrapper<Tag, IntegerType> const& lhs, IntWrapper<Tag, IntegerType> const& rhs)
{
    return lhs.template as<>() == rhs.template as<>();
}

template<DimensionTag Tag, typename IntegerType>
inline bool operator != (IntWrapper<Tag, IntegerType> const& lhs, IntWrapper<Tag, IntegerType> const& rhs)
{
    return lhs.template as<>() != rhs.template as<>();
}

template<DimensionTag Tag, typename IntegerType>
inline bool operator <= (IntWrapper<Tag, IntegerType> const& lhs, IntWrapper<Tag, IntegerType> const& rhs)
{
    return lhs.template as<>() <= rhs.template as<>();
}

template<DimensionTag Tag, typename IntegerType>
inline bool operator >= (IntWrapper<Tag, IntegerType> const& lhs, IntWrapper<Tag, IntegerType> const& rhs)
{
    return lhs.template as<>() >= rhs.template as<>();
}

template<DimensionTag Tag, typename IntegerType>
inline bool operator < (IntWrapper<Tag, IntegerType> const& lhs, IntWrapper<Tag, IntegerType> const& rhs)
{
    return lhs.template as<>() < rhs.template as<>();
}

template<DimensionTag Tag, typename IntegerType>
inline bool operator > (IntWrapper<Tag, IntegerType> const& lhs, IntWrapper<Tag, IntegerType> const& rhs)
{
    return lhs.template as<>() > rhs.template as<>();
}
} // namespace detail

/** @brief The integer Height of a video. */
typedef detail::IntWrapper<detail::DimensionTag::height, std::uint32_t> Height;
/** @brief The integer Width of a video. */
typedef detail::IntWrapper<detail::DimensionTag::width, std::uint32_t> Width;

/** @brief Height and Width of a video. */
typedef std::tuple<Height, Width> Dimensions;
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_VIDEO_DIMENSIONS_H_
