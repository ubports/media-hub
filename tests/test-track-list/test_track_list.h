/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#ifndef TEST_TRACK_LIST_H_
#define TEST_TRACK_LIST_H_

#include <core/media/player.h>
#include <core/media/track.h>

#include <core/dbus/signal.h>

#include <chrono>
#include <memory>
#include <string>

namespace core
{
namespace ubuntu
{
namespace media
{

class Player;
class Service;
class TrackList;

class TestTrackList
{
public:
    TestTrackList();
    ~TestTrackList();

    void create_new_player_session();
    void destroy_player_session();

    void add_track(const std::string &uri, bool make_current = false);

    // Takes in one or two files for playback, adds it/them to the TrackList, and plays
    void test_basic_playback(const std::string &uri1, const std::string &uri2 = std::string{});

    // Takes in one or two files for playback, adds it/them to the TrackList, plays and makes sure
    // that the Player advances the TrackList
    void test_has_next_track(const std::string &uri1, const std::string &uri2);

protected:
    // Synchronously verify that a signal is emitted waiting up to timeout milliseconds
    template<class T>
    bool verify_signal_is_emitted(const core::Signal<T> &signal, const std::chrono::milliseconds &timeout);

    void wait_for_about_to_finish();
    void wait_for_end_of_stream();
    void wait_for_playback_status_changed(core::ubuntu::media::Player::PlaybackStatus status);
    core::ubuntu::media::Track::Id wait_for_on_track_added();

private:
    std::shared_ptr<core::ubuntu::media::Service> m_hubService;
    std::shared_ptr<core::ubuntu::media::Player> m_hubPlayerSession;
    std::shared_ptr<core::ubuntu::media::TrackList> m_hubTrackList;
};

} // media
} // ubuntu
} // core

#endif
