/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include <core/posix/fork.h>

#include "core/media/xesam.h"
#include "core/media/gstreamer/engine.h"
#include "core/media/gstreamer/meta_data_support.h"

#include "../test_data.h"
#include "../waitable_state_transition.h"
#include "web_server.h"

#include <gtest/gtest.h>

#include <cstdio>

#include <condition_variable>
#include <functional>
#include <thread>

namespace media = core::ubuntu::media;

struct EnsureFakeAudioSinkEnvVarIsSet
{
    EnsureFakeAudioSinkEnvVarIsSet()
    {
        ::setenv("CORE_UBUNTU_MEDIA_SERVICE_AUDIO_SINK_NAME", "fakesink", 1);
    }
} ensure_fake_audio_sink_env_var_is_set;

struct EnsureFakeVideoSinkEnvVarIsSet
{
    EnsureFakeVideoSinkEnvVarIsSet()
    {
        ::setenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME", "fakesink", 1);
    }
};

struct EnsureMirVideoSinkEnvVarIsSet
{
    EnsureMirVideoSinkEnvVarIsSet()
    {
        ::setenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME", "mirsink", 1);
    }
};

TEST(GStreamerEngine, construction_and_deconstruction_works)
{
    gstreamer::Engine engine{0};
}

TEST(GStreamerEngine, DISABLED_setting_uri_and_starting_audio_only_playback_works)
{
    const std::string test_file{"/tmp/test-audio.ogg"};
    const std::string test_file_uri{"file:///tmp/test-audio.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-audio.ogg", test_file));

    core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State> wst(
                core::ubuntu::media::Engine::State::ready);

    gstreamer::Engine engine{0};

    engine.track_meta_data().changed().connect(
                [](const std::tuple<media::Track::UriType, media::Track::MetaData>& md)
                {
                    if (0 < std::get<1>(md).count(xesam::Album::name))
                        EXPECT_EQ("Ezwa", std::get<1>(md).get(xesam::Album::name));
                    if (0 < std::get<1>(md).count(xesam::AlbumArtist::name))
                        EXPECT_EQ("Ezwa", std::get<1>(md).get(xesam::AlbumArtist::name));
                    if (0 < std::get<1>(md).count(xesam::Artist::name))
                        EXPECT_EQ("Ezwa", std::get<1>(md).get(xesam::Artist::name));
                    if (0 < std::get<1>(md).count(xesam::DiscNumber::name))
                        EXPECT_EQ("42", std::get<1>(md).get(xesam::DiscNumber::name));
                    if (0 < std::get<1>(md).count(xesam::Genre::name))
                        EXPECT_EQ("Test", std::get<1>(md).get(xesam::Genre::name));
                    if (0 < std::get<1>(md).count(xesam::TrackNumber::name))
                        EXPECT_EQ("42", std::get<1>(md).get(xesam::TrackNumber::name));
                });

    engine.state().changed().connect(
                std::bind(
                    &core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    static const bool do_pipeline_reset = false;
    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri, do_pipeline_reset));
    static const bool use_main_context = true;
    EXPECT_TRUE(engine.play(use_main_context));
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::seconds{4}));
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::ready,
                    std::chrono::seconds{10}));
}

TEST(GStreamerEngine, DISABLED_setting_uri_and_starting_video_playback_works)
{
    const std::string test_file{"/tmp/test-video.ogg"};
    const std::string test_file_uri{"file:///tmp/test-video.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-video.ogg", test_file));
    // Make sure a video sink is added to the pipeline
    const EnsureFakeVideoSinkEnvVarIsSet efs;

    core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State> wst(
                core::ubuntu::media::Engine::State::ready);

    gstreamer::Engine engine{0};

    engine.track_meta_data().changed().connect(
                [](const std::tuple<media::Track::UriType, media::Track::MetaData>& md)
                {
                    if (0 < std::get<1>(md).count(xesam::Album::name))
                        EXPECT_EQ("Test series", std::get<1>(md).get(xesam::Album::name));
                    if (0 < std::get<1>(md).count(xesam::Artist::name))
                        EXPECT_EQ("Canonical", std::get<1>(md).get(xesam::Artist::name));
                    if (0 < std::get<1>(md).count(xesam::Genre::name))
                        EXPECT_EQ("Documentary", std::get<1>(md).get(xesam::Genre::name));
                });

    engine.state().changed().connect(
                std::bind(
                    &core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    static const bool do_pipeline_reset = false;
    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri, do_pipeline_reset));
    static const bool use_main_context = true;
    EXPECT_TRUE(engine.play(use_main_context));
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::stopped,
                    std::chrono::seconds{10}));
}

TEST(GStreamerEngine, setting_uri_and_audio_playback_with_http_headers_works)
{
    const std::string test_file{"/tmp/test-audio-1.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-audio-1.ogg", test_file));

    const std::string test_audio_uri{"http://localhost:5000"};
    const core::ubuntu::media::Player::HeadersType headers{{ "User-Agent", "MediaHub" }, { "Cookie", "A=B;X=Y" }};

    // test server
    core::testing::CrossProcessSync cps; // server - ready -> client

    testing::web::server::Configuration configuration
    {
        5000,
        [test_file](mg_connection* conn)
        {
            std::map<std::string, std::set<std::string>> headers;
            for (int i = 0; i < conn->num_headers; ++i) {
              headers[conn->http_headers[i].name].insert(conn->http_headers[i].value);
            }

            EXPECT_TRUE(headers.at("User-Agent").count("MediaHub") == 1);
            EXPECT_TRUE(headers.at("Cookie").count("A=B") == 1);
            EXPECT_TRUE(headers.at("Cookie").count("X=Y") == 1);

            mg_send_file(conn, test_file.c_str(), 0);
            return MG_MORE;
        }
    };

    auto server = core::posix::fork(
                std::bind(testing::a_web_server(configuration), cps),
                core::posix::StandardStream::empty);
    cps.wait_for_signal_ready_for(std::chrono::seconds{2});
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    // test
    core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State> wst(
                core::ubuntu::media::Engine::State::ready);

    gstreamer::Engine engine{0};

    engine.state().changed().connect(
                std::bind(
                    &core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    EXPECT_TRUE(engine.open_resource_for_uri(test_audio_uri, headers));
    static const bool use_main_context = true;
    EXPECT_TRUE(engine.play(use_main_context));
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::seconds{10}));

    EXPECT_TRUE(engine.stop());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::stopped,
                    std::chrono::seconds{10}));
}

TEST(GStreamerEngine, DISABLED_stop_pause_play_seek_audio_only_works)
{
    const std::string test_file{"/tmp/test-audio.ogg"};
    const std::string test_file_uri{"file:///tmp/test-audio.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-audio.ogg", test_file));

    core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State> wst(
                core::ubuntu::media::Engine::State::ready);

    gstreamer::Engine engine{0};

    engine.state().changed().connect(
                std::bind(
                    &core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    static const bool do_pipeline_reset = true;
    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri, do_pipeline_reset));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));
    EXPECT_TRUE(engine.stop());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::stopped,
                    std::chrono::milliseconds{4000}));
    EXPECT_TRUE(engine.pause());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::paused,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{10}));
    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{0}));
    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{25}));

    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::ready,
                    std::chrono::seconds{40}));
}

TEST(GStreamerEngine, DISABLED_stop_pause_play_seek_video_works)
{
    const std::string test_file{"/tmp/test-video.ogg"};
    const std::string test_file_uri{"file:///tmp/test-video.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-video.ogg", test_file));
    // Make sure a video sink is added to the pipeline
    const EnsureFakeVideoSinkEnvVarIsSet efs;

    core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State> wst(
                core::ubuntu::media::Engine::State::ready);

    gstreamer::Engine engine{0};

    engine.state().changed().connect(
                std::bind(
                    &core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    static const bool do_pipeline_reset = true;
    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri, do_pipeline_reset));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));
    EXPECT_TRUE(engine.stop());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::stopped,
                    std::chrono::milliseconds{4000}));
    EXPECT_TRUE(engine.pause());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::paused,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{10}));
    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{0}));
    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{25}));

    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::ready,
                    std::chrono::seconds{40}));
}

TEST(GStreamerEngine, get_position_duration_work)
{
    const std::string test_file{"/tmp/test-video.ogg"};
    const std::string test_file_uri{"file:///tmp/test-video.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-video.ogg", test_file));

    core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State> wst(
                core::ubuntu::media::Engine::State::ready);

    gstreamer::Engine engine{0};

    engine.state().changed().connect(
                std::bind(
                    &core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));

    static const bool do_pipeline_reset = true;
    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri, do_pipeline_reset));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    EXPECT_TRUE(engine.seek_to(std::chrono::seconds{10}));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    std::cout << "position: " << engine.position() << std::endl;
    std::cout << "duration: " << engine.duration() << std::endl;

    EXPECT_TRUE(engine.position() > 9e9);
    EXPECT_TRUE(engine.duration() > 9e9);
}

TEST(GStreamerEngine, adjusting_volume_works)
{
    const std::string test_file{"/tmp/test-audio-1.ogg"};
    const std::string test_file_uri{"file:///tmp/test-audio-1.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-audio-1.ogg", test_file));

    core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State> wst(
                core::ubuntu::media::Engine::State::ready);

    gstreamer::Engine engine{0};
    engine.state().changed().connect(
                std::bind(
                    &core::testing::WaitableStateTransition<core::ubuntu::media::Engine::State>::trigger,
                    std::ref(wst),
                    std::placeholders::_1));
    static const bool do_pipeline_reset = true;
    EXPECT_TRUE(engine.open_resource_for_uri(test_file_uri, do_pipeline_reset));
    EXPECT_TRUE(engine.play());
    EXPECT_TRUE(wst.wait_for_state_for(
                    core::ubuntu::media::Engine::State::playing,
                    std::chrono::milliseconds{4000}));

    std::thread t([&]()
    {
        for(unsigned i = 0; i < 100; i++)
        {
            for (double v = 0.; v < 1.1; v += 0.1)
            {
                try
                {
                    media::Engine::Volume volume{v};
                    engine.volume() = volume;
                    EXPECT_EQ(volume, engine.volume());
                } catch(...)
                {
                }
            }
        }
    });

    if (t.joinable())
        t.join();
}

TEST(GStreamerEngine, provides_non_null_meta_data_extractor)
{
    gstreamer::Engine engine{0};
    EXPECT_NE(nullptr, engine.meta_data_extractor());
}

TEST(GStreamerEngine, meta_data_extractor_provides_correct_tags)
{
    const std::string test_file{"/tmp/test.mp3"};
    const std::string test_file_uri{"file:///tmp/test.mp3"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test.mp3", test_file));

    gstreamer::Engine engine{0};

    core::ubuntu::media::Track::MetaData md;
    ASSERT_NO_THROW({
        try {
        md = engine.meta_data_extractor()->meta_data_for_track_with_uri(test_file_uri);
	} catch (const std::runtime_error& error) {
            std::cerr << "runtime_error while getting meta data: " << error.what() << std::endl;
            // as the audio is not being played but just being parsed this runtime error is 
            // expected. see meta_data_extractor.h: meta_data_for_track_with_uri
            const std::string rtem{"Problem extracting meta data for track"};
            if(rtem.compare(error.what()) != 0)
                throw;
        } catch (const std::exception& e) {
            std::cerr << "exception while getting meta data: " << e.what() << std::endl;
            throw;
        }
    });

    if (0 < md.count(xesam::Album::name))
        EXPECT_EQ("Test", md.get(xesam::Album::name));
    if (0 < md.count(xesam::AlbumArtist::name))
        EXPECT_EQ("Test", md.get(xesam::AlbumArtist::name));
    if (0 < md.count(xesam::Artist::name))
        EXPECT_EQ("Ezwa", md.get(xesam::Artist::name));
    if (0 < md.count(xesam::DiscNumber::name))
        EXPECT_EQ("1", md.get(xesam::DiscNumber::name));
    if (0 < md.count(xesam::Genre::name))
        EXPECT_EQ("Test", md.get(xesam::Genre::name));
    if (0 < md.count(xesam::TrackNumber::name))
        EXPECT_EQ("42", md.get(xesam::TrackNumber::name));
}

TEST(GStreamerEngine, meta_data_extractor_supports_embedded_album_art)
{
    const std::string test_file{"/tmp/test-audio.ogg"};
    std::remove(test_file.c_str());
    ASSERT_TRUE(test::copy_test_media_file_to("test-audio.ogg", test_file));
    const std::string test_audio_uri{"http://localhost:5000"};
    //const std::string test_audio_uri{"http://www.wndbz.nl/ubuntu-touch/test-audio.ogg"};
    //const std::string test_audio_uri{"file:///tmp/test-audio.ogg"};

    // test server
    core::testing::CrossProcessSync cps;
    testing::web::server::Configuration configuration
    {
        5000,
        [test_file](mg_connection* conn)
        {
            std::cerr << "test server will call mg_send_file" << std::endl;
            mg_send_file(conn, test_file.c_str(), 0);
            return MG_MORE;
        }
    };
    auto server = core::posix::fork(
                std::bind(testing::a_web_server(configuration), cps),
                core::posix::StandardStream::empty);
    cps.wait_for_signal_ready_for(std::chrono::seconds{2});
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    // test
    core::ubuntu::media::Track::MetaData md;
    gstreamer::Engine engine{0};

    ASSERT_NO_THROW({
        try {
        md = engine.meta_data_extractor()->meta_data_for_track_with_uri(test_audio_uri);
	} catch (const std::runtime_error& error) {
            std::cerr << "runtime_error while getting meta data: " << error.what() << std::endl;
            // as the audio is not being played but just being parsed this runtime error is 
            // expected. see meta_data_extractor.h: meta_data_for_track_with_uri
            const std::string rtem{"Problem extracting meta data for track"};
            if(rtem.compare(error.what()) != 0)
                throw;
        } catch (const std::exception& e) {
            std::cerr << "exception while getting meta data: " << e.what() << std::endl;
            throw;
        }
    });

    // old tags still ok?
    if (0 < md.count(xesam::Album::name))
        EXPECT_EQ("Test", md.get(xesam::Album::name));
    if (0 < md.count(xesam::Artist::name))
        EXPECT_EQ("Test", md.get(xesam::Artist::name));
    if (0 < md.count(xesam::AlbumArtist::name))
        EXPECT_EQ("Test", md.get(xesam::AlbumArtist::name));
    if (0 < md.count(xesam::Genre::name))
        EXPECT_EQ("Test", md.get(xesam::Genre::name));

    // found and handled embedded album art
    EXPECT_TRUE(md.has_embedded_album_art());
    EXPECT_EQ(0x9599, md.embeddedAlbumArtCRC);
    ASSERT_TRUE(test::file_exists(md.embeddedAlbumArtFileName));

    // does it clean it up?
    gstreamer::MetaDataSupport::cleanupTempAlbumArtFile(md);
    ASSERT_FALSE(test::file_exists(md.embeddedAlbumArtFileName));
}
