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
#ifndef COM_UBUNTU_MUSIC_SIGNAL_H_
#define COM_UBUNTU_MUSIC_SIGNAL_H_

#include <core/media/connection.h>

#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <set>

namespace com
{
namespace ubuntu
{
namespace music
{
template<typename T>
class Signal
{
  public:
    static unsigned int& tag()
    {
        static unsigned int counter = 0;
        return counter;
    }

    typedef std::function<void(const T&)> Slot;

    inline Signal() : d(new Private())
    {
    }

    Signal(const Signal&) = delete;
    inline ~Signal() = default;

    Signal& operator=(const Signal&) = delete;
    bool operator==(const Signal&) const = delete;

    inline Connection connect(const Slot& slot) const
    {
        std::lock_guard<std::mutex> lg(d->guard);
        auto result = d->slots.insert(d->slots.end(), slot);

        return Connection(
            std::bind(
                &Private::disconnect_slot_for_iterator,
                d,
                result));
    }

    void operator()(const T& value)
    {
        std::lock_guard<std::mutex> lg(d->guard);
        for(auto slot : d->slots)
        {
            slot(value);
        }
    }

  private:
    struct Private
    {
        void disconnect_slot_for_iterator(typename std::list<Slot>::iterator it)
        {
            std::lock_guard<std::mutex> lg(guard);
            slots.erase(it);
        }
        
        std::mutex guard;
        std::list<Slot> slots;
        unsigned int counter = Signal<T>::tag()++;
    };
    std::shared_ptr<Private> d;
};

template<>
class Signal<void>
{
  public:
    typedef std::function<void()> Slot;

    inline Signal() : d(new Private())
    {
    }

    Signal(const Signal&) = delete;
    inline ~Signal() = default;

    Signal& operator=(const Signal&) = delete;
    bool operator==(const Signal&) const = delete;

    inline Connection connect(const Slot& slot) const
    {
        std::lock_guard<std::mutex> lg(d->guard);
        auto result = d->slots.insert(d->slots.end(), slot);

        return Connection(
            std::bind(
                &Private::disconnect_slot_for_iterator,
                d,
                result));
    }

    void operator()()
    {
        std::lock_guard<std::mutex> lg(d->guard);
        for(auto slot : d->slots)
        {
            slot();
        }
    }

  private:
    struct Private
    {
        void disconnect_slot_for_iterator(typename std::list<Slot>::iterator it)
        {
            std::lock_guard<std::mutex> lg(guard);
            slots.erase(it);
        }

        std::mutex guard;
        std::list<Slot> slots;
    };
    std::shared_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_SIGNAL_H_
