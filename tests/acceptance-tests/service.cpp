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
#include <com/ubuntu/music/session.h>

#include <gtest/gtest.h>

TEST(MusicService, accessing_and_creating_a_session_works)
{
    auto service = com::ubuntu::music::Service::stub();
    auto session = service->start_session();

    EXPECT_TRUE(session);
}

TEST(MusicService, starting_playback_on_non_empty_playqueue_works)
{
    auto service = com::ubuntu::music::Service::stub();
    auto session = service->start_session();

    session->

    EXPECT_TRUE(session);
}
