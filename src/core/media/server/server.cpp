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

#include "core/media/service_implementation.h"

#include <core/posix/signal.h>

#include <iostream>

namespace media = core::ubuntu::media;

using namespace std;

#if defined(MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
#include <hybris/media/media_codec_layer.h>

namespace
{
// All platform-specific initialization routines go here.
void platform_init()
{
    decoding_service_init();
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

    // Init platform-specific functionality.
    platform_init();

    // We keep track of our state.
    bool shutdown_requested{false};

    // Our helper for connecting to external services.
    core::ubuntu::media::helper::ExternalServices external_services;

    std::thread external_services_worker
    {
        [&shutdown_requested, &external_services]()
        {
            while (not shutdown_requested)
            {
                try
                {
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

    // We assemble the configuration for executing the service now.
    media::ServiceImplementation::Configuration service_config
    {
        external_services
    };

    auto service = std::make_shared<media::ServiceImplementation>(service_config);

    std::thread service_worker
    {
        [&shutdown_requested, service]()
        {
            while (not shutdown_requested)
            {
                try
                {
                    service->run();
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
    trap->run();

    // Inform our workers that we should shutdown gracefully
    shutdown_requested = true;

    // And stop execution of helper and actual service.
    external_services.stop();
    service->stop();

    if (external_services_worker.joinable())
        external_services_worker.join();

    if (service_worker.joinable())
        service_worker.join();

    return 0;
}
