/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "core/media/non_copyable.h"
#include "core/media/util/utils.h"

#include <boost/optional.hpp>

#include <string>

namespace core {
namespace ubuntu {
namespace media {
// A Logger enables persisting of messages describing & explaining the
// state of the system.
class Logger : public core::ubuntu::media::NonCopyable {
public:
    // Severity enumerates all known severity levels
    // applicable to log messages.
    enum class Severity {
        kTrace,
        kDebug,
        kInfo,
        kWarning,
        kError,
        kFatal
    };

    // A Location describes the origin of a log message.
    struct Location {
        std::string file; // The name of the file that contains the log message.
        std::string function; // The function that contains the log message.
        std::uint32_t line; // The line in file that resulted in the log message.
    };

    virtual void Init(const core::ubuntu::media::Logger::Severity &severity = core::ubuntu::media::Logger::Severity::kWarning) = 0;

    virtual void Log(Severity severity, const std::string &message, const boost::optional<Location>& location) = 0;

    virtual void Trace(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Debug(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Info(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Warning(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Error(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});
    virtual void Fatal(const std::string& message, const boost::optional<Location>& location = boost::optional<Location>{});


    template<typename... T>
    void Tracef(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Trace(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Debugf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Debug(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Infof(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Info(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Warningf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Warning(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Errorf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Error(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

    template<typename... T>
    void Fatalf(const boost::optional<Location>& location, const std::string& pattern, T&&...args) {
        Fatal(Utils::Sprintf(pattern, std::forward<T>(args)...), location);
    }

protected:
    Logger() = default;
};

// operator<< inserts severity into out.
std::ostream& operator<<(std::ostream& out, Logger::Severity severity);

// operator<< inserts location into out.
std::ostream& operator<<(std::ostream& out, const Logger::Location &location);

// Log returns the core::ubuntu::media-wide configured logger instance.
// Save to call before/after main.
Logger& Log();
// SetLog installs the given logger as core::ubuntu::media-wide default logger.
void SetLogger(const std::shared_ptr<Logger>& logger);

#define TRACE(...) Log().Tracef(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define DEBUG(...) Log().Debugf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define INFO(...) Log().Infof(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define WARNING(...) Log().Warningf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define ERROR(...) Log().Errorf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define FATAL(...) Log().Fatalf(Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
} // namespace media
} // namespace ubuntu
} // namespace core

#define MH_TRACE(...) core::ubuntu::media::Log().Tracef(\
        core::ubuntu::media::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define MH_DEBUG(...) core::ubuntu::media::Log().Debugf(\
        core::ubuntu::media::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define MH_INFO(...) core::ubuntu::media::Log().Infof(\
        core::ubuntu::media::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define MH_WARNING(...) core::ubuntu::media::Log().Warningf(core::ubuntu::media::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define MH_ERROR(...) core::ubuntu::media::Log().Errorf(core::ubuntu::media::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)
#define MH_FATAL(...) core::ubuntu::media::Log().Fatalf(core::ubuntu::media::Logger::Location{__FILE__, __FUNCTION__, __LINE__}, __VA_ARGS__)

#endif
