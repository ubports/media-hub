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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#include <core/media/stub_client_death_observer.h>

namespace media = core::ubuntu::media;

namespace
{
typedef std::pair<media::Player::PlayerKey, std::weak_ptr<media::StubClientDeathObserver>> Holder;
}

void media::StubClientDeathObserver::on_client_died_cb(void* context)
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

// Creates an instance of the StubClientDeathObserver.
media::ClientDeathObserver::Ptr media::StubClientDeathObserver::create()
{
    return media::ClientDeathObserver::Ptr{new media::StubClientDeathObserver{}};
}

media::StubClientDeathObserver::StubClientDeathObserver()
{
}

media::StubClientDeathObserver::~StubClientDeathObserver()
{
}

void media::StubClientDeathObserver::register_for_death_notifications_with_key(const media::Player::PlayerKey&)
{
}

const core::Signal<media::Player::PlayerKey>& media::StubClientDeathObserver::on_client_with_key_died() const
{
    return client_with_key_died;
}
