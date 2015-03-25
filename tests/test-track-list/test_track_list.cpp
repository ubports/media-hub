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
        cout << "FATAL: Failed to start a new media-hub player session: " << e.what() << endl;
    }
}

int main (int argc, char **argv)
{
    shared_ptr<media::TestTrackList> test_tl = make_shared<media::TestTrackList>();
    return 0;
}
