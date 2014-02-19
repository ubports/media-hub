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

#include "../waitable_state_transition.h"

#include "test_data.h"

#include <core/testing/cross_process_sync.h>
#include <core/testing/fork_and_run.h>

#include <gtest/gtest.h>

#include <cstdio>

#include <condition_variable>
#include <functional>
#include <thread>

namespace media = core::ubuntu::media;

namespace
{
struct SigTermCatcher
{
    inline SigTermCatcher()
    {
        sigemptyset(&signal_mask);

        if (-1 == sigaddset(&signal_mask, SIGTERM))
            throw std::system_error(errno, std::system_category());

        if (-1 == sigprocmask(SIG_BLOCK, &signal_mask, NULL))
            throw std::system_error(errno, std::system_category());
    }

    inline void wait_for_signal()
    {
        int signal = -1;
        ::sigwait(&signal_mask, &signal);
    }

    sigset_t signal_mask;
};

void sleep_for(const std::chrono::milliseconds& ms)
{
    long int ns = ms.count() * 1000 * 1000;
    timespec ts{ 0, ns};
    ::nanosleep(&ts, nullptr);
}
}

TEST(MusicService, DISABLED_accessing_and_creating_a_session_works)
{
    core::testing::CrossProcessSync sync_service_start;

    auto service = [this, &sync_service_start]()
    {
        SigTermCatcher sc;

        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.try_signal_ready_for(std::chrono::milliseconds{500});

        sc.wait_for_signal(); service->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, &sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        auto service = media::Service::Client::instance();
        auto session = service->create_session(media::Player::Client::default_configuration());

        EXPECT_TRUE(session != nullptr);

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty,
              core::testing::fork_and_run(service, client));
}

TEST(MusicService, DISABLED_remotely_querying_track_meta_data_works)
{
    const std::string test_file{"/tmp/test.ogg"};
    const std::string test_file_uri{"file:///tmp/test.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_ogg_file_to(test_file));

    core::testing::CrossProcessSync sync_service_start;

    auto service = [this, &sync_service_start]()
    {
        SigTermCatcher sc;

        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.try_signal_ready_for(std::chrono::milliseconds{500});

        sc.wait_for_signal(); service->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, &sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready_for(std::chrono::milliseconds{500});

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

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty,
              core::testing::fork_and_run(service, client));
}

TEST(MusicService, DISABLED_play_pause_seek_after_open_uri_works)
{
    const std::string test_file{"/tmp/test.mp3"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_mp3_file_to(test_file));

    core::testing::CrossProcessSync sync_service_start;

    auto service = [this, &sync_service_start]()
    {
        SigTermCatcher sc;

        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.try_signal_ready_for(std::chrono::milliseconds{500});

        sc.wait_for_signal(); service->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, &sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        static const media::Track::UriType uri{"file:///tmp/test.mp3"};

        auto service = media::Service::Client::instance();
        auto session = service->create_session(media::Player::Client::default_configuration());

        EXPECT_EQ(true, session->can_play().get());
        EXPECT_EQ(true, session->can_pause().get());
        EXPECT_EQ(true, session->can_seek().get());
        EXPECT_EQ(true, session->can_go_previous().get());
        EXPECT_EQ(true, session->can_go_next().get());
        EXPECT_EQ(media::Player::PlaybackStatus::null, session->playback_status());
        EXPECT_EQ(media::Player::LoopStatus::none, session->loop_status());
        EXPECT_NEAR(1.f, session->playback_rate().get(), 1E-5);
        EXPECT_EQ(true, session->is_shuffle());
        EXPECT_EQ(true, session->track_list()->can_edit_tracks().get());

        EXPECT_TRUE(session->open_uri(uri));

        core::testing::WaitableStateTransition<media::Player::PlaybackStatus>
                state_transition(
                    media::Player::stopped);

        session->playback_status().changed().connect(
            std::bind(&core::testing::WaitableStateTransition<media::Player::PlaybackStatus>::trigger,
                      std::ref(state_transition),
                      std::placeholders::_1));

        session->play();
        sleep_for(std::chrono::seconds{1});
        EXPECT_EQ(media::Player::PlaybackStatus::playing, session->playback_status());
        session->stop();
        sleep_for(std::chrono::seconds{1});
        EXPECT_EQ(media::Player::PlaybackStatus::stopped, session->playback_status());
        session->play();
        sleep_for(std::chrono::seconds{1});
        EXPECT_EQ(media::Player::PlaybackStatus::playing, session->playback_status());
        session->pause();
        sleep_for(std::chrono::seconds{1});
        EXPECT_EQ(media::Player::PlaybackStatus::paused, session->playback_status());
        session->stop();
        sleep_for(std::chrono::seconds{1});
        EXPECT_EQ(media::Player::PlaybackStatus::stopped, session->playback_status());

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty,
              core::testing::fork_and_run(service, client));
}


TEST(MusicService, DISABLED_starting_playback_on_non_empty_playqueue_works)
{
    const std::string test_file{"/tmp/test.mp3"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_mp3_file_to(test_file));

    core::testing::CrossProcessSync sync_service_start;

    auto service = [this, &sync_service_start]()
    {
        SigTermCatcher sc;

        auto service = std::make_shared<media::ServiceImplementation>();
        std::thread t([&service](){service->run();});

        sync_service_start.try_signal_ready_for(std::chrono::milliseconds{500});

        sc.wait_for_signal(); service->stop();

        if (t.joinable())
            t.join();

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [this, &sync_service_start]()
    {
        sync_service_start.wait_for_signal_ready_for(std::chrono::milliseconds{500});

        static const media::Track::UriType uri{"file:///tmp/test.mp3"};
        static const bool dont_make_current{false};

        auto service = media::Service::Client::instance();
        auto session = service->create_session(media::Player::Client::default_configuration());

        EXPECT_EQ(true, session->can_play().get());
        EXPECT_EQ(true, session->can_play().get());
        EXPECT_EQ(true, session->can_pause().get());
        EXPECT_EQ(true, session->can_seek().get());
        EXPECT_EQ(true, session->can_go_previous().get());
        EXPECT_EQ(true, session->can_go_next().get());
        EXPECT_EQ(media::Player::PlaybackStatus::null, session->playback_status());
        EXPECT_EQ(media::Player::LoopStatus::none, session->loop_status());
        EXPECT_NEAR(1.f, session->playback_rate().get(), 1E-5);
        EXPECT_EQ(true, session->is_shuffle());
        EXPECT_EQ(true, session->track_list()->can_edit_tracks().get());

        session->track_list()->add_track_with_uri_at(uri, media::TrackList::after_empty_track(), dont_make_current);

        EXPECT_EQ(std::uint32_t{1}, session->track_list()->tracks()->size());

        core::testing::WaitableStateTransition<media::Player::PlaybackStatus>
                state_transition(
                    media::Player::stopped);

        session->playback_status().changed().connect(
            std::bind(&core::testing::WaitableStateTransition<media::Player::PlaybackStatus>::trigger,
                      std::ref(state_transition),
                      std::placeholders::_1));

        session->play();

        EXPECT_TRUE(state_transition.wait_for_state_for(
                        media::Player::playing,
                        std::chrono::milliseconds{2000}));

        return ::testing::Test::HasFailure() ? core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty,
              core::testing::fork_and_run(service, client));
}
