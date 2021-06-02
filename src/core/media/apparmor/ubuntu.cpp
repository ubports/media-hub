/*
 * Copyright © 2014 Canonical Ltd.
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

#include <core/media/apparmor/ubuntu.h>

#include "core/media/logging.h"

#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDebug>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <sys/apparmor.h>
#include <unistd.h> // geteuid()

namespace apparmor = core::ubuntu::media::apparmor;
namespace media = core::ubuntu::media;
namespace ubuntu = apparmor::ubuntu;

namespace
{

static constexpr std::size_t index_package{0};
static constexpr std::size_t index_app{1};
static const char unity_name[] = "unity8-dash";
static const char unity8_snap_name[] = "snap.unity8-session.unity8-session";

// ad-hoc for mediaplayer-app/music-app until it settles down with proper handling
// Bug #1642611
static const char mediaplayer_snap_name[] = "snap.mediaplayer-app.mediaplayer-app";
static const char music_snap_name[] = "snap.music-app.music-app";
// Returns true if the context name is a valid Ubuntu app id.
// If it is, out is populated with the package and app name.
bool process_context_name(const QString &s, QStringList &out,
                          QString &pkg_name)
{
    // See https://wiki.ubuntu.com/AppStore/Interfaces/ApplicationId.

    if ((s == "messaging-app" or s == unity_name or s == unity8_snap_name or
            s == mediaplayer_snap_name or s == music_snap_name)
            and s.contains('-'))
    {
        pkg_name = s;
        return true;
    }


    out = s.split('_');
    if (out.count() == 2 || out.count() == 3)
    {
        pkg_name = out[index_package];
        return true;
    }

    return false;
}
}

apparmor::ubuntu::Context::Context(const QString &name)
    : apparmor::Context{name},
      unconfined_{str() == ubuntu::unconfined},
      unity_{name == unity_name || name == unity8_snap_name},
      has_package_name_{process_context_name(str(), app_id_parts, pkg_name_)}
{
    MH_DEBUG("apparmor profile name: %s", qUtf8Printable(name));
    MH_DEBUG("is_unconfined(): %s", (is_unconfined() ? "true" : "false"));
    MH_DEBUG("has_package_name(): %s", (has_package_name() ? "true" : "false"));
    if (not is_unconfined() and not is_unity() and not has_package_name()) {
        MH_FATAL("apparmor::ubuntu::Context: Invalid profile name %s", qUtf8Printable(str()));
    }
}

bool apparmor::ubuntu::Context::is_unconfined() const
{
    return unconfined_;
}

bool apparmor::ubuntu::Context::is_unity() const
{
    return unity_;
}

bool apparmor::ubuntu::Context::has_package_name() const
{
    return has_package_name_;
}

QString apparmor::ubuntu::Context::package_name() const
{
    return pkg_name_;
}

QString apparmor::ubuntu::Context::profile_name() const
{
    return app_id_parts.isEmpty() ?
        str() : (app_id_parts[index_package] + "-" + app_id_parts[index_app]);
}

apparmor::ubuntu::DBusDaemonRequestContextResolver::DBusDaemonRequestContextResolver():
    m_connection(QDBusConnection::sessionBus())
{
}

void apparmor::ubuntu::DBusDaemonRequestContextResolver::resolve_context_for_dbus_name_async(
        const QString &name,
        apparmor::ubuntu::RequestContextResolver::ResolveCallback cb)
{
    const QString dbusServiceName =
        qEnvironmentVariable("MEDIA_HUB_MOCKED_DBUS", "org.freedesktop.DBus");
    QDBusMessage msg =
        QDBusMessage::createMethodCall(dbusServiceName,
                                       "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus",
                                       "GetConnectionCredentials");
    msg.setArguments({ name });
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(call);
    QObject::connect(callWatcher, &QDBusPendingCallWatcher::finished,
                     [cb](QDBusPendingCallWatcher *callWatcher) {
        QDBusReply<QVariantMap> reply(*callWatcher);
        QString appId;
        if (reply.isValid()) {
            QVariantMap map = reply.value();
            QByteArray context = map.value("LinuxSecurityLabel").toByteArray();
            if (!context.isEmpty()) {
                aa_splitcon(context.data(), NULL);
                appId = QString::fromUtf8(context);
            }
        } else {
            QDBusError error = reply.error();
            qWarning() << "Error getting app ID:" << error.name() <<
                error.message();
        }
        cb(apparmor::ubuntu::Context(appId));
        callWatcher->deleteLater();
    });
}

apparmor::ubuntu::RequestAuthenticator::Result apparmor::ubuntu::ExistingAuthenticator::authenticate_open_uri_request(const apparmor::ubuntu::Context& context, const QUrl &uri)
{
    if (context.is_unconfined())
        return Result{true, "Client allowed access since it's unconfined"};

    QString path = uri.path();
    MH_DEBUG("context.profile_name(): %s", qUtf8Printable(context.profile_name()));
    MH_DEBUG("parsed_uri.path: %s", qUtf8Printable(path));

    // All confined apps can access their own files
    if (path.contains(".local/share/" + context.package_name() + "/") ||
        path.contains(".cache/" + context.package_name() + "/") ||
        path.contains("/run/user/" + QString::number(geteuid()) + "/confined/" + context.package_name()))
    {
        return Result
        {
            true,
            "Client can access content in ~/.local/share/" + context.package_name() + " or ~/.cache/" + context.package_name()
        };
    }
    // Check for trust-store compatible path name using full messaging-app profile_name
    else if (context.package_name() == "messaging-app" &&
             /* Since the full APP_ID is not available yet (see aa_query_file_path()), add an exception: */
             (path.contains(".local/share/com.ubuntu." + context.profile_name() + "/") ||
             path.contains(".cache/com.ubuntu." + context.profile_name() + "/")))
    {
        return Result
        {
            true,
            "Client can access content in ~/.local/share/" + context.profile_name() + " or ~/.cache/" + context.profile_name()
        };
    }
    else if (path.contains("opt/click.ubuntu.com/") &&
             path.contains(context.package_name()))
    {
        return Result{true, "Client can access content in own opt directory"};
    }
    else if ((path.contains("/system/media/audio/ui/") ||
              path.contains("/android/system/media/audio/ui/")) &&
              context.package_name() == "com.ubuntu.camera")
    {
        return Result{true, "Camera app can access ui sounds"};
    }

    // TODO: Check if the trust store previously allowed direct access to uri

    // Check in ~/Music and ~/Videos
    // TODO: when the trust store lands, check it to see if this app can access the dirs and
    // then remove the explicit whitelist of the music-app, and gallery-app
    else if ((context.package_name() == "com.ubuntu.music" || context.package_name() == "com.ubuntu.gallery" ||
              context.profile_name() == unity_name || context.profile_name() == unity8_snap_name ||
              context.profile_name() == mediaplayer_snap_name || context.profile_name() == music_snap_name) &&
            (path.contains("Music/") ||
             path.contains("Videos/") ||
             path.contains("/media")))
    {
        return Result{true, "Client can access content in ~/Music or ~/Videos"};
    }
    else if (path.contains("/usr/share/sounds"))
    {
        return Result{true, "Client can access content in /usr/share/sounds"};
    }
    else if (uri.scheme() == "http" ||
             uri.scheme() == "https" ||
             uri.scheme() == "rtsp")
    {
        return Result{true, "Client can access streaming content"};
    }

    return Result{false, "Client is not allowed to access: " + uri.toString()};
}
