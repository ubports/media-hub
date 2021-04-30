/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
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
 */

#ifndef MPRIS_H_
#define MPRIS_H_

#include <string>
#include <tuple>
#include <vector>

#include <cstdint>

namespace mpris {

struct Service
{
    static const QString &name()
    {
        static const QString s{"core.ubuntu.media.Service"};
        return s;
    }

    struct Errors
    {
        struct CreatingSession
        {
            static const QString &name()
            {
                static const QString s
                {
                    "core.ubuntu.media.Service.Error.CreatingSession"
                };
                return s;
            }
        };

        struct DetachingSession
        {
            static const QString &name()
            {
                static const QString s
                {
                    "core.ubuntu.media.Service.Error.DetachingSession"
                };
                return s;
            }
        };

        struct ReattachingSession
        {
            static const QString &name()
            {
                static const QString s
                {
                    "core.ubuntu.media.Service.Error.ReattachingSession"
                };
                return s;
            }
        };

        struct DestroyingSession
        {
            static const QString &name()
            {
                static const QString s
                {
                    "core.ubuntu.media.Service.Error.DestroyingSession"
                };
                return s;
            }
        };

        struct CreatingFixedSession
        {
            static const QString &name()
            {
                static const QString s
                {
                    "core.ubuntu.media.Service.Error.CreatingFixedSession"
                };
                return s;
            }
        };

        struct ResumingSession
        {
            static const QString &name()
            {
                static const QString s
                {
                    "core.ubuntu.media.Service.Error.ResumingSession"
                };
                return s;
            }
        };

        struct PlayerKeyNotFound
        {
            static const QString &name()
            {
                static const QString s
                {
                    "core.ubuntu.media.Service.Error.PlayerKeyNotFound"
                };
                return s;
            }
        };
    };

};

struct Player
{
    static const QString &name()
    {
        static const QString s{"org.mpris.MediaPlayer2.Player"};
        return s;
    }

    struct LoopStatus
    {
        LoopStatus() = delete;

        static const char* from(core::ubuntu::media::Player::LoopStatus status)
        {
            switch(status)
            {
            case core::ubuntu::media::Player::LoopStatus::none:
                return LoopStatus::none;
            case core::ubuntu::media::Player::LoopStatus::track:
                return LoopStatus::track;
            case core::ubuntu::media::Player::LoopStatus::playlist:
                return LoopStatus::playlist;
            }

            return nullptr;
        }

        static constexpr const char* none{"None"};
        static constexpr const char* track{"Track"};
        static constexpr const char* playlist{"Playlist"};
    };

    struct PlaybackStatus
    {
        PlaybackStatus() = delete;

        static const char* from(core::ubuntu::media::Player::PlaybackStatus status)
        {
            switch(status)
            {
            case core::ubuntu::media::Player::PlaybackStatus::null:
            case core::ubuntu::media::Player::PlaybackStatus::ready:
            case core::ubuntu::media::Player::PlaybackStatus::stopped:
                return PlaybackStatus::stopped;

            case core::ubuntu::media::Player::PlaybackStatus::playing:
                return PlaybackStatus::playing;
            case core::ubuntu::media::Player::PlaybackStatus::paused:
                return PlaybackStatus::paused;
            }

            return nullptr;
        }

        static constexpr const char* playing{"Playing"};
        static constexpr const char* paused{"Paused"};
        static constexpr const char* stopped{"Stopped"};
    };

    struct Error
    {
        struct OutOfProcessBufferStreamingNotSupported
        {
            static constexpr const char* name
            {
                "mpris.Player.Error.OutOfProcessBufferStreamingNotSupported"
            };
        };

        struct InsufficientAppArmorPermissions
        {
            static constexpr const char* name
            {
                "mpris.Player.Error.InsufficientAppArmorPermissions"
            };
        };

        struct UriNotFound
        {
            static constexpr const char* name
            {
                "mpris.Player.Error.UriNotFound"
            };
        };
    };
};

struct TrackList
{
    static const std::string& name()
    {
        static const std::string s{"org.mpris.MediaPlayer2.TrackList"};
        return s;
    }

    struct Error
    {
        struct InsufficientPermissionsToAddTrack
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.InsufficientPermissionsToAddTrack"
            };
        };

        struct FailedToMoveTrack
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.FailedToMoveTrack"
            };
        };

        struct FailedToFindMoveTrackSource
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.FailedToFindMoveTrackSource"
            };
        };

        struct FailedToFindMoveTrackDest
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.FailedToFindMoveTrackDest"
            };
        };

        struct TrackNotFound
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.TrackNotFound"
            };
        };
    };
};

}

#endif // MPRIS_PLAYER_H_
