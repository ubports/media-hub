/*
 * Copyright © 2013 Canonical Ltd.
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

#include <com/ubuntu/music/service.h>
#include <com/ubuntu/music/player.h>
#include <com/ubuntu/music/property.h>
#include <com/ubuntu/music/track_list.h>

#include "com/ubuntu/music/service_implementation.h"

#include "../cross_process_sync.h"
#include "../fork_and_run.h"
#include "../waitable_state_transition.h"

#include "test_data.h"

#include <gtest/gtest.h>

#include <cstdio>

#include <condition_variable>
#include <functional>
#include <thread>

namespace music = com::ubuntu::music;



/*TEST(MusicService, accessing_and_creating_a_session_works)
{
    test::CrossProcessSync sync_service_start;

    auto service = [&sync_service_start]()
    {
        auto service = std::make_shared<music::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.signal_ready();

        if (t.joinable())
            t.join();
    };

    auto client = [&sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready();

        auto service = music::Service::Client::instance();
        auto session = service->create_session(music::Player::Client::default_configuration());

        EXPECT_TRUE(session != nullptr);
    };

    ASSERT_NO_FATAL_FAILURE(test::fork_and_run(service, client));
}*/

TEST(MusicService, starting_playback_on_non_empty_playqueue_works)
{
    const std::string test_file{"/tmp/test.mp3"};
    std::remove(test_file.c_str()); ASSERT_TRUE(test::copy_test_mp3_file_to(test_file));

    test::CrossProcessSync sync_service_start;

    auto service = [&sync_service_start]()
    {
        auto service = std::make_shared<music::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.signal_ready();

        if (t.joinable())
            t.join();
    };

    auto client = [&sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready();

        static const music::Track::UriType uri{"file:///tmp/test.mp3"};
        static const bool dont_make_current{false};

        auto service = music::Service::Client::instance();
        auto session = service->create_session(music::Player::Client::default_configuration());

        ASSERT_EQ(true, session->can_play().get());
        ASSERT_EQ(true, session->can_play().get());
        ASSERT_EQ(true, session->can_pause().get());
        ASSERT_EQ(true, session->can_seek().get());
        ASSERT_EQ(true, session->can_go_previous().get());
        ASSERT_EQ(true, session->can_go_next().get());
        ASSERT_EQ(music::Player::PlaybackStatus::stopped, session->playback_status());
        ASSERT_EQ(music::Player::LoopStatus::none, session->loop_status());
        ASSERT_NEAR(1.f, session->playback_rate().get(), 1E-5);
        ASSERT_EQ(true, session->is_shuffle());
        ASSERT_EQ(true, session->track_list()->can_edit_tracks().get());

        session->track_list()->add_track_with_uri_at(uri, music::TrackList::after_empty_track(), dont_make_current);

        EXPECT_EQ(1, session->track_list()->tracks()->size());

        test::WaitableStateTransition<music::Player::PlaybackStatus>
                state_transition(
                    music::Player::stopped);

        session->playback_status().changed().connect(
            std::bind(&test::WaitableStateTransition<music::Player::PlaybackStatus>::trigger,
                      std::ref(state_transition),
                      std::placeholders::_1));

        session->play();

        EXPECT_TRUE(state_transition.wait_for_state_for(music::Player::playing, std::chrono::milliseconds{500}));
    };

    ASSERT_NO_FATAL_FAILURE(test::fork_and_run(service, client));
}
