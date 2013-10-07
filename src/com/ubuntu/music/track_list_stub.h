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

#ifndef COM_UBUNTU_MUSIC_TRACK_LIST_STUB_H_
#define COM_UBUNTU_MUSIC_TRACK_LIST_STUB_H_

#include <com/ubuntu/music/track_list.h>

#include "track_list_traits.h"

#include <org/freedesktop/dbus/stub.h>

#include <memory>

namespace com
{
namespace ubuntu
{
namespace music
{
class TrackListStub : public org::freedesktop::dbus::Stub<com::ubuntu::music::TrackList>
{
public:
    TrackListStub(
            const std::shared_ptr<Player>& parent,
            const org::freedesktop::dbus::types::ObjectPath& op);
    ~TrackListStub();

    const Property<bool>& can_edit_tracks() const;
    const Property<Container>& tracks() const;

    void add_track_with_uri_at(const Track::UriType& uri, const Track::Id& position, bool make_current);
    void remove_track(const Track::Id& id);

    void go_to(const Track::Id& track);

    const Signal<void>& on_track_list_replaced() const;
    const Signal<std::shared_ptr<Track>>& on_track_added() const;
    const Signal<std::shared_ptr<Track>>& on_track_removed() const;
    const Signal<std::shared_ptr<Track>>& on_track_changed() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_TRACK_LIST_STUB_H_
