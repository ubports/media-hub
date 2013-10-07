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

#include "com/ubuntu/music/gstreamer/engine.h"

#include "../test_data.h"
#include "../waitable_state_transition.h"

#include <gtest/gtest.h>

#include <cstdio>

#include <condition_variable>
#include <functional>
#include <thread>

namespace music = com::ubuntu::music;

TEST(GStreamerEngine, construction_and_deconstruction_works)
{
    gstreamer::Engine engine;
}

TEST(GStreamerEngine, setting_uri_and_starting_playback_works)
{
    const std::string test_file{"/tmp/test.ogg"};
    const std::string test_file_uri{"file:///tmp/test.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_ogg_file_to(test_file));

    test::WaitableStateTransition<com::ubuntu::music::Engine::State> wst(
                com::ubuntu::music::Engine::State::ready);

    gstreamer::Engine engine;

    engine.track_meta_data().changed().connect(
                [](const std::tuple<music::Track::UriType, music::Track::MetaData>& md)
                {
                    if (0 < std::get<1>(md).count(music::Engine::Xesam::album()))
                        EXPECT_EQ("Test", std::get<1>(md).at(music::Engine::Xesam::album()));
                    if (0 < std::get<1>(md).count(music::Engine::Xesam::album_artist()))
                        EXPECT_EQ("Test", std::get<1>(md).at(music::Engine::Xesam::album_artist()));
                    if (0 < std::get<1>(md).count(music::Engine::Xesam::artist()))
                        EXPECT_EQ("Test", std::get<1>(md).at(music::Engine::Xesam::artist()));
                    if (0 < std::get<1>(md).count(music::Engine::Xesam::disc_number()))
                        EXPECT_EQ("42", std::get<1>(md).at(music::Engine::Xesam::disc_number()));
                    if (0 < std::get<1>(md).count(music::Engine::Xesam::genre()))
                        EXPECT_EQ("Test", std::get<1>(md).at(music::Engine::Xesam::genre()));
                    if (0 < std::get<1>(md).count(music::Engine::Xesam::track_number()))
                        EXPECT_EQ("42", std::get<1>(md).at(music::Engine::Xesam::track_number()));
                });

    engine.state().changed().connect(
                std::bind(
                    &test::WaitableStateTransition<com::ubuntu::music::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::ready,
                    std::chrono::seconds{10}));
}

TEST(GStreamerEngine, stop_pause_play_seek_works)
{
    const std::string test_file{"/tmp/test.mp3"};
    const std::string test_file_uri{"file:///tmp/test.mp3"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_mp3_file_to(test_file));

    test::WaitableStateTransition<com::ubuntu::music::Engine::State> wst(
                com::ubuntu::music::Engine::State::ready);

    gstreamer::Engine engine;

    engine.state().changed().connect(
                std::bind(
                    &test::WaitableStateTransition<com::ubuntu::music::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::playing,
                    std::chrono::milliseconds{4000}));
    EXPECT_TRUE(engine.stop());
    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::stopped,
                    std::chrono::milliseconds{4000}));
    EXPECT_TRUE(engine.pause());
    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::paused,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{10}));
    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{0}));
    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{25}));

    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::ready,
                    std::chrono::seconds{40}));
}

TEST(GStreamerEngine, adjusting_volume_works)
{
    const std::string test_file{"/tmp/test.mp3"};
    const std::string test_file_uri{"file:///tmp/test.mp3"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_mp3_file_to(test_file));

    test::WaitableStateTransition<com::ubuntu::music::Engine::State> wst(
                com::ubuntu::music::Engine::State::ready);

    gstreamer::Engine engine;

    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    com::ubuntu::music::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    music::Engine::Volume half{0.5};
    engine.volume().set(music::Engine::Volume{0.5});
    EXPECT_EQ(half, engine.volume());
}
