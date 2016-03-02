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

#include "core/media/hashed_keyed_player_store.h"

#include <core/media/service.h>
#include <core/media/player.h>

#include <gtest/gtest.h>

#include <condition_variable>
#include <functional>
#include <thread>

namespace media = core::ubuntu::media;

TEST(PlayerStore, adding_players_from_multiple_threads_works)
{
    media::HashedKeyedPlayerStore store;

    media::Player::PlayerKey key = 0;

    size_t i;
    std::vector<std::thread> workers;
    for (i=0; i<2; i++)
    {
        workers.emplace_back(std::thread([i, &key]()
        {
            //std::shared_ptr<media::Player> player {new Player()};
            //store.add_player_for_key(key, player);
            ++key;
        }));
    }

    for (i=0; i<workers.size(); ++i)
    {
        workers[i].join();
    }
}
