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
    typedef std::map<std::string, std::string> MetaDataType;

    class MetaData
    {
    public:
        static constexpr const char* TrackArtlUrlKey = "mpris:artUrl";
        static constexpr const char* TrackLengthKey = "mpris:length";
        static constexpr const char* TrackIdKey = "mpris:trackid";

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

        bool is_set(const std::string& key) const
        {
            try {
                return !map.at(key).empty();
            } catch (const std::out_of_range& e) {
                return false;
            }
        }

        const std::map<std::string, std::string>& operator*() const
        {
            return map;
        }

        std::string encode(const std::string& key) const;

        const std::string& album() const;
        const std::string& artist() const;
        const std::string& title() const;
        const std::string& track_id() const;
        const std::string& track_length() const;
        const std::string& art_url() const;
        const std::string& last_used() const;

        void set_album(const std::string& album);
        void set_artist(const std::string& artist);
        void set_title(const std::string& title);
        void set_track_id(const std::string& id);
        void set_track_length(const std::string& id);
        void set_art_url(const std::string& url);
        void set_last_used(const std::string& datetime);

    private:
        MetaDataType map;
    };

    Track(const Id& id);
    Track(const Track&) = delete;
    virtual ~Track();

    Track& operator=(const Track&);
    bool operator==(const Track&) const;

    virtual const Id& id() const;
    virtual const UriType& uri() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_TRACK_H_
