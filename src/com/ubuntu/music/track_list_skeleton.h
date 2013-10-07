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
#ifndef COM_UBUNTU_MUSIC_TRACK_LIST_SKELETON_H_
#define COM_UBUNTU_MUSIC_TRACK_LIST_SKELETON_H_

#include <com/ubuntu/music/track_list.h>

#include <com/ubuntu/music/player.h>

#include <org/freedesktop/dbus/skeleton.h>

namespace com
{
namespace ubuntu
{
namespace music
{
class TrackListSkeleton : public org::freedesktop::dbus::Skeleton<com::ubuntu::music::TrackList>
{
public:
    TrackListSkeleton(
            const org::freedesktop::dbus::types::ObjectPath& op);
    ~TrackListSkeleton();

    const Property<bool>& can_edit_tracks() const;
    const Property<Container>& tracks() const;

    const Signal<void>& on_track_list_replaced() const;
    const Signal<std::shared_ptr<Track>>& on_track_added() const;
    const Signal<std::shared_ptr<Track>>& on_track_removed() const;
    const Signal<std::shared_ptr<Track>>& on_track_changed() const;

protected:
    Property<bool>& can_edit_tracks();
    Property<Container>& tracks();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}
#endif // COM_UBUNTU_MUSIC_PROPERTY_H_
