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

#include <core/media/service.h>
#include <core/media/player.h>
#include <core/media/track_list.h>

#include "core/media/service_implementation.h"

#include "../cross_process_sync.h"
#include "../fork_and_run.h"
#include "../waitable_state_transition.h"

#include "test_data.h"

#include <gtest/gtest.h>

#include <cstdio>

#include <condition_variable>
#include <functional>
#include <thread>

namespace media = core::ubuntu::media;

namespace
{
void sleep_for(const std::chrono::milliseconds& ms)
{
    timespec ts{ 0, static_cast<long int>(ms.count()) * 1000 * 1000};
    ::nanosleep(&ts, nullptr);
}
}

TEST(MusicService, accessing_and_creating_a_session_works)
{
    test::CrossProcessSync sync_service_start;

    auto service = [&sync_service_start]()
    {
        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.signal_ready();

        if (t.joinable())
            t.join();
    };

    auto client = [&sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready();

        auto service = media::Service::Client::instance();
        auto session = service->create_session(media::Player::Client::default_configuration());

        EXPECT_TRUE(session != nullptr);
    };

    ASSERT_NO_FATAL_FAILURE(test::fork_and_run(service, client));
}

TEST(MusicService, remotely_querying_track_meta_data_works)
{
    const std::string test_file{"/tmp/test.ogg"};
    const std::string test_file_uri{"file:///tmp/test.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_ogg_file_to(test_file));

    test::CrossProcessSync sync_service_start;

    auto service = [&sync_service_start]()
    {
        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.signal_ready();

        if (t.joinable())
            t.join();
    };

    auto client = [&sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready();

        static const media::Track::UriType uri{"file:///tmp/test.ogg"};

        auto service = media::Service::Client::instance();
        auto session = service->create_session(media::Player::Client::default_configuration());
        auto track_list = session->track_list();

        track_list->add_track_with_uri_at(uri, media::TrackList::after_empty_track(), false);

        EXPECT_EQ(std::uint32_t{1}, track_list->tracks()->size());

        auto md = track_list->query_meta_data_for_track(track_list->tracks()->front());

        for (auto pair : *md)
        {
            std::cout << pair.first << " -> " << pair.second << std::endl;
        }
    };

    ASSERT_NO_FATAL_FAILURE(test::fork_and_run(service, client));
}

TEST(MusicService, DISABLED_play_pause_seek_after_open_uri_works)
{
    const std::string test_file{"/tmp/test.mp3"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_mp3_file_to(test_file));

    test::CrossProcessSync sync_service_start;

    auto service = [&sync_service_start]()
    {
        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.signal_ready();

        if (t.joinable())
            t.join();
    };

    auto client = [&sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready();

        static const media::Track::UriType uri{"file:///tmp/test.mp3"};

        auto service = media::Service::Client::instance();
        auto session = service->create_session(media::Player::Client::default_configuration());

        ASSERT_EQ(true, session->can_play().get());
        ASSERT_EQ(true, session->can_pause().get());
        ASSERT_EQ(true, session->can_seek().get());
        ASSERT_EQ(true, session->can_go_previous().get());
        ASSERT_EQ(true, session->can_go_next().get());
        ASSERT_EQ(media::Player::PlaybackStatus::null, session->playback_status());
        ASSERT_EQ(media::Player::LoopStatus::none, session->loop_status());
        ASSERT_NEAR(1.f, session->playback_rate().get(), 1E-5);
        ASSERT_EQ(true, session->is_shuffle());
        ASSERT_EQ(true, session->track_list()->can_edit_tracks().get());

        EXPECT_TRUE(session->open_uri(uri));

        test::WaitableStateTransition<media::Player::PlaybackStatus>
                state_transition(
                    media::Player::stopped);

        session->playback_status().changed().connect(
            std::bind(&test::WaitableStateTransition<media::Player::PlaybackStatus>::trigger,
                      std::ref(state_transition),
                      std::placeholders::_1));

        session->play();
        sleep_for(std::chrono::seconds{1});
        ASSERT_EQ(media::Player::PlaybackStatus::playing, session->playback_status());
        session->stop();
        sleep_for(std::chrono::seconds{1});
        ASSERT_EQ(media::Player::PlaybackStatus::stopped, session->playback_status());
        session->play();
        sleep_for(std::chrono::seconds{1});
        ASSERT_EQ(media::Player::PlaybackStatus::playing, session->playback_status());
        session->pause();
        sleep_for(std::chrono::seconds{1});
        ASSERT_EQ(media::Player::PlaybackStatus::paused, session->playback_status());
    };

    ASSERT_NO_FATAL_FAILURE(test::fork_and_run(service, client));
}


TEST(MusicService, DISABLED_starting_playback_on_non_empty_playqueue_works)
{
    const std::string test_file{"/tmp/test.mp3"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_mp3_file_to(test_file));

    test::CrossProcessSync sync_service_start;

    auto service = [&sync_service_start]()
    {
        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.signal_ready();

        if (t.joinable())
            t.join();
    };

    auto client = [&sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready();

        static const media::Track::UriType uri{"file:///tmp/test.mp3"};
        static const bool dont_make_current{false};

        auto service = media::Service::Client::instance();
        auto session = service->create_session(media::Player::Client::default_configuration());

        ASSERT_EQ(true, session->can_play().get());
        ASSERT_EQ(true, session->can_play().get());
        ASSERT_EQ(true, session->can_pause().get());
        ASSERT_EQ(true, session->can_seek().get());
        ASSERT_EQ(true, session->can_go_previous().get());
        ASSERT_EQ(true, session->can_go_next().get());
        ASSERT_EQ(media::Player::PlaybackStatus::null, session->playback_status());
        ASSERT_EQ(media::Player::LoopStatus::none, session->loop_status());
        ASSERT_NEAR(1.f, session->playback_rate().get(), 1E-5);
        ASSERT_EQ(true, session->is_shuffle());
        ASSERT_EQ(true, session->track_list()->can_edit_tracks().get());

        session->track_list()->add_track_with_uri_at(uri, media::TrackList::after_empty_track(), dont_make_current);

        EXPECT_EQ(std::uint32_t{1}, session->track_list()->tracks()->size());

        test::WaitableStateTransition<media::Player::PlaybackStatus>
                state_transition(
                    media::Player::stopped);

        session->playback_status().changed().connect(
            std::bind(&test::WaitableStateTransition<media::Player::PlaybackStatus>::trigger,
                      std::ref(state_transition),
                      std::placeholders::_1));

        session->play();

        EXPECT_TRUE(state_transition.wait_for_state_for(
                        media::Player::playing,
                        std::chrono::milliseconds{500}));
    };

    ASSERT_NO_FATAL_FAILURE(test::fork_and_run(service, client));
}
