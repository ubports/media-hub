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
#ifndef CORE_UBUNTU_MEDIA_APPARMOR_UBUNTU_H_
#define CORE_UBUNTU_MEDIA_APPARMOR_UBUNTU_H_

#include <core/media/apparmor/context.h>

#include <QDBusConnection>
#include <QSharedPointer>
#include <QStringList>

#include <functional>

class QUrl;

namespace core
{
namespace ubuntu
{
namespace media
{
namespace apparmor
{
// Collects Ubuntu-specific apparmor conventions, e.g., format
// of short and full package names as well as convenience functionality
// to inspect apparmor::Context instances.
namespace ubuntu
{
// The unconfined profile, unconditionally trusted
// by the system.
static constexpr const char* unconfined
{
    "unconfined"
};

class Context : public apparmor::Context
{
public:
    // Constructs a new Context instance for the given raw name.
    // Throws std::logic_error for empty names or for names not
    // complying to Ubuntu conventions.
    Context(const QString &name);

    // Returns true iff the context is unconfined.
    bool is_unconfined() const;

    // Returns true iff the context matches Unity.
    bool is_unity() const;

    // Returns true iff the context contains a package name.
    bool has_package_name() const;

    // Returns the package name or throws if no package name can be found.
    QString package_name() const;

    QString profile_name() const;

private:
    QStringList app_id_parts;
    QString pkg_name_;
    const bool unconfined_;
    const bool unity_;
    const bool has_package_name_;
};

// Abstracts query for the apparmor context of an incoming request
class RequestContextResolver
{
public:
    // To save us some typing.
    typedef QSharedPointer<RequestContextResolver> Ptr;

    // Callback for resolve context operations.
    typedef std::function<void(const Context&)> ResolveCallback;

    // Resolves the given name (of a dbus participant) to its apparmor context,
    // invoking the callback whenever a result is available.
    virtual void resolve_context_for_dbus_name_async(const QString &name, ResolveCallback cb) = 0;

protected:
    RequestContextResolver() = default;
    RequestContextResolver(const RequestContextResolver&) = delete;
    virtual ~RequestContextResolver() = default;
    RequestContextResolver& operator=(const RequestContextResolver&) = delete;
};

// An implementation of RequestContextResolver that queries the dbus
// daemon to resolve the apparmor context.
class DBusDaemonRequestContextResolver : public RequestContextResolver
{
public:
    // Constructs a new instance for the given bus connection.
    DBusDaemonRequestContextResolver();

    // From RequestContextResolver
    void resolve_context_for_dbus_name_async(const QString &name, ResolveCallback) override;

private:
    QDBusConnection m_connection;
};

// Abstracts an apparmor-based authentication of
// incoming requests from clients.
class RequestAuthenticator
{
public:
    // To save us some typing.
    typedef QSharedPointer<RequestAuthenticator> Ptr;

    // Return type of an authentication call.
    typedef std::tuple
    <
        bool,       // True if authenticated, false if not.
        QString // Reason for the result.
    > Result;

    virtual ~RequestAuthenticator() = default;

    // Returns true iff the client identified by the given apparmor::Context is allowed
    // to access the given uri, false otherwise.
    virtual Result authenticate_open_uri_request(const Context&, const QUrl &uri) = 0;

protected:
    RequestAuthenticator() = default;
    RequestAuthenticator(const RequestAuthenticator&) = default;   
    RequestAuthenticator& operator=(const RequestAuthenticator&) = default;
};

// Takes the existing logic and exposes it as an implementation
// of the RequestAuthenticator interface.
struct ExistingAuthenticator : public RequestAuthenticator
{
    ExistingAuthenticator() = default;
    // From RequestAuthenticator
    Result authenticate_open_uri_request(const Context&, const QUrl &uri) override;
};

}
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_APPARMOR_UBUNTU_H_
