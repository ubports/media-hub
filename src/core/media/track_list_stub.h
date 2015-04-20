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

#ifndef CORE_UBUNTU_MEDIA_TRACK_LIST_STUB_H_
#define CORE_UBUNTU_MEDIA_TRACK_LIST_STUB_H_

#include <core/media/track_list.h>

#include "track_list_traits.h"

#include <core/dbus/stub.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class TrackListStub : public core::ubuntu::media::TrackList
{
public:
    TrackListStub(
            const std::shared_ptr<Player>& parent,
            const core::dbus::Object::Ptr& object);
    ~TrackListStub();

    const core::Property<bool>& can_edit_tracks() const;
    const core::Property<Container>& tracks() const;

    Track::MetaData query_meta_data_for_track(const Track::Id& id);

    void add_track_with_uri_at(const Track::UriType& uri, const Track::Id& position, bool make_current);
    void remove_track(const Track::Id& id);

    void go_to(const Track::Id& track, bool toggle_player_state);

    void shuffle_tracks();
    void unshuffle_tracks();

    void reset();

    const core::Signal<ContainerTrackIdTuple>& on_track_list_replaced() const;
    const core::Signal<Track::Id>& on_track_added() const;
    const core::Signal<Track::Id>& on_track_removed() const;
    const core::Signal<Track::Id>& on_track_changed() const;
    const core::Signal<std::pair<Track::Id, bool>>& on_go_to_track() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_TRACK_LIST_STUB_H_
