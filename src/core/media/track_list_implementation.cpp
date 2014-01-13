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

#include "track_list_implementation.h"

#include "engine.h"

#include <core/media/property.h>

namespace dbus = org::freedesktop::dbus;
namespace music = com::ubuntu::music;

struct music::TrackListImplementation::Private
{
    typedef std::map<Track::Id, std::tuple<Track::UriType, Track::MetaData>> MetaDataCache;

    dbus::types::ObjectPath path;
    MetaDataCache meta_data_cache;
    std::shared_ptr<music::Engine::MetaDataExtractor> extractor;
};

music::TrackListImplementation::TrackListImplementation(
        const dbus::types::ObjectPath& op,
        const std::shared_ptr<music::Engine::MetaDataExtractor>& extractor)
    : music::TrackListSkeleton(op),
      d(new Private{op, Private::MetaDataCache{}, extractor})
{
    can_edit_tracks().set(true);
}

music::TrackListImplementation::~TrackListImplementation()
{
}

music::Track::MetaData music::TrackListImplementation::query_meta_data_for_track(const music::Track::Id& id)
{
    auto it = d->meta_data_cache.find(id);

    if (it == d->meta_data_cache.end())
        return Track::MetaData{};

    return std::get<1>(it->second);
}

void music::TrackListImplementation::add_track_with_uri_at(
        const music::Track::UriType& uri,
        const music::Track::Id& position,
        bool make_current)
{
    static size_t track_counter = 0;

    std::stringstream ss; ss << d->path.as_string() << "/" << track_counter++;
    Track::Id id{ss.str()};

    auto result = tracks().update([this, id, position, make_current](TrackList::Container& container)
    {        
        auto it = std::find(container.begin(), container.end(), position);
        container.insert(it, id);
        return true;
    });

    if (result)
    {
        if (d->meta_data_cache.count(id) == 0)
        {
            d->meta_data_cache[id] = std::make_tuple(
                        uri,
                        d->extractor->meta_data_for_track_with_uri(uri));
        } else
        {
            std::get<0>(d->meta_data_cache[id]) = uri;
        }

        if (make_current)
            go_to(id);

        on_track_added()(id);
    }
}

void music::TrackListImplementation::remove_track(const music::Track::Id& id)
{
    auto result = tracks().update([id](TrackList::Container& container)
    {
        container.remove(id);
        return true;
    });

    if (result)
    {
        d->meta_data_cache.erase(id);

        on_track_removed()(id);
    }

}

void music::TrackListImplementation::go_to(const music::Track::Id& track)
{
    (void) track;
}
