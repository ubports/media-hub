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

#include "error_p.h"

#include "dbus_constants.h"

#include <QDBusMessage>
#include <QDebug>

namespace lomiri {
namespace MediaHub {

Error errorFromDBus(const QDBusMessage &msg)
{
    using E = Error::Code;

    Error::Code code = E::NoError;

    const QString dbusCode = msg.errorName();
    if (dbusCode.startsWith(QStringLiteral(MPRIS_ERROR_PREFIX))) {
        const size_t prefixLength = sizeof(MPRIS_ERROR_PREFIX) - 1;
        const QStringRef name = dbusCode.midRef(prefixLength);
        if (name == MPRIS_ERROR_CREATING_SESSION ||
            name == MPRIS_ERROR_DESTROYING_SESSION) {
            /* The client perfectly knows what it was trying to do, so there's
             * no point in sending that information back.
             */
            code = E::InternalError;
        } else if (name == MPRIS_ERROR_INSUFFICIENT_APPARMOR_PERMISSIONS) {
            code = E::AccessDeniedError;
        } else if (name == MPRIS_ERROR_OOP_STREAMING_NOT_SUPPORTED) {
            code = E::OutOfProcessBufferStreamingNotSupported;
        } else if (name == MPRIS_ERROR_URI_NOT_FOUND) {
            code = E::ResourceError;
        }
    } else if (dbusCode.startsWith(QStringLiteral(FDO_ERROR_PREFIX))) {
        const size_t prefixLength = sizeof(FDO_ERROR_PREFIX) - 1;
        const QStringRef name = dbusCode.midRef(prefixLength);
        if (name == FDO_ERROR_ACCESS_DENIED) {
            code = E::AccessDeniedError;
        } else if (name == FDO_ERROR_DISCONNECTED ||
                   name == FDO_ERROR_NAME_HAS_NO_OWNER ||
                   name == FDO_ERROR_SERVICE_UNKNOWN) {
            code = E::ServiceMissingError;
        }
    }

    if (code == E::NoError) {
        qWarning() << "Unexpected error code" << dbusCode;
        code = E::InternalError;
    }

    return Error(code, msg.errorMessage());
}

Error errorFromApiCode(quint16 apiCode)
{
    Error::Code code;
    QString text;
    switch (apiCode) {
        case DBusConstants::NoError:
            code = Error::NoError;
            break;
        case DBusConstants::ResourceError:
            code = Error::ResourceError;
            text = "A media resource couldn't be resolved.";
            break;
        case DBusConstants::FormatError:
            code = Error::FormatError;
            text = "The media format type is not playable "
                "due to a missing codec.";
            break;
        case DBusConstants::NetworkError:
            code = Error::NetworkError;
            text = "A network error occurred.";
            break;
        case DBusConstants::AccessDeniedError:
            code = Error::AccessDeniedError;
            text = "Insufficient privileges to play that media.";
            break;
        case DBusConstants::ServiceMissingError:
            code = Error::ServiceMissingError;
            text = "A valid playback service was not found, "
                "playback cannot proceed.";
            break;
        default:
            qWarning() << "Unknown error code" << apiCode;
            code = Error::InternalError;
            text = "The backend emitted an unrecognized error code";
    }

    return Error(code, text);
}

} // namespace MediaHub
} // namespace lomiri
