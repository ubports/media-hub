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
#ifndef CORE_UBUNTU_MEDIA_PLAYER_H_
#define CORE_UBUNTU_MEDIA_PLAYER_H_

#include <QMap>
#include <QMetaType>
#include <QString>

#include <cstdint>
#include <stdexcept>

namespace core {
namespace ubuntu {
namespace media {

class Service;
class TrackListImplementation;

struct AVBackend
{
    enum Backend
    {
        none,
        hybris,
        mir
    };

    /**
     * @brief Returns the type of audio/video decoding/encoding backend being used.
     * @return Returns the current backend type.
     */
    static Backend get_backend_type();
};

class Player
{
public:
    typedef double PlaybackRate;
    typedef double Volume;
    typedef uint32_t PlayerKey;
    typedef void* GLConsumerWrapperHybris;
    typedef QMap<QString,QString> HeadersType;

    static const PlayerKey invalidKey = 0xffffffff;

    struct Client {
        PlayerKey key;
        QString name; // D-Bus unique connection ID

        bool operator==(const Client &o) const {
            return key == o.key && name == o.name;
        }
    };

    struct Errors
    {
        Errors() = delete;

        struct ExceptionBase: public std::runtime_error {
            ExceptionBase(const QString &msg = {}):
                std::runtime_error(msg.toStdString()) {}
        };

        struct OutOfProcessBufferStreamingNotSupported: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };

        struct InsufficientAppArmorPermissions: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };

        struct UriNotFound: public ExceptionBase {
            using ExceptionBase::ExceptionBase;
        };
    };

    enum PlaybackStatus
    {
        null,
        ready,
        playing,
        paused,
        stopped
    };

    enum LoopStatus
    {
        none,
        track,
        playlist
    };

    /**
     * Audio stream role types used to categorize audio playback.
     * multimedia is the default role type and will be automatically
     * paused by media-hub when other types need to play.
     */
    enum AudioStreamRole
    {
        alarm,
        alert,
        multimedia,
        phone
    };

    enum Orientation
    {
        rotate0,
        rotate90,
        rotate180,
        rotate270
    };

    enum Lifetime
    {
        normal,
        resumable
    };

    enum Error
    {
        no_error,
        resource_error,
        format_error,
        network_error,
        access_denied_error,
        service_missing_error
    };
};

}
}
}

Q_DECLARE_METATYPE(core::ubuntu::media::Player::Client)

#endif // CORE_UBUNTU_MEDIA_PLAYER_H_
