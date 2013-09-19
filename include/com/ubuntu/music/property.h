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
#ifndef COM_UBUNTU_MUSIC_PROPERTY_H_
#define COM_UBUNTU_MUSIC_PROPERTY_H_

#include <com/ubuntu/music/signal.h>

namespace com
{
namespace ubuntu
{
namespace music
{
template<typename T>
class Property
{
  private:
    inline static T& mutable_default_value()
    {
        static T default_value = T{};
        return default_value;
    }

  public:
    typedef T ValueType;

    inline static const T& default_value()
    {
        return mutable_default_value(); 
    }

    inline static void set_default_value(const T& new_default_value)
    {
        mutable_default_value() = new_default_value;
    }

    inline explicit Property(const T& t = Property<T>::default_value()) : value{t}
    {
    }

    inline Property(const Property<T>& rhs) : value{rhs.value}
    {
    }

    inline ~Property() = default;

    inline Property& operator=(const Property<T>& rhs)
    {
        value = rhs.value;
        return *this;
    }

    inline bool operator==(const Property<T>& rhs) const
    {
        return value == rhs.value;
    }

    inline virtual void set(const T& new_value)
    {
        if (value != new_value)
        {
            value = new_value;
            signal_changed(value);
        }
    }

    inline virtual const T& get() const
    {
        return value;
    }

    inline const Signal<T>& changed() const
    {
        return signal_changed;
    }

  protected:
    inline Signal<T>& changed()
    {
        return signal_changed;
    }
    
    inline T& mutable_get() const
    {
        return value;
    }

  private:
    mutable T value;
    Signal<T> signal_changed;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_SIGNAL_H_
