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

#ifndef WAITABLE_STATE_TRANSITION_H_
#define WAITABLE_STATE_TRANSITION_H_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace test
{
template<typename T>
struct WaitableStateTransition
{
    WaitableStateTransition(const T& initial_state)
            : last_state(initial_state)
    {
    }

    bool wait_for_state_for(const T& expected_final_state,
                            const std::chrono::milliseconds& duration)
    {
        std::unique_lock<std::mutex> ul{mutex};

        while (last_state != expected_final_state)
        {
            auto status = cv.wait_for(ul, duration);

            if (status == std::cv_status::timeout)
                return false;

            // In theory, this is not required. However, if executing under
            // valgrind and together with its single-threaded execution model, we
            // need to give up the timeslice here.
            std::this_thread::yield();
        }

        return true;
    }

    void trigger(const T& new_state)
    {
        std::lock_guard<std::mutex> lg{mutex};
        last_state = new_state;

        cv.notify_all();
    }

    T last_state;
    std::mutex mutex;
    std::condition_variable cv;
};
}

#endif // WAITABLE_STATE_TRANSITION_H_
