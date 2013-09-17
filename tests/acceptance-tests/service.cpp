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
#include <com/ubuntu/music/track_list.h>

#include <gtest/gtest.h>

#include <condition_variable>
#include <functional>
#include <thread>

namespace music = com::ubuntu::music;

namespace
{
template<typename T>
struct WaitableStateTransition
{
    struct InitialState { T value; };
    struct FinalState { T value; };

    WaitableStateTransition(const InitialState& initial_state, const FinalState& expected_final_state)
            : expected_final_state(expected_final_state.value),
              last_state(initial_state.value)
    {
    }

    bool wait_for(const std::chrono::milliseconds& duration)
    {
        std::unique_lock<std::mutex> ul{mutex};

        while (last_state != expected_final_state)
        {
            auto status = cv.wait_for(ul, duration);

            if (status == std::cv_status::timeout)
                return false;
        }

        return true;
    }

    void trigger(const T& new_state)
    {   
        last_state = new_state;

        if (last_state == expected_final_state)
            cv.notify_all();
    }

    T expected_final_state;
    T last_state;
    std::mutex mutex;
    std::condition_variable cv;
};
}

TEST(MusicService, accessing_and_creating_a_session_works)
{
    auto service = music::Service::Client::instance();
    auto session = service->create_session();

    //EXPECT_TRUE(session);
}

TEST(MusicService, starting_playback_on_non_empty_playqueue_works)
{
    static const music::Track::UriType uri{"file:///tmp/test.mp3"};
    static const bool dont_make_current{false};

    auto service = music::Service::Client::instance();
    auto session = service->create_session();

    ASSERT_TRUE(session->can_play());
    ASSERT_TRUE(session->track_list()->is_editable());

    session->track_list()->append_track_with_uri(uri, dont_make_current);
    
    ASSERT_EQ(1, session->track_list()->size());
    
    WaitableStateTransition<music::Player::PlaybackStatus> state_transition(
    WaitableStateTransition<music::Player::PlaybackStatus>::InitialState{music::Player::stopped},
        WaitableStateTransition<music::Player::PlaybackStatus>::FinalState{music::Player::playing});

    session->on_playback_status_changed(
        std::bind(&WaitableStateTransition<music::Player::PlaybackStatus>::trigger,
                  std::ref(state_transition),
                  std::placeholders::_1));

    session->play();

    EXPECT_TRUE(state_transition.wait_for(std::chrono::milliseconds{500}));
}
