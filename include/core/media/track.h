/*
 * Copyright © 2013 Canonical Ltd.
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
#ifndef CORE_UBUNTU_MEDIA_TRACK_H_
#define CORE_UBUNTU_MEDIA_TRACK_H_

#include "core/media/connection.h"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace core
{
namespace ubuntu
{
namespace media
{
template<typename T> class Property;

class Track
{
public:
    typedef std::string UriType;
    typedef std::string Id;

    class MetaData
    {
    public:
        bool operator==(const MetaData& rhs) const
        {
            return map == rhs.map;
        }

        bool operator!=(const MetaData& rhs) const
        {
            return map != rhs.map;
        }

        template<typename Tag>
        std::size_t count() const
        {
            return count(Tag::name());
        }

        template<typename Tag>
        void set(const typename Tag::ValueType& value)
        {
            std::stringstream ss; ss << value;
            set(Tag::name(), ss.str());
        }

        template<typename Tag>
        typename Tag::ValueType get() const
        {
            std::stringstream ss(get(Tag::name()));
            typename Tag::ValueType value; ss >> value;

            return std::move(value);
        }

        std::size_t count(const std::string& key) const
        {
            return map.count(key);
        }

        void set(const std::string& key, const std::string& value)
        {
            map[key] = value;
        }

        const std::string& get(const std::string& key) const
        {
            return map.at(key);
        }

        const std::map<std::string, std::string>& operator*() const
        {
            return map;
        }

    private:
        std::map<std::string, std::string> map;
    };

    Track(const Id& id);
    Track(const Track&) = delete;
    virtual ~Track();

    Track& operator=(const Track&);
    bool operator==(const Track&) const;

    virtual const Id& id() const;
    /*
    class MetaData
    {
    public:
        MetaData() = default;
        MetaData(const MetaData&) = default;
        ~MetaData() = default;

        MetaData& operator=(const MetaData&) = default;

        bool operator==(const MetaData&) const
        {
            return true;
        }

        bool operator!=(const MetaData&) const
        {
            return false;
        }

        struct NotImplementedFields
        {
            NotImplementedFields() = default;

            virtual const UriType& uri() const = 0;
            virtual const std::chrono::microseconds length() const = 0;
            virtual const UriType& art_uri() const = 0;
            virtual const std::string& album() const = 0;
            virtual const std::vector<std::string>& album_artist() const = 0;
            virtual const std::vector<std::string>& artist() const = 0;
            virtual const std::string& as_text() const = 0;
            virtual unsigned int audio_bpm() const = 0;
            virtual float auto_rating() const = 0;
            virtual const std::vector<std::string>& comment() const = 0;
            virtual const std::vector<std::string>& composer() const = 0;
            virtual unsigned int disc_number() const = 0;
            virtual const std::vector<std::string>& genre() const = 0;
            virtual const std::vector<std::string>& lyricist() const = 0;
            virtual const std::string title() const = 0;
            virtual unsigned int track_number() const = 0;
            virtual unsigned int use_count() const = 0;
            virtual float user_rating() const = 0;
        };
    };
*/
private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_TRACK_H_
