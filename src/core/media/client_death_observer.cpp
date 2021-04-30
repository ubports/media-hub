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

#include "core/media/logging.h"

#include <core/media/client_death_observer.h>
#include <core/media/hybris_client_death_observer.h>
#include <core/media/stub_client_death_observer.h>

namespace media = core::ubuntu::media;

using namespace media;

namespace {

// Accesses the default client death observer implementation for the platform.
ClientDeathObserverPrivate *
platform_default_client_death_observer(ClientDeathObserver *q)
{
    const media::AVBackend::Backend b {media::AVBackend::get_backend_type()};
    switch (b)
    {
        case media::AVBackend::Backend::hybris:
            return new media::HybrisClientDeathObserver(q);
        case media::AVBackend::Backend::mir:
        case media::AVBackend::Backend::none:
            MH_WARNING(
                "No video backend selected. Client disconnect functionality won't work."
            );
            return new media::StubClientDeathObserver(q);
        default:
            MH_INFO("Invalid or no A/V backend specified, using \"hybris\" as a default.");
            return new media::HybrisClientDeathObserver(q);
    }
}

} // namespace

ClientDeathObserver::ClientDeathObserver(QObject *parent):
    QObject(parent),
    d_ptr(platform_default_client_death_observer(this))
{
    qRegisterMetaType<Player::PlayerKey>("Player::PlayerKey");
}

ClientDeathObserver::~ClientDeathObserver() = default;

void ClientDeathObserver::registerForDeathNotificationsWithKey(const Player::PlayerKey &key)
{
    Q_D(ClientDeathObserver);
    d->registerForDeathNotificationsWithKey(key);
}
