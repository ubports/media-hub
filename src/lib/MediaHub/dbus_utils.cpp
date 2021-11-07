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

#include "dbus_utils.h"

#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QEventLoop>

using namespace lomiri::MediaHub;

/* For compatibility with the old client library, some of this library methods
 * are blocking.  However we also want to continue iterating the main loop,
 * because we update the property cache when the D-Bus signals arrive (and they
 * will arrive just before the D-Bus call returns).
 */
bool DBusUtils::waitForFinished(const QDBusPendingCall &call)
{
    auto watcher = new QDBusPendingCallWatcher(call);
    QEventLoop loop;
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     [&loop](QDBusPendingCallWatcher *call) {
        call->deleteLater();
        QDBusPendingReply<void> reply(*call);
        if (reply.isError()) {
            qWarning() << Q_FUNC_INFO << reply.error();
            loop.exit(1);
        } else {
            loop.exit(0);
        }
    });
    int rc = loop.exec(QEventLoop::ExcludeUserInputEvents);
    return rc == 0;
}
