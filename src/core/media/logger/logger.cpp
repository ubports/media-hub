/*
 * Copyright © 2016 Canonical Ltd.
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
 */

#include <thread>

#include "logger.h"

#define BOOST_LOG_DYN_LINK
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/manipulators.hpp>
#include <boost/log/utility/setup.hpp>

namespace media = core::ubuntu::media;

namespace {
namespace attrs {
BOOST_LOG_ATTRIBUTE_KEYWORD(Severity, "core::ubuntu::media::Severity", media::Logger::Severity)
BOOST_LOG_ATTRIBUTE_KEYWORD(Location, "Location", media::Logger::Location)
BOOST_LOG_ATTRIBUTE_KEYWORD(Timestamp, "Timestamp", boost::posix_time::ptime)
}

struct BoostLogLogger : public media::Logger {
    BoostLogLogger() :
        initialized_(false) {
    }

    void Init(const media::Logger::Severity &severity = media::Logger::Severity::kWarning) override {
        if (initialized_)
            return;

        boost::log::formatter formatter = boost::log::expressions::stream
            << "[" << attrs::Severity << " "
            << boost::log::expressions::format_date_time< boost::posix_time::ptime >("Timestamp", "%Y-%m-%d %H:%M:%S.%f")
            << "] "
            << boost::log::expressions::if_(boost::log::expressions::has_attr(attrs::Location))
               [
                   boost::log::expressions::stream << "[" << attrs::Location << "] "
               ]
            << boost::log::expressions::smessage;

        boost::log::core::get()->remove_all_sinks();
        auto logger = boost::log::add_console_log(std::cout);
        logger->set_formatter(formatter);

        // FIXME need to enable this once we found how we wrap this
        // properly into our service architecture. For now left as
        // it is.
        boost::ignore_unused_variable_warning(severity);
        // logger->set_filter(attrs::Severity < severity);

        initialized_ = true;
    }

    void Log(Severity severity, const std::string& message, const boost::optional<Location> &loc) {
        if (!initialized_)
            Init();

        if (auto rec = boost::log::trivial::logger::get().open_record()) {
            boost::log::record_ostream out{rec};
            out << boost::log::add_value(attrs::Severity, severity)
                << boost::log::add_value(attrs::Timestamp, boost::posix_time::microsec_clock::universal_time())
                << message;

            if (loc) {
                // We have to pass in a temporary as boost::log (<= 1.55) expects a
                // mutable reference to be passed to boost::log::add_value(...).
                auto tmp = *loc;
                out << boost::log::add_value(attrs::Location, tmp);
            }

            boost::log::trivial::logger::get().push_record(std::move(rec));
        }
    }

private:
    bool initialized_;
};

std::shared_ptr<media::Logger>& MutableInstance() {
    static std::shared_ptr<media::Logger> instance{new BoostLogLogger()};
    return instance;
}

void SetInstance(const std::shared_ptr<media::Logger>& logger) {
    MutableInstance() = logger;
}
}

void media::Logger::Trace(const std::string& message, const boost::optional<Location>& location) {
    Log(Severity::kTrace, message, location);
}

void media::Logger::Debug(const std::string& message, const boost::optional<Location>& location) {
    Log(Severity::kDebug, message, location);
}

void media::Logger::Info(const std::string& message, const boost::optional<Location>& location) {
    Log(Severity::kInfo, message, location);
}

void media::Logger::Warning(const std::string& message, const boost::optional<Location>& location) {
    Log(Severity::kWarning, message, location);
}

void media::Logger::Error(const std::string& message, const boost::optional<Location>& location) {
    Log(Severity::kError, message, location);
}

void media::Logger::Fatal(const std::string& message, const boost::optional<Location>& location) {
    Log(Severity::kFatal, message, location);
}

namespace core {
namespace ubuntu {
namespace media {

std::ostream& operator<<(std::ostream& strm, Logger::Severity severity) {
    switch (severity) {
    case media::Logger::Severity::kTrace: return strm << "TT";
    case media::Logger::Severity::kDebug: return strm << "DD";
    case media::Logger::Severity::kInfo: return strm << "II";
    case media::Logger::Severity::kWarning: return strm << "WW";
    case media::Logger::Severity::kError: return strm << "EE";
    case media::Logger::Severity::kFatal: return strm << "FF";
    default: return strm << static_cast<uint>(severity);
    }
}

std::ostream& operator<<(std::ostream& out, const Logger::Location &location) {
    return out << Utils::Sprintf("%s:%d@%s", boost::filesystem::path(location.file).filename().string(), location.line, location.function);
}

Logger& Log() {
    return *MutableInstance();
}

void SetLogger(const std::shared_ptr<Logger>& logger) {
    SetInstance(logger);
}

} // namespace media
} // namespace ubuntu
} // namespace core
