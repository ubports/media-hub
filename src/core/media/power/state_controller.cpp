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

#include <core/media/power/state_controller.h>

#include <core/dbus/macros.h>
#include <core/dbus/object.h>

#include <iostream>

namespace media = core::ubuntu::media;

namespace com { namespace canonical {
struct Unity
{
    struct Screen
    {
        static const std::string& name()
        {
            static std::string s = "com.canonical.Unity.Screen";
            return s;
        }

        static const core::dbus::types::ObjectPath& path()
        {
            static core::dbus::types::ObjectPath p{"/com/canonical/Unity/Screen"};
            return p;
        }

        DBUS_CPP_METHOD_DEF(keepDisplayOn, Screen)
        DBUS_CPP_METHOD_DEF(removeDisplayOnRequest, Screen)
    };
};
namespace powerd {
struct Interface
{
    static std::string& name()
    {
        static std::string s = "com.canonical.powerd";
        return s;
    }

    static const core::dbus::types::ObjectPath& path()
    {
        static core::dbus::types::ObjectPath p{"/com/canonical/powerd"};
        return p;
    }

    DBUS_CPP_METHOD_DEF(requestSysState, com::canonical::powerd::Interface)
    DBUS_CPP_METHOD_DEF(clearSysState, com::canonical::powerd::Interface)
};
}}}

namespace
{
namespace impl
{
struct DisplayStateLock : public media::power::StateController::Lock<media::power::DisplayState>,
                          public std::enable_shared_from_this<DisplayStateLock>
{
    // To safe us some typing
    typedef std::shared_ptr<DisplayStateLock> Ptr;

    // We postpone releasing the display for this amount of time.
    static boost::posix_time::seconds timeout_for_release()
    {
        return boost::posix_time::seconds{4};
    }

    // The invalid cookie marker.
    static constexpr const std::int32_t the_invalid_cookie{-1};

    DisplayStateLock(const media::power::StateController::Ptr& parent,
                     boost::asio::io_service& io_service,
                     const core::dbus::Object::Ptr& object)
        : parent{parent},
          timeout{io_service},
          object{object},
          cookie{the_invalid_cookie}
    {
    }

    // From core::ubuntu::media::power::StateController::Lock<DisplayState>
    void request_acquire(media::power::DisplayState state) override
    {
        if (state == media::power::DisplayState::off)
            return;

        std::weak_ptr<DisplayStateLock> wp{shared_from_this()};

        object->invoke_method_asynchronously_with_callback<com::canonical::Unity::Screen::keepDisplayOn, std::int32_t>(
                    [wp, state](const core::dbus::Result<std::int32_t>& result)
                    {
                        if (result.is_error())
                        {
                            std::cerr << result.error().print() << std::endl;
                            return;
                        }

                        if (auto sp = wp.lock())
                        {
                            sp->cookie = result.value();
                            sp->signals.acquired(state);
                        }
                    });
    }

    void request_release(media::power::DisplayState state) override
    {
        if (state == media::power::DisplayState::off)
            return;

        if (cookie == the_invalid_cookie)
            return;

        // We make sure that we keep ourselves alive to make sure
        // that release requests are always correctly issued.
        auto sp = shared_from_this();

        auto current_cookie(cookie);

        timeout.expires_from_now(timeout_for_release());
        timeout.async_wait([sp, state, current_cookie](const boost::system::error_code& ec)
        {
            // We only return early from the timeout handler if the operation has been
            // explicitly aborted before.
            if (ec == boost::asio::error::operation_aborted)
                return;

            sp->object->invoke_method_asynchronously_with_callback<com::canonical::Unity::Screen::removeDisplayOnRequest, void>(
                        [sp, state, current_cookie](const core::dbus::Result<void>& result)
                        {
                            if (result.is_error())
                            {
                                std::cerr << result.error().print() << std::endl;
                                return;
                            }

                            sp->signals.released(state);

                            // We might have issued a different request before and
                            // only call the display state done if the original cookie
                            // corresponds to the one we just gave up.
                            if (sp->cookie == current_cookie)
                                sp->cookie = the_invalid_cookie;

                        }, current_cookie);
        });
    }

    // Emitted whenever the acquire request completes.
    const core::Signal<media::power::DisplayState>& acquired() const override
    {
        return signals.acquired;
    }

    // Emitted whenever the release request completes.
    const core::Signal<media::power::DisplayState>& released() const override
    {
        return signals.released;
    }

    media::power::StateController::Ptr parent;
    boost::asio::deadline_timer timeout;
    core::dbus::Object::Ptr object;
    std::int32_t cookie;

    struct
    {
        core::Signal<media::power::DisplayState> acquired;
        core::Signal<media::power::DisplayState> released;
    } signals;
};

struct SystemStateLock : public media::power::StateController::Lock<media::power::SystemState>,
                         public std::enable_shared_from_this<SystemStateLock>
{
    static constexpr const char* wake_lock_name
    {
        "media-hub-playback_lock"
    };

    SystemStateLock(const media::power::StateController::Ptr& parent, const core::dbus::Object::Ptr& object)
        : parent{parent},
          object{object}
    {
    }

    // Informs the system that the caller would like
    // the system to stay active.
    void request_acquire(media::power::SystemState state) override
    {
        if (state == media::power::SystemState::suspend)
            return;

        std::lock_guard<std::mutex> lg{system_state_cookie_store_guard};
        if (system_state_cookie_store.count(state) > 0)
            return;

        std::weak_ptr<SystemStateLock> wp{shared_from_this()};

        object->invoke_method_asynchronously_with_callback<com::canonical::powerd::Interface::requestSysState, std::string>([wp, state](const core::dbus::Result<std::string>& result)
        {
            if (result.is_error()) // TODO(tvoss): We should log the error condition here.
                return;

            if (auto sp = wp.lock())
            {
                std::lock_guard<std::mutex> lg{sp->system_state_cookie_store_guard};

                sp->system_state_cookie_store[state] = result.value();
                sp->signals.acquired(state);
            }
        }, std::string{wake_lock_name}, static_cast<std::int32_t>(state));
    }

    // Informs the system that the caller does not
    // require the system to stay active anymore.
    void request_release(media::power::SystemState state) override
    {
        if (state == media::power::SystemState::suspend)
            return;

        std::lock_guard<std::mutex> lg{system_state_cookie_store_guard};

        if (system_state_cookie_store.count(state) == 0)
            return;

        std::weak_ptr<SystemStateLock> wp{shared_from_this()};

        object->invoke_method_asynchronously_with_callback<com::canonical::powerd::Interface::clearSysState, void>([wp, state](const core::dbus::Result<void>& result)
        {
            if (result.is_error())
                std::cerr << result.error().print() << std::endl;

            if (auto sp = wp.lock())
            {
                std::lock_guard<std::mutex> lg{sp->system_state_cookie_store_guard};

                sp->system_state_cookie_store.erase(state);
                sp->signals.released(state);
            }
        }, system_state_cookie_store.at(state));
    }

    // Emitted whenever the acquire request completes.
    const core::Signal<media::power::SystemState>& acquired() const override
    {
        return signals.acquired;
    }

    // Emitted whenever the release request completes.
    const core::Signal<media::power::SystemState>& released() const override
    {
        return signals.released;
    }

    // Guards concurrent accesses to the cookie store.
    std::mutex system_state_cookie_store_guard;
    // Maps previously requested system states to the cookies returned
    // by the remote end. Used for keeping track of acquired states and
    // associated cookies to be able to release previously granted acquisitions.
    std::map<media::power::SystemState, std::string> system_state_cookie_store;
    media::power::StateController::Ptr parent;
    core::dbus::Object::Ptr object;
    struct
    {
        core::Signal<media::power::SystemState> acquired;
        core::Signal<media::power::SystemState> released;
    } signals;
};

struct StateController : public media::power::StateController,
                         public std::enable_shared_from_this<impl::StateController>
{
    StateController(media::helper::ExternalServices& es)
        : external_services{es},
          powerd
          {
              core::dbus::Service::use_service<com::canonical::powerd::Interface>(external_services.system)
                  ->object_for_path(com::canonical::powerd::Interface::path())
          },
          unity_screen
          {
              core::dbus::Service::use_service<com::canonical::Unity::Screen>(external_services.system)
                  ->object_for_path(com::canonical::Unity::Screen::path())
          }
    {
    }

    media::power::StateController::Lock<media::power::SystemState>::Ptr system_state_lock() override
    {
        return std::make_shared<impl::SystemStateLock>(shared_from_this(), powerd);
    }

    media::power::StateController::Lock<media::power::DisplayState>::Ptr display_state_lock() override
    {
        return std::make_shared<impl::DisplayStateLock>(shared_from_this(), external_services.io_service, unity_screen);
    }

    media::helper::ExternalServices& external_services;
    core::dbus::Object::Ptr powerd;
    core::dbus::Object::Ptr unity_screen;
};
}
}

media::power::StateController::Ptr media::power::make_platform_default_state_controller(core::ubuntu::media::helper::ExternalServices& external_services)
{
    return std::make_shared<impl::StateController>(external_services);
}

// operator<< pretty prints the given display state to the given output stream.
std::ostream& media::power::operator<<(std::ostream& out, media::power::DisplayState state)
{
    switch (state)
    {
    case media::power::DisplayState::off:
        return out << "DisplayState::off";
    case media::power::DisplayState::on:
        return out << "DisplayState::on";
    }

    return out;
}

// operator<< pretty prints the given system state to the given output stream.
std::ostream& media::power::operator<<(std::ostream& out, media::power::SystemState state)
{
    switch (state)
    {
    case media::power::SystemState::active:
        return out << "SystemState::active";
    case media::power::SystemState::blank_on_proximity:
        return out << "SystemState::blank_on_proximity";
    case media::power::SystemState::suspend:
        return out << "SystemState::suspend";
    }

    return out;
}
