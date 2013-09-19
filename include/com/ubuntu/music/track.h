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
#include <map>
#include <memory>
#include <string>

namespace com
{
namespace ubuntu
{
namespace music
{
template<typename T> class Property;

class Track
{
public:
    typedef std::string UriType;

    typedef std::map<std::string, std::string> MetaData;

    Track(const Track&);
    ~Track();

    Track& operator=(const Track&);
    bool operator==(const Track&) const;

    const UriType& uri() const;

    const Property<MetaData>& meta_data() const;
    
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
