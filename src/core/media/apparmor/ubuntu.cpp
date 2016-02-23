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

#include <core/media/external_services.h>

#include <iostream>
#include <regex>

namespace apparmor = core::ubuntu::media::apparmor;
namespace media = core::ubuntu::media;
namespace ubuntu = apparmor::ubuntu;

namespace
{
struct Uri
{
    std::string scheme;
    std::string authority;
    std::string path;
    std::string query;
    std::string fragment;
};

// Poor mans version of a uri parser.
// See https://tools.ietf.org/html/rfc3986#appendix-B
Uri parse_uri(const std::string& s)
{
    // Indices into the regex match go here.
    struct Index
    {
        const std::size_t scheme{2};
        const std::size_t authority{4};
        const std::size_t path{5};
        const std::size_t query{7};
        const std::size_t fragment{9};
    } static index;

    static const std::regex regex{R"delim(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)delim"};
    std::smatch match;

    if (not std::regex_match(s, match, regex)) throw std::runtime_error
    {
        "Not a valid URI: " + s
    };

    return Uri
    {
        match.str(index.scheme),
        match.str(index.authority),
        match.str(index.path),
        match.str(index.query),
        match.str(index.fragment)
    };
}

static constexpr std::size_t index_package{1};
static constexpr std::size_t index_app{2};
static const std::string unity_name{"unity8-dash"};

// Returns true if the context name is a valid Ubuntu app id.
// If it is, out is populated with the package and app name.
bool process_context_name(const std::string& s, std::smatch& out,
        std::string& pkg_name)
{
    // See https://wiki.ubuntu.com/AppStore/Interfaces/ApplicationId.
    static const std::regex short_re{"(.*)_(.*)"};
    static const std::regex full_re{"(.*)_(.*)_(.*)"};
    static const std::regex trust_store_re{"(.*)-(.*)"};

    if ((s == "messaging-app" or s == unity_name)
            and std::regex_match(s, out, trust_store_re))
    {
        pkg_name = s;
        return true;
    }

    if (std::regex_match(s, out, full_re) or std::regex_match(s, out, short_re))
    {
        pkg_name = out[index_package];
        return true;
    }

    return false;
}
}

apparmor::ubuntu::Context::Context(const std::string& name)
    : apparmor::Context{name},
      unconfined_{str() == ubuntu::unconfined},
      unity_{name == unity_name},
      has_package_name_{process_context_name(str(), match_, pkg_name_)}
{
    std::cout << "apparmor profile name: " << name;
    std::cout << ", is_unconfined(): " << is_unconfined();
    std::cout << ", has_package_name(): " << has_package_name() << std::endl;
    if (not is_unconfined() and not is_unity() and not has_package_name())
        throw std::logic_error
        {
            "apparmor::ubuntu::Context: Invalid profile name " + str()
        };
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

std::string apparmor::ubuntu::Context::package_name() const
{
    return pkg_name_;
}

std::string apparmor::ubuntu::Context::profile_name() const
{
    return std::string{match_[index_package]} + "-" + std::string{match_[index_app]};
}

apparmor::ubuntu::DBusDaemonRequestContextResolver::DBusDaemonRequestContextResolver(const core::dbus::Bus::Ptr& bus) : dbus_daemon{bus}
{
}

void apparmor::ubuntu::DBusDaemonRequestContextResolver::resolve_context_for_dbus_name_async(
        const std::string& name,
        apparmor::ubuntu::RequestContextResolver::ResolveCallback cb)
{
    dbus_daemon.get_connection_app_armor_security_async(name, [cb](const std::string& context_name)
    {
        cb(apparmor::ubuntu::Context{context_name});
    });
}

apparmor::ubuntu::RequestAuthenticator::Result apparmor::ubuntu::ExistingAuthenticator::authenticate_open_uri_request(const apparmor::ubuntu::Context& context, const std::string& uri)
{
    if (context.is_unconfined())
        return Result{true, "Client allowed access since it's unconfined"};

    Uri parsed_uri = parse_uri(uri);

    std::cout << "context.profile_name(): " << context.profile_name() << std::endl;
    std::cout << "parsed_uri.path: " << parsed_uri.path << std::endl;

    // All confined apps can access their own files
    if (parsed_uri.path.find(std::string(".local/share/" + context.package_name() + "/")) != std::string::npos ||
        parsed_uri.path.find(std::string(".cache/" + context.package_name() + "/")) != std::string::npos)
    {
        return Result
        {
            true,
            "Client can access content in ~/.local/share/" + context.package_name() + " or ~/.cache/" + context.package_name()
        };
    }
    // Check for trust-store compatible path name using full messaging-app profile_name
    else if (context.profile_name() == "messaging-app" &&
             /* Since the full APP_ID is not available yet (see aa_query_file_path()), add an exception: */
             (parsed_uri.path.find(std::string(".local/share/com.ubuntu." + context.profile_name() + "/")) != std::string::npos ||
             parsed_uri.path.find(std::string(".cache/com.ubuntu." + context.profile_name() + "/")) != std::string::npos))
    {
        return Result
        {
            true,
            "Client can access content in ~/.local/share/" + context.profile_name() + " or ~/.cache/" + context.profile_name()
        };
    }
    else if (parsed_uri.path.find(std::string("opt/click.ubuntu.com/")) != std::string::npos &&
             parsed_uri.path.find(context.package_name()) != std::string::npos)
    {
        return Result{true, "Client can access content in own opt directory"};
    }
    else if ((parsed_uri.path.find(std::string("/system/media/audio/ui/")) != std::string::npos ||
              parsed_uri.path.find(std::string("/android/system/media/audio/ui/")) != std::string::npos) &&
              context.package_name() == "com.ubuntu.camera")
    {
        return Result{true, "Camera app can access ui sounds"};
    }

    // TODO: Check if the trust store previously allowed direct access to uri

    // Check in ~/Music and ~/Videos
    // TODO: when the trust store lands, check it to see if this app can access the dirs and
    // then remove the explicit whitelist of the music-app, and gallery-app
    else if ((context.package_name() == "com.ubuntu.music" || context.package_name() == "com.ubuntu.gallery" ||
              context.profile_name() == unity_name) &&
            (parsed_uri.path.find(std::string("Music/")) != std::string::npos ||
             parsed_uri.path.find(std::string("Videos/")) != std::string::npos ||
             parsed_uri.path.find(std::string("/media")) != std::string::npos))
    {
        return Result{true, "Client can access content in ~/Music or ~/Videos"};
    }
    else if (parsed_uri.path.find(std::string("/usr/share/sounds")) != std::string::npos)
    {
        return Result{true, "Client can access content in /usr/share/sounds"};
    }
    else if (parsed_uri.scheme == "http" ||
             parsed_uri.scheme == "https" ||
             parsed_uri.scheme == "rtsp")
    {
        return Result{true, "Client can access streaming content"};
    }

    return Result{false, "Client is not allowed to access: " + uri};
}

// Returns the platform-default implementation of RequestContextResolver.
apparmor::ubuntu::RequestContextResolver::Ptr apparmor::ubuntu::make_platform_default_request_context_resolver(media::helper::ExternalServices& es)
{
    return std::make_shared<apparmor::ubuntu::DBusDaemonRequestContextResolver>(es.session);
}

// Returns the platform-default implementation of RequestAuthenticator.
apparmor::ubuntu::RequestAuthenticator::Ptr apparmor::ubuntu::make_platform_default_request_authenticator()
{
    return std::make_shared<apparmor::ubuntu::ExistingAuthenticator>();
}
