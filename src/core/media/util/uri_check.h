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

#include <QFile>
#include <QUrl>

#include <memory>

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
        : is_local_file_(false),
          local_file_exists_(false)
    {
    }

    UriCheck(const QUrl &uri)
        : uri_(uri),
          is_local_file_(determine_if_local_file()),
          local_file_exists_(determine_if_file_exists())
    {
    }

    virtual ~UriCheck()
    {
    }

    void set(const QUrl &uri)
    {
        if (uri.isEmpty())
            return;

        uri_ = uri;
        is_local_file_ = determine_if_local_file();
        local_file_exists_ = determine_if_file_exists();
    }

    void clear()
    {
        uri_.clear();
    }

    bool is_local_file() const
    {
        return is_local_file_;
    }

    bool file_exists() const
    {
        return local_file_exists_;
    }

protected:
    UriCheck(const UriCheck&) = delete;
    UriCheck& operator=(const UriCheck&) = delete;

private:
    bool determine_if_local_file()
    {
        return uri_.isLocalFile();
    }

    bool determine_if_file_exists()
    {
        if (!is_local_file_)
            return false;

        return QFile::exists(uri_.toLocalFile());
    }

private:
    QUrl uri_;
    bool is_local_file_;
    bool local_file_exists_;
};

}
}
}

#endif // URI_CHECK_H_
