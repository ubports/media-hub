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

#include "core/media/client_death_observer.h"
#include "core/media/hashed_keyed_player_store.h"
#include "core/media/player_implementation.h"
#include "core/media/player_skeleton.h"
#include "core/media/power/state_controller.h"
#include "core/media/apparmor/ubuntu.h"

#include <core/media/service.h>
#include <core/media/player.h>

#include <gtest/gtest.h>

#include <condition_variable>
#include <functional>
#include <iostream>
#include <thread>

namespace media = core::ubuntu::media;

TEST(PlayerStore, adding_players_from_multiple_threads_works)
{
    media::HashedKeyedPlayerStore store;
    media::Player::PlayerKey key = 0;

    static constexpr num_workers = 3;
    size_t i;
    std::vector<std::thread> workers;
    for (i=0; i<num_workers; i++)
    {
        workers.emplace_back(std::thread([&store, i, &key]()
        {
            const std::shared_ptr<media::Player> player;
            store.add_player_for_key(key, player);
            std::cout << "Added Player instance with key: " << key << std::endl;
            ++key;
        }));
    }

    for (i=0; i<workers.size(); i++)
        workers[i].join();

    ASSERT_EQ(store.number_of_players(), workers.size());
}
