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

#include "hybris_client_death_observer.h"

#include "core/media/logging.h"

#include <hybris/media/media_codec_layer.h>

#include <QPair>
#include <QPointer>

namespace media = core::ubuntu::media;

namespace
{
typedef QPair<media::Player::Client, QPointer<media::HybrisClientDeathObserver>> Holder;
}

void media::HybrisClientDeathObserver::on_client_died_cb(void* context)
{
    auto holder = static_cast<Holder*>(context);

    if (not holder)
        return;

    // We check if we are still alive or if we already got killed.
    if (auto sp = holder->second.data())
    {
        sp->notifyClientDeath(holder->first);
    }

    // And with that, we have reached end of life for our holder object.
    delete holder;
}

media::HybrisClientDeathObserver::HybrisClientDeathObserver(ClientDeathObserver *q):
    ClientDeathObserverPrivate(q)
{
}

media::HybrisClientDeathObserver::~HybrisClientDeathObserver()
{
}

void media::HybrisClientDeathObserver::registerForDeathNotifications(const media::Player::Client &client)
{
    decoding_service_set_client_death_cb(
            &media::HybrisClientDeathObserver::on_client_died_cb,
            client.key, new Holder{client, this});
}
