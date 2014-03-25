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
#ifndef CORE_UBUNTU_MEDIA_GSTREAMER_ENGINE_H_
#define CORE_UBUNTU_MEDIA_GSTREAMER_ENGINE_H_

#include "../engine.h"

namespace gstreamer
{
class Engine : public core::ubuntu::media::Engine
{
public:
    Engine();
    ~Engine();

    const std::shared_ptr<MetaDataExtractor>& meta_data_extractor() const;

    const core::Property<State>& state() const;

    bool open_resource_for_uri(const core::ubuntu::media::Track::UriType& uri);
    void create_video_sink(uint32_t texture_id);

    bool play();
    bool stop();
    bool pause();
    bool seek_to(const std::chrono::microseconds& ts);

    const core::Property<uint64_t>& position() const;
    const core::Property<uint64_t>& duration() const;

    const core::Property<core::ubuntu::media::Engine::Volume>& volume() const;
    core::Property<core::ubuntu::media::Engine::Volume>& volume();

    const core::Property<std::tuple<core::ubuntu::media::Track::UriType, core::ubuntu::media::Track::MetaData>>& track_meta_data() const;

    const core::Signal<void>& about_to_finish_signal() const;
    const core::Signal<void>& end_of_stream_signal() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
}

#endif // CORE_UBUNTU_MEDIA_GSTREAMER_ENGINE_H_
