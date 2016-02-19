/*
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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 *
 */

#ifndef URI_CHECK_H_
#define URI_CHECK_H_

#include <gio/gio.h>

#include <memory>
#include <iostream>

namespace core
{
namespace ubuntu
{
namespace media
{

class UriCheck
{
public:
    typedef std::shared_ptr<UriCheck> Ptr;

    UriCheck()
        : is_encoded_(false),
          is_local_file_(false),
          local_file_exists_(false)
    {
    }

    UriCheck(const std::string& uri)
        : uri_(uri),
          is_encoded_(is_encoded()),
          is_local_file_(is_local_file()),
          local_file_exists_(file_exists())
    {
    }

    virtual ~UriCheck()
    {
    }

    void set(const std::string& uri)
    {
        if (uri.empty())
            return;

        uri_ = uri;
        is_encoded_ = is_encoded();
        is_local_file_ = is_local_file();
        local_file_exists_ = file_exists();
    }

    void clear()
    {
        uri_.clear();
    }

    bool is_encoded() const
    {
        if (uri_.empty())
            return false;

        const std::string unescaped_uri{g_uri_unescape_string(uri_.c_str(), nullptr)};
        std::cout << "original uri: " << uri_ << std::endl;
        std::cout << "unescaped uri: " << unescaped_uri << std::endl;
        return unescaped_uri.length() < uri_.length();
    }

    bool is_local_file() const
    {
        if (uri_.empty())
            return false;

        const std::string uri_scheme {g_uri_parse_scheme(uri_.c_str())};
        return uri_.at(0) == '/' or
                (uri_.at(0) == '.' and uri_.at(1) == '/') or
                uri_scheme == "file";
    }

    bool file_exists() const
    {
        if (!is_local_file_)
            return false;

        GError *error = nullptr;
        // Open the URI and get the mime type from it. This will currently only work for
        // a local file
        std::unique_ptr<GFile, void(*)(void *)> file(
                g_file_new_for_uri(uri_.c_str()), g_object_unref);
        std::unique_ptr<GFileInfo, void(*)(void *)> info(
                g_file_query_info(
                    file.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE ","
                    G_FILE_ATTRIBUTE_ETAG_VALUE, G_FILE_QUERY_INFO_NONE,
                    /* cancellable */ NULL, &error),
                g_object_unref);

        std::cout << "File \"" << uri_ << "\" exists: " << (info.get() != nullptr) << std::endl;

        return info.get() != nullptr;
    }

protected:
    UriCheck(const UriCheck&) = delete;
    UriCheck& operator=(const UriCheck&) = delete;

private:
    std::string uri_;
    bool is_encoded_;
    bool is_local_file_;
    bool local_file_exists_;
};

}
}
}

#endif // URI_CHECK_H_
