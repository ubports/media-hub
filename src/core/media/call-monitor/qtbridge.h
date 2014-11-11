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
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *              Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef QT_CORE_WORLD_BRIDGE_H_
#define QT_CORE_WORLD_BRIDGE_H_

#include <QObject>

#include <functional>
#include <future>
#include <iostream>

namespace qt
{
namespace core
{
namespace world
{
/**
 * @brief Sets up the Qt core world and executes its event loop. Blocks until destroy() is called.
 * @param argc Number of arguments in argv.
 * @param argv Array of command-line arguments.
 * @param ready Functor be called when the world has been setup and is about to be executed.
 * @throw std::runtime_error in case of errors.
 */
void build_and_run(int argc, char** argv, const std::function<void()>& ready);

/**
 * @brief Destroys the Qt core world and quits its event loop.
 */
void destroy();

/**
 * @brief Enters the Qt core world and schedules the given task for execution.
 * @param task The task to be executed in the Qt core world.
 * @return A std::future that can be waited for to synchronize to the world's internal event loop.
 */
std::future<void> enter_with_task(const std::function<void()>& task);


/**
 * @brief Enters the Qt core world and schedules the given task for execution.
 * @param task The task to be executed in the Qt core world.
 * @return A std::future that can be waited for to get hold of the result of the task.
 */
template<typename T>
inline std::future<T> enter_with_task_and_expect_result(const std::function<T()>& task)
{
    std::shared_ptr<std::promise<T>> promise = std::make_shared<std::promise<T>>();
    std::future<T> future = promise->get_future();

    auto t = [promise, task]()
    {
        try
        {
            promise->set_value(task());
        } catch(...)
        {
            promise->set_exception(std::current_exception());
        }
    };

    enter_with_task(t);

    return future;
}
}
}
}

#endif // QT_CORE_WORLD_BRIDGE_H_
