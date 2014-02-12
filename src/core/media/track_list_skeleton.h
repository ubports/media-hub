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
#ifndef CORE_UBUNTU_MEDIA_TRACK_LIST_SKELETON_H_
#define CORE_UBUNTU_MEDIA_TRACK_LIST_SKELETON_H_

#include <core/media/track_list.h>

#include <core/media/player.h>

#include <core/dbus/skeleton.h>

namespace core
{
namespace ubuntu
{
namespace media
{
class TrackListSkeleton : public core::dbus::Skeleton<core::ubuntu::media::TrackList>
{
public:
    TrackListSkeleton(
            const core::dbus::types::ObjectPath& op);
    ~TrackListSkeleton();

    bool has_next() const;
    const Track::Id& next();

    const core::Property<bool>& can_edit_tracks() const;
    const core::Property<Container>& tracks() const;

    const core::Signal<void>& on_track_list_replaced() const;
    const core::Signal<Track::Id>& on_track_added() const;
    const core::Signal<Track::Id>& on_track_removed() const;
    const core::Signal<Track::Id>& on_track_changed() const;

protected:
    core::Property<bool>& can_edit_tracks();
    core::Property<Container>& tracks();

    core::Signal<void>& on_track_list_replaced();
    core::Signal<Track::Id>& on_track_added();
    core::Signal<Track::Id>& on_track_removed();
    core::Signal<Track::Id>& on_track_changed();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}
#endif // CORE_UBUNTU_MEDIA_PROPERTY_H_
