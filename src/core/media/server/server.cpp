/*
 * Copyright (C) 2014 Canonical Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#include <core/media/service.h>
#include <core/media/player.h>
#include <core/media/track_list.h>

#include "core/media/backend.h"
#include "core/media/hashed_keyed_player_store.h"
#include "core/media/logger/logger.h"
#include "core/media/service_implementation.h"

#include <core/posix/signal.h>

#include <iostream>

namespace media = core::ubuntu::media;

using namespace std;

#if defined(MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
#include <hybris/media/media_codec_layer.h>

namespace
{
void logger_init()
{
    const char *level = ::getenv("MH_LOG_LEVEL");
    // Default level is kInfo
    media::Logger::Severity severity{media::Logger::Severity::kInfo};
    if (level)
    {
        if (strcmp(level, "trace") == 0)
            severity = media::Logger::Severity::kTrace;
        else if (strcmp(level, "debug") == 0)
            severity = media::Logger::Severity::kDebug;
        else if (strcmp(level, "info") == 0)
            severity = media::Logger::Severity::kInfo;
        else if (strcmp(level, "warning") == 0)
            severity = media::Logger::Severity::kWarning;
        else if (strcmp(level, "error") == 0)
            severity = media::Logger::Severity::kError;
        else if (strcmp(level, "fatal") == 0)
            severity = media::Logger::Severity::kFatal;
        else
            std::cerr << "Invalid log level \"" << level
                << "\", setting to info. Valid options: [trace, debug, info, warning, error, fatal]."
                << std::endl;
    }
    else
        std::cout << "Using default log level: info" << std::endl;

    media::Log().Init(severity);
    cout << "Log level: " << severity << std::endl;
}

// All platform-specific initialization routines go here.
void platform_init()
{
    const media::AVBackend::Backend b {media::AVBackend::get_backend_type()};
    switch (b)
    {
        case media::AVBackend::Backend::hybris:
            decoding_service_init();
            break;
        case media::AVBackend::Backend::none:
            MH_WARNING("No video backend selected. Video functionality won't work.");
            break;
        default:
            MH_INFO("Invalid or no A/V backend specified, using \"hybris\" as a default.");
            decoding_service_init();
    }
}
}
#else  // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER
namespace
{
// All platform-specific initialization routines go here.
void platform_init()
{
    // Consciously left empty
}
}
#endif // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER

int main()
{
    auto trap = core::posix::trap_signals_for_all_subsequent_threads(
    {
        core::posix::Signal::sig_int,
        core::posix::Signal::sig_term
    });

    trap->signal_raised().connect([trap](core::posix::Signal)
    {
        trap->stop();
    });

    logger_init();

    // Init platform-specific functionality.
    platform_init();

    // We keep track of our state.
    bool shutdown_requested{false};

    // Our helper for connecting to external services.
    core::ubuntu::media::helper::ExternalServices external_services;

    // We move communication with all external services to its own worker thread
    // to keep the actual service thread free from such operations.
    std::thread external_services_worker
    {
        // We keep on running until shutdown has been explicitly requested.
        // All exceptions thrown on this thread are caught, and reported to
        // the terminal for post-mortem debugging purposes.
        [&shutdown_requested, &external_services]()
        {
            while (not shutdown_requested)
            {
                try
                {
                    // Blocking call to the underlying reactor implementation.
                    // Only returns cleanly when explicitly stopped.
                    external_services.io_service.run();
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error while executing the underlying io_service: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "Error while executing the underlying io_service." << std::endl;
                }
            }
        }
    };

    // Our common player store instance for tracking player instances.
    auto player_store = std::make_shared<media::HashedKeyedPlayerStore>();
    // We assemble the configuration for executing the service now.
    media::ServiceImplementation::Configuration service_config
    {
        std::make_shared<media::HashedKeyedPlayerStore>(),
        external_services
    };

    auto impl = std::make_shared<media::ServiceImplementation>(media::ServiceImplementation::Configuration
    {
        player_store,
        external_services
    });

    auto skeleton = std::make_shared<media::ServiceSkeleton>(media::ServiceSkeleton::Configuration
    {
        impl,
        player_store,
        external_services
    });

    std::thread service_worker
    {
        [&shutdown_requested, skeleton]()
        {
            while (not shutdown_requested)
            {
                try
                {
                    skeleton->run();
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Recoverable error while executing the service: " << e.what() << std::endl;
                }
                catch (...)
                {
                    std::cerr << "Recoverable error while executing the service." << std::endl;
                }
            }
        }
    };

    // We block on waiting for signals telling us to gracefully shutdown.
    // Incoming signals are handled in a lambda connected to signal_raised()
    // which is setup at the beginning of main(...).
    trap->run();

    // Inform our workers that we should shutdown gracefully
    shutdown_requested = true;

    // And stop execution of helper and actual service.
    skeleton->stop();

    if (service_worker.joinable())
        service_worker.join();

    external_services.stop();

    if (external_services_worker.joinable())
        external_services_worker.join();

    return 0;
}
