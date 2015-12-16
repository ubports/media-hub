/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include <gtest/gtest.h>

#include <cstdio>

#include <condition_variable>
#include <functional>
#include <iostream>
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
    timespec ts{ 0, static_cast<long int>(ms.count()) * 1000 * 1000};
    ::nanosleep(&ts, nullptr);
}
}

TEST(MediaService, move_track_in_tracklist_works)
{
    auto service = media::Service::Client::instance();
    auto session = service->create_session(media::Player::Client::default_configuration());
    EXPECT_TRUE(session != nullptr);

    std::vector<media::Track::Id> ids;

    auto tracklist = session->track_list();

    // Catch the new track Ids that are assigned each track that's added
    core::Connection c = tracklist->on_track_added().connect([&ids](const media::Track::Id &new_id)
    {
        std::cout << "track added: " << new_id << std::endl;
        ids.push_back(new_id);
    });

    const media::Track::UriType uri1{"file:///tmp/test-audio.ogg"};
    const media::Track::UriType uri2{"file:///tmp/test-audio1.ogg"};
    const media::Track::UriType uri3{"file:///tmp/test.mp3"};
    const media::Track::UriType uri4{"file:///tmp/test-video.ogg"};

    tracklist->add_track_with_uri_at(uri1, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri2, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri3, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri4, media::TrackList::after_empty_track(), false);
    EXPECT_EQ(std::uint32_t{4}, tracklist->tracks()->size());

    std::cout << "ids.at(0): " << ids.at(1) << std::endl;
    std::cout << "ids.at(2): " << ids.at(2) << std::endl;
    // Move track 3 into the second position in the TrackList
    tracklist->move_track(ids.at(2), ids.at(1));

    // Original list order and current order should be different
    EXPECT_NE(ids, tracklist->tracks().get());
    EXPECT_EQ(ids[1], tracklist->tracks()->at(2));
    EXPECT_EQ(ids[2], tracklist->tracks()->at(1));
}

TEST(MediaService, move_track_to_first_position_in_tracklist_works)
{
    auto service = media::Service::Client::instance();
    auto session = service->create_session(media::Player::Client::default_configuration());
    EXPECT_TRUE(session != nullptr);

    std::vector<media::Track::Id> ids;

    auto tracklist = session->track_list();

    // Catch the new track Ids that are assigned each track that's added
    core::Connection c = tracklist->on_track_added().connect([&ids](const media::Track::Id &new_id)
    {
        std::cout << "track added: " << new_id << std::endl;
        ids.push_back(new_id);
    });

    const media::Track::UriType uri1{"file:///tmp/test-audio.ogg"};
    const media::Track::UriType uri2{"file:///tmp/test-audio1.ogg"};
    const media::Track::UriType uri3{"file:///tmp/test.mp3"};
    const media::Track::UriType uri4{"file:///tmp/test-video.ogg"};

    tracklist->add_track_with_uri_at(uri1, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri2, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri3, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri4, media::TrackList::after_empty_track(), false);
    EXPECT_EQ(std::uint32_t{4}, tracklist->tracks()->size());

    std::cout << "ids.at(0): " << ids.at(0) << std::endl;
    std::cout << "ids.at(2): " << ids.at(2) << std::endl;
    // Move track 3 into the first position in the TrackList
    tracklist->move_track(ids.at(2), ids.at(0));

    // Original list order and current order should be different
    EXPECT_NE(ids, tracklist->tracks().get());
    EXPECT_EQ(ids[2], tracklist->tracks()->at(0));
    EXPECT_EQ(ids[0], tracklist->tracks()->at(1));
}

TEST(MediaService, move_track_to_last_position_in_tracklist_works)
{
    auto service = media::Service::Client::instance();
    auto session = service->create_session(media::Player::Client::default_configuration());
    EXPECT_TRUE(session != nullptr);

    std::vector<media::Track::Id> ids;

    auto tracklist = session->track_list();

    // Catch the new track Ids that are assigned each track that's added
    core::Connection c = tracklist->on_track_added().connect([&ids](const media::Track::Id &new_id)
    {
        std::cout << "track added: " << new_id << std::endl;
        ids.push_back(new_id);
    });

    const media::Track::UriType uri1{"file:///tmp/test-audio.ogg"};
    const media::Track::UriType uri2{"file:///tmp/test-audio1.ogg"};
    const media::Track::UriType uri3{"file:///tmp/test.mp3"};
    const media::Track::UriType uri4{"file:///tmp/test-video.ogg"};

    tracklist->add_track_with_uri_at(uri1, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri2, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri3, media::TrackList::after_empty_track(), false);
    tracklist->add_track_with_uri_at(uri4, media::TrackList::after_empty_track(), false);
    EXPECT_EQ(std::uint32_t{4}, tracklist->tracks()->size());

    sleep(1);

    std::cout << "ids.at(0): " << ids.at(0) << std::endl;
    std::cout << "ids.at(3): " << ids.at(3) << std::endl;
    // Move track 3 into the first position in the TrackList
    tracklist->move_track(ids.at(0), ids.at(3));

    // Original list order and current order should be different
    EXPECT_NE(ids, tracklist->tracks().get());
    EXPECT_EQ(ids[1], tracklist->tracks()->at(0));
    EXPECT_EQ(ids[0], tracklist->tracks()->at(3));
}
