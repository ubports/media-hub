/*
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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 *
 * Adapted from: http://stackoverflow.com/questions/14650885/how-to-create-timer-events-using-c-11
 *
 */

#ifndef TIMEOUT_H_
#define TIMEOUT_H_

#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

namespace core
{
namespace ubuntu
{
namespace media
{

class timeout
{
public:
    /**
     * @brief Start a timeout with millisecond resolution
     * @param timeout_ms Timeout value in milliseconds
     * @param async Whether to block while the timeout proceeds or not
     * @param f The function to call when the timeout expires
     * @param args Any arguments to pass to f when the timeout expires
     */
    template <class callable, class... arguments>
    timeout(int timeout_ms, bool async, callable&& f, arguments&&... args)
    {
        std::function<typename std::result_of<callable(arguments...)>::type()>
            task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

        if (async)
        {
            // Timeout without blocking
            std::thread([timeout_ms, task]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
                task();
            }).detach();
        }
        else
        {
            // Timeout with blocking
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
            task();
        }
    }
};

}
}
}

#endif // TIMEOUT_H_
