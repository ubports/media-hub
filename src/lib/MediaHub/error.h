/*
 * Copyright © 2021 UBports Foundation.
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

#ifndef LOMIRI_MEDIAHUB_ERROR_H
#define LOMIRI_MEDIAHUB_ERROR_H

#include "global.h"

#include <QDebug>
#include <QMetaType>
#include <QString>

namespace lomiri {
namespace MediaHub {

class MH_EXPORT Error
{
public:
    enum Code {
        NoError,
        ResourceError,
        FormatError,
        NetworkError,
        AccessDeniedError,
        ServiceMissingError,
        InternalError,
        OutOfProcessBufferStreamingNotSupported,
    };

    Error(Code code = NoError, const QString &message = QString()):
        m_code(code), m_message(message) {}

    bool operator==(const Error &o) const {
        return o.code() == code() && o.message() == message();
    }

    Code code() const { return m_code; }
    bool isError() const { return m_code != NoError; }

    const QString &message() const { return m_message; }

    QString toString() const {
        return m_message + QString(" (%1)").arg(m_code);
    }

private:
    Code m_code;
    QString m_message;
};

inline QDebug operator<<(QDebug dbg, const Error &error)
{
    dbg.nospace() << "Error(" << error.code() << "): " << error.message();
    return dbg.maybeSpace();
}

} // namespace MediaHub
} // namespace lomiri

Q_DECLARE_METATYPE(lomiri::MediaHub::Error)

#endif // LOMIRI_MEDIAHUB_ERROR_H
