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

#ifndef MPRIS_SERVICE_H_
#define MPRIS_SERVICE_H_

#include <core/dbus/macros.h>

#include <chrono>
#include <string>

namespace mpris
{
struct Service
{
    static const std::string& name()
    {
        static const std::string s{"core.ubuntu.media.Service"};
        return s;
    }

    struct Errors
    {
        struct CreatingSession
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.CreatingSession"
                };
                return s;
            }
        };

        struct DetachingSession
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.DetachingSession"
                };
                return s;
            }
        };

        struct ReattachingSession
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.ReattachingSession"
                };
                return s;
            }
        };

        struct DestroyingSession
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.DestroyingSession"
                };
                return s;
            }
        };

        struct CreatingFixedSession
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.CreatingFixedSession"
                };
                return s;
            }
        };

        struct ResumingSession
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.ResumingSession"
                };
                return s;
            }
        };

        struct PlayerKeyNotFound
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.PlayerKeyNotFound"
                };
                return s;
            }
        };

        struct EqualizerSetBand
        {
            static const std::string& name()
            {
                static const std::string s
                {
                    "core.ubuntu.media.Service.Error.EqualizerSetBand"
                };
                return s;
            }
        };
    };

    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(CreateSession, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(DetachSession, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(ReattachSession, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(DestroySession, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(CreateFixedSession, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(ResumeSession, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(PauseOtherSessions, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(EqualizerGetBands, Service, 1000)
    DBUS_CPP_METHOD_WITH_TIMEOUT_DEF(EqualizerSetBand, Service, 1000)
};
}

#endif // MPRIS_SERVICE_H_
