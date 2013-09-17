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
#ifndef COM_UBUNTU_MUSIC_TRACK_H_
#define COM_UBUNTU_MUSIC_TRACK_H_

#include "com/ubuntu/music/connection.h"

#include <functional>
#include <memory>
#include <string>

namespace com
{
namespace ubuntu
{
namespace music
{
class Track
{
public:
    typedef std::string UriType;

    class MetaData
    {
      public:
        typedef std::string KeyType;
        typedef std::string ValueType;
        
        static const KeyType& track_id_key();

        ~MetaData();
        MetaData(const MetaData&);
        MetaData& operator=(const MetaData&);
        bool operator==(const MetaData&) const;
        
        bool has_value_for_key(const KeyType& key) const;
        const ValueType& value_for_key(const KeyType& key) const;

        void for_each(const std::function<void(const KeyType&, const ValueType&)>& f); 

      private:
        friend class Player;
        friend class Track;
        friend class TrackList;

        MetaData();
        
        struct Private;
        std::unique_ptr<Private> d;
    };

    Track(const Track&);
    ~Track();

    Track& operator=(const Track&);
    bool operator==(const Track&) const;

    const UriType& uri() const;

    const MetaData& meta_data() const;
    Connection on_meta_data_changed(const std::function<void(const MetaData&)>& f);
    
  private:
    friend class Player;
    friend class TrackList;

    explicit Track(const UriType& uri, const MetaData& meta_data);

    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_TRACK_H_
