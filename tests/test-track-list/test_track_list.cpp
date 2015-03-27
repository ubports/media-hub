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

#include "test_track_list.h"

#include <core/media/service.h>
#include <core/media/track_list.h>

#include <cassert>
#include <iostream>

namespace media = core::ubuntu::media;
using namespace std;

media::TestTrackList::TestTrackList()
{
    try {
        m_hubService = media::Service::Client::instance();
        m_hubPlayerSession = m_hubService->create_session(media::Player::Client::default_configuration());
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to start a new media-hub player session: " << e.what() << endl;
    }

    try {
        m_hubTrackList = m_hubPlayerSession->track_list();
    }
    catch (std::runtime_error &e) {
        cerr << "FATAL: Failed to retrieve the current player's TrackList: " << e.what() << endl;
    }
}

void media::TestTrackList::add_track(const std::string uri)
{
    assert (m_hubTrackList.get() != nullptr);

    cout << "Adding " << uri << " to the TrackList for playback." << endl;

    try {
        bool can_edit_tracks = m_hubTrackList->can_edit_tracks();
        cout << "can_edit_tracks: " << (can_edit_tracks ? "yes" : "no") << std::endl;
        m_hubTrackList->add_track_with_uri_at(uri, media::TrackList::after_empty_track(), false);
    }
    catch (std::runtime_error &e) {
        cerr << "ERROR: Failed to add track " << uri << " to tracklist: " << e.what() << endl;
    }
}

int main (int argc, char **argv)
{
    shared_ptr<media::TestTrackList> tracklist = make_shared<media::TestTrackList>();

    if (argc == 2)
        tracklist->add_track(argv[1]);
    else if (argc == 3)
    {
        tracklist->add_track(argv[1]);
        tracklist->add_track(argv[2]);
    }
    else
    {
        cout << "Can't test TrackList, no track(s) specified." << endl;
        cout << argv[0] << " <track_uri_1> [<track_uri_2>]" << endl;
    }

    return 0;
}
