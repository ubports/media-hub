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

#include "qtbridge.h"

#include<QCoreApplication>
#include<QNetworkAccessManager>
#include<QNetworkRequest>
#include<QNetworkReply>
#include<QThread>
#include<QDebug>

#include <iostream>

namespace
{
QCoreApplication* app = nullptr;
}

namespace qt
{
namespace core
{
namespace world
{
namespace detail
{
QEvent::Type qt_core_world_task_event_type()
{
    static QEvent::Type event_type = static_cast<QEvent::Type>(QEvent::registerEventType());
    return event_type;
}

class TaskEvent : public QEvent
{
public:
    TaskEvent(const std::function<void()>& task)
        : QEvent(qt_core_world_task_event_type()),
          task(task)
    {
    }

    void run()
    {
        try
        {
            task();
            promise.set_value();
        } catch(...)
        {
            promise.set_exception(std::current_exception());
        }
    }

    std::future<void> get_future()
    {
        return promise.get_future();
    }

private:
    std::function<void()> task;
    std::promise<void> promise;
};

class TaskHandler : public QObject
{
    Q_OBJECT

public:
    TaskHandler(QObject* parent) : QObject(parent)
    {
    }

    bool event(QEvent* e);
};



void createCoreApplicationInstanceWithArgs(int argc, char** argv)
{
    app = new QCoreApplication(argc, argv);
}

void destroyCoreApplicationInstace()
{
    delete app;
}

QCoreApplication* coreApplicationInstance()
{
    return app;
}

TaskHandler* task_handler()
{
    static TaskHandler* instance = new TaskHandler(coreApplicationInstance());
    return instance;
}

bool TaskHandler::event(QEvent *e)
{
    if (e->type() != qt_core_world_task_event_type())
        return QObject::event(e);

    auto te = dynamic_cast<TaskEvent*>(e);
    if (te)
    {
        te->run();
        return true;
    }

    return false;
}
}

void build_and_run(int argc, char** argv, const std::function<void()>& ready)
{
    QThread::currentThread();
    if (QCoreApplication::instance() != nullptr)
        throw std::runtime_error(
                "qt::core::world::build_and_run: "
                "There is already a QCoreApplication running.");

    detail::createCoreApplicationInstanceWithArgs(argc, argv);

    detail::task_handler()->moveToThread(
                detail::coreApplicationInstance()->thread());

    // Signal to other worlds that we are good to go.
    ready();

    detail::coreApplicationInstance()->exec();

    // Someone has called quit and we clean up on the correct thread here.
    detail::destroyCoreApplicationInstace();
}

void destroy()
{
    enter_with_task([]()
    {
        // We make sure that all tasks have completed before quitting the app.
        QEventLoopLocker locker;
    }).wait_for(std::chrono::seconds{1});
}

std::future<void> enter_with_task(const std::function<void()>& task)
{
    QCoreApplication* instance = QCoreApplication::instance();

    if (!instance)
    {
        throw std::runtime_error("Qt world has not been built before calling this function.");
    }

    detail::TaskEvent* te = new detail::TaskEvent(task);
    auto future = te->get_future();

    // We hand over ownership of te here. The event is deleted later after it has
    // been processed by the event loop.
    instance->postEvent(detail::task_handler(), te);

    return future;
}

}
}
}

#include "qtbridge.moc"
