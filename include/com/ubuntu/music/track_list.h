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
#ifndef COM_UBUNTU_MUSIC_TRACK_LIST_H_
#define COM_UBUNTU_MUSIC_TRACK_LIST_H_

#include "com/ubuntu/music/track.h"

#include <functional>
#include <memory>
#include <vector>

namespace com
{
namespace ubuntu
{
namespace music
{
class TrackList
{
  public:
    typedef std::vector<Track>::iterator Iterator;
    typedef std::vector<Track>::const_iterator ConstIterator;

    TrackList();
    TrackList(const TrackList& rhs);
    ~TrackList();

    TrackList& operator=(const TrackList&);
    bool operator==(const TrackList&) const;

    bool is_editable();

    std::size_t size() const;

    Iterator begin();
    ConstIterator begin() const;
    Iterator end();
    ConstIterator end() const;
    Iterator find_for_uri(const Track::UriType& uri);
    ConstIterator find_for_uri(const Track::UriType& uri) const;

    void prepend_track_with_uri(const Track::UriType& uri, bool make_current);
    void append_track_with_uri(const Track::UriType& uri, bool make_current);
    void add_track_with_uri_at(const Track::UriType& uri, Iterator position, bool make_current);
    void remove_track_at(Iterator at);
    
    void for_each(const std::function<void(const Track&)> functor) const;

    void go_to(const Track& track);

    Connection on_track_list_replaced(const std::function<void()>& slot);
    Connection on_track_added(const std::function<void(const Track& t)>& slot);
    Connection on_track_removed(const std::function<void(const Track& t)>& slot);
    Connection on_current_track_changed(const std::function<void(const Track&)>& slot);
    
  private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_TRACK_LIST_H_
