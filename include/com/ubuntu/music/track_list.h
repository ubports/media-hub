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

namespace com
{
namespace ubuntu
{
namespace music
{
class TrackList
{
  public:
    TrackList();
    TrackList(const TrackList& rhs);
    ~TrackList();

    TrackList& operator=(const TrackList&);
    bool operator==(const TrackList&) const;

    bool is_editable();

    void add_track_with_uri(const Track::UriType& uri, const Track& after, bool make_current);
    void remove_track(const Track& track);

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
