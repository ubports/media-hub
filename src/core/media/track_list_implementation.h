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

#ifndef CORE_UBUNTU_MEDIA_TRACK_LIST_IMPLEMENTATION_H_
#define CORE_UBUNTU_MEDIA_TRACK_LIST_IMPLEMENTATION_H_

#include "engine.h"
#include "track_list_skeleton.h"

namespace core
{
namespace ubuntu
{
namespace media
{
class TrackListImplementation : public TrackListSkeleton
{
public:
    TrackListImplementation(
            const core::dbus::types::ObjectPath& op,
            const std::shared_ptr<Engine::MetaDataExtractor>& extractor);
    ~TrackListImplementation();

    Track::UriType query_uri_for_track(const Track::Id& id);
    Track::MetaData query_meta_data_for_track(const Track::Id& id);

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

#endif // CORE_UBUNTU_MEDIA_TRACK_LIST_IMPLEMENTATION_H_
