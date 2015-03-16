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

#include <core/media/client_death_observer.h>

namespace media = core::ubuntu::media;

#if defined(MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
#include <core/media/hybris_client_death_observer.h>

// Accesses the default client death observer implementation for the platform.
media::ClientDeathObserver::Ptr media::platform_default_client_death_observer()
{
    return media::HybrisClientDeathObserver::create();
}
#else  // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER
// Just throws a std::logic_error as we have not yet defined a default way to
// identify client death changes. One possible way of implementing the interface
// would be to listen to dbus name changes and react accordingly.
media::ClientDeathObserver::Ptr media::platform_default_client_death_observer()
{
    throw std::logic_error
    {
        "No platform-specific death observer implementation known."
    };
}
#endif // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER
