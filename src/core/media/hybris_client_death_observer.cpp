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

#include <core/media/hybris_client_death_observer.h>

#include <hybris/media/media_codec_layer.h>

namespace media = core::ubuntu::media;


namespace
{
typedef std::pair<media::Player::PlayerKey, std::weak_ptr<media::HybrisClientDeathObserver>> Holder;
}

void media::HybrisClientDeathObserver::on_client_died_cb(void* context)
{
    auto holder = static_cast<Holder*>(context);

    if (not holder)
        return;

    // We check if we are still alive or if we already got killed.
    if (auto sp = holder->second.lock())
    {
        sp->client_with_key_died(holder->first);
    }

    // And with that, we have reached end of life for our holder object.
    delete holder;
}

// Creates an instance of the HybrisClientDeathObserver or throws
// if the underlying platform does not support it.
media::ClientDeathObserver::Ptr media::HybrisClientDeathObserver::create()
{
    return media::ClientDeathObserver::Ptr{new media::HybrisClientDeathObserver{}};
}

media::HybrisClientDeathObserver::HybrisClientDeathObserver()
{
}

media::HybrisClientDeathObserver::~HybrisClientDeathObserver()
{
}

void media::HybrisClientDeathObserver::register_for_death_notifications_with_key(const media::Player::PlayerKey& key)
{
    decoding_service_set_client_death_cb(&media::HybrisClientDeathObserver::on_client_died_cb, key, new Holder{key, shared_from_this()});
}

const core::Signal<media::Player::PlayerKey>& media::HybrisClientDeathObserver::on_client_with_key_died() const
{
    return client_with_key_died;
}
