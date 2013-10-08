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
#ifndef COM_UBUNTU_MUSIC_GSTREAMER_ENGINE_H_
#define COM_UBUNTU_MUSIC_GSTREAMER_ENGINE_H_

#include "../engine.h"

namespace gstreamer
{
class Engine : public com::ubuntu::music::Engine
{
public:
    Engine();
    ~Engine();

    const std::shared_ptr<MetaDataExtractor>& meta_data_extractor() const;

    const com::ubuntu::music::Property<State>& state() const;

    bool open_resource_for_uri(const com::ubuntu::music::Track::UriType& uri);

    bool play();
    bool stop();
    bool pause();
    bool seek_to(const std::chrono::microseconds& ts);

    const com::ubuntu::music::Property<com::ubuntu::music::Engine::Volume>& volume() const;
    com::ubuntu::music::Property<com::ubuntu::music::Engine::Volume>& volume();

    const com::ubuntu::music::Property<std::tuple<com::ubuntu::music::Track::UriType, com::ubuntu::music::Track::MetaData>>& track_meta_data() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}

#endif // COM_UBUNTU_MUSIC_GSTREAMER_ENGINE_H_
