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

    

    EXPECT_TRUE(session);
}
