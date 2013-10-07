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

#ifndef COM_UBUNTU_MUSIC_TRACK_LIST_IMPLEMENTATION_H_
#define COM_UBUNTU_MUSIC_TRACK_LIST_IMPLEMENTATION_H_

#include "track_list_skeleton.h"

namespace com
{
namespace ubuntu
{
namespace music
{
class TrackListImplementation : public TrackListSkeleton
{
public:
    TrackListImplementation(
            const org::freedesktop::dbus::types::ObjectPath& op);
    ~TrackListImplementation();

    void add_track_with_uri_at(const Track::UriType& uri, const Track::Id& position, bool make_current);
    void remove_track(const Track::Id& id);

    void go_to(const Track::Id& track);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_TRACK_LIST_IMPLEMENTATION_H_
