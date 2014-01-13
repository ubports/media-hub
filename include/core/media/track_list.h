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
#ifndef CORE_UBUNTU_MEDIA_TRACK_LIST_H_
#define CORE_UBUNTU_MEDIA_TRACK_LIST_H_

#include <core/media/track.h>

#include <functional>
#include <list>
#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class Player;
template<typename T> class Property;
template<typename T> class Signal;

class TrackList : public std::enable_shared_from_this<TrackList>
{
  public:
    typedef std::list<Track::Id> Container;
    typedef Container::iterator Iterator;
    typedef Container::const_iterator ConstIterator;

    static const Track::Id& after_empty_track();

    TrackList(const TrackList&) = delete;
    ~TrackList();

    TrackList& operator=(const TrackList&) = delete;
    bool operator==(const TrackList&) const = delete;

    virtual const Property<bool>& can_edit_tracks() const = 0;
    virtual const Property<Container>& tracks() const = 0;

    virtual Track::MetaData query_meta_data_for_track(const Track::Id& id) = 0;
    virtual void add_track_with_uri_at(const Track::UriType& uri, const Track::Id& position, bool make_current) = 0;
    virtual void remove_track(const Track::Id& id) = 0;

    virtual void go_to(const Track::Id& track) = 0;

    virtual const Signal<void>& on_track_list_replaced() const = 0;
    virtual const Signal<Track::Id>& on_track_added() const = 0;
    virtual const Signal<Track::Id>& on_track_removed() const = 0;
    virtual const Signal<Track::Id>& on_track_changed() const = 0;

protected:
    TrackList();
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_TRACK_LIST_H_
