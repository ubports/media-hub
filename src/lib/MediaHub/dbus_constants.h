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

#ifndef LOMIRI_MEDIAHUB_DBUS_CONSTANTS_H
#define LOMIRI_MEDIAHUB_DBUS_CONSTANTS_H

#define MEDIAHUB_SERVICE_NAME "core.ubuntu.media.Service"
#define MEDIAHUB_SERVICE_PATH "/core/ubuntu/media/Service"
#define MEDIAHUB_SERVICE_INTERFACE "core.ubuntu.media.Service"

#define MPRIS_SERVICE_NAME "org.mpris.MediaPlayer2.MediaHub"
#define MPRIS_SERVICE_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"
#define MPRIS_TRACKLIST_INTERFACE "org.mpris.MediaPlayer2.TrackList"

#define MPRIS_ERROR_PREFIX "mpris.Player.Error."
#define MPRIS_ERROR_CREATING_SESSION "CreatingSession"
#define MPRIS_ERROR_DESTROYING_SESSION "DestroyingSession"
#define MPRIS_ERROR_INSUFFICIENT_APPARMOR_PERMISSIONS \
    "InsufficientAppArmorPermissions"
#define MPRIS_ERROR_OOP_STREAMING_NOT_SUPPORTED \
    "OutOfProcessBufferStreamingNotSupported"
#define MPRIS_ERROR_URI_NOT_FOUND "UriNotFound"

#define FDO_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"

#define FDO_ERROR_PREFIX "org.freedesktop.DBus.Error."
#define FDO_ERROR_ACCESS_DENIED "AccessDenied"
#define FDO_ERROR_DISCONNECTED "Disconnected"
#define FDO_ERROR_NAME_HAS_NO_OWNER "NameHasNoOwner"
#define FDO_ERROR_SERVICE_UNKNOWN "ServiceUnknown"
#define FDO_ERROR_TIMEOUT "Timeout"
#define FDO_ERROR_UNKNOWN_OBJECT "UnknownObject"

/*
 * Error codes for the Player::Error() D-Bus signal (quint16)
 *
 * Keep in sync with core::ubuntu::media::Player::Error defined in
 * src/core/media/player.h
 */
namespace DBusConstants {

enum Error
{
    NoError,
    ResourceError,
    FormatError,
    NetworkError,
    AccessDeniedError,
    ServiceMissingError,
};

enum Backend
{
    None,
    Hybris,
    Mir
};

} // namespace

#endif // LOMIRI_MEDIAHUB_DBUS_CONSTANTS_H
