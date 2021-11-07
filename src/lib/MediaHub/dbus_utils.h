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

#ifndef LOMIRI_MEDIAHUB_DBUS_UTILS_H
#define LOMIRI_MEDIAHUB_DBUS_UTILS_H

class QDBusPendingCall;

namespace lomiri {
namespace MediaHub {

struct DBusUtils {
    static bool waitForFinished(const QDBusPendingCall &call);
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_DBUS_UTILS_H
