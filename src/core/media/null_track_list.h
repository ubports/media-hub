/*
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

#ifndef CORE_MEDIA_NULL_TRACK_LIST_H_
#define CORE_MEDIA_NULL_TRACK_LIST_H_

#include <core/media/track.h>
#include <core/media/track_list.h>

namespace core
{
namespace ubuntu
{
namespace media
{
// A helper type to replace the playlist implementation below.
// Please note that this type is only a temporary manner. Ideally,
// the actual implementation should be injected as a dependency from the
// outside.
struct NullTrackList : public media::TrackList
{
    NullTrackList() = default;

    bool has_next()
    {
        return false;
    }

    media::Track::Id next()
    {
        return media::Track::Id{};
    }

    media::Track::UriType query_uri_for_track(const media::Track::Id&)
    {
        return media::Track::UriType{};
    }

    const core::Property<bool>& can_edit_tracks() const override
    {
        return props_and_sigs.can_edit_tracks;
    }

    const core::Property<Container>& tracks() const override
    {
        return props_and_sigs.tracks;
    }

    virtual media::Track::MetaData query_meta_data_for_track(const media::Track::Id&) override
    {
        return media::Track::MetaData{};
    }

    void add_track_with_uri_at(const media::Track::UriType&, const media::Track::Id&, bool) override
    {
    }

    void remove_track(const media::Track::Id&) override
    {
    }

    void go_to(const media::Track::Id&) override
    {
    }

    const core::Signal<void>& on_track_list_replaced() const override
    {
        return props_and_sigs.on_track_list_replaced;
    }

    const core::Signal<media::Track::Id>& on_track_added() const override
    {
        return props_and_sigs.on_track_added;
    }

    const core::Signal<media::Track::Id>& on_track_removed() const override
    {
        return props_and_sigs.on_track_removed;
    }

    const core::Signal<media::Track::Id>& on_track_changed() const override
    {
        return props_and_sigs.on_track_changed;
    }

    struct
    {
        core::Property<bool> can_edit_tracks;
        core::Property<TrackList::Container> tracks;
        core::Signal<void> on_track_list_replaced;
        core::Signal<media::Track::Id> on_track_added;
        core::Signal<media::Track::Id> on_track_removed;
        core::Signal<media::Track::Id> on_track_changed;
    } props_and_sigs;
};
}
}
}

#endif // CORE_MEDIA_NULL_TRACK_LIST_H_
