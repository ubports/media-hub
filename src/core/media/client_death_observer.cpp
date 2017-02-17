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

#include "core/media/logger/logger.h"

#include <core/media/client_death_observer.h>
#include <core/media/hybris_client_death_observer.h>
#include <core/media/stub_client_death_observer.h>
#include <core/media/hybris_client_death_observer.h>

namespace media = core::ubuntu::media;

// Accesses the default client death observer implementation for the platform.
media::ClientDeathObserver::Ptr media::platform_default_client_death_observer()
{
    const media::AVBackend::Backend b {media::AVBackend::get_backend_type()};
    switch (b)
    {
        case media::AVBackend::Backend::hybris:
            return media::HybrisClientDeathObserver::create();
        case media::AVBackend::Backend::mir:
        case media::AVBackend::Backend::none:
            MH_WARNING(
                "No video backend selected. Client disconnect functionality won't work."
            );
            return media::StubClientDeathObserver::create();
        default:
            MH_INFO("Invalid or no A/V backend specified, using \"hybris\" as a default.");
            return media::HybrisClientDeathObserver::create();
    }
}
