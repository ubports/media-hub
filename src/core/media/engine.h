/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */
#ifndef CORE_UBUNTU_MEDIA_ENGINE_H_
#define CORE_UBUNTU_MEDIA_ENGINE_H_

#include <core/media/player.h>
#include <core/media/track.h>

#include <core/property.h>

#include <chrono>

namespace core
{
namespace ubuntu
{
namespace media
{
class Engine
{
public:

    enum class State
    {
        no_media,
        ready,
        busy,
        playing,
        paused,
        stopped
    };

    struct Volume
    {
        static double min();
        static double max();

        explicit Volume(double v = max());

        bool operator!=(const Volume& rhs) const;
        bool operator==(const Volume& rhs) const;

        double value;
    };

    class MetaDataExtractor
    {
    public:
        virtual Track::MetaData meta_data_for_track_with_uri(const Track::UriType& uri) = 0;

    protected:
        MetaDataExtractor() = default;
        MetaDataExtractor(const MetaDataExtractor&) = delete;
        virtual ~MetaDataExtractor() = default;

        MetaDataExtractor& operator=(const MetaDataExtractor&) = delete;
    };

    virtual const std::shared_ptr<MetaDataExtractor>& meta_data_extractor() const = 0;

    virtual const core::Property<State>& state() const = 0;

    virtual bool open_resource_for_uri(const core::ubuntu::media::Track::UriType& uri, bool do_pipeline_reset) = 0;
    virtual bool open_resource_for_uri(const core::ubuntu::media::Track::UriType& uri, const Player::HeadersType&) = 0;
    // Throws core::ubuntu::media::Player::Error::OutOfProcessBufferStreamingNotSupported if the implementation does not
    // support this feature.
    virtual void create_video_sink(uint32_t texture_id) = 0;

    virtual bool play(bool use_main_context = false) = 0;
    virtual bool stop(bool use_main_context = false)  = 0;
    virtual bool pause() = 0;
    virtual bool seek_to(const std::chrono::microseconds& ts) = 0;
    virtual void equalizer_set_band(int band, double gain) = 0;

    virtual const core::Property<bool>& is_video_source() const = 0;
    virtual const core::Property<bool>& is_audio_source() const = 0;

    virtual const core::Property<uint64_t>& position() const = 0;
    virtual const core::Property<uint64_t>& duration() const = 0;

    virtual const core::Property<Volume>& volume() const = 0;
    virtual core::Property<Volume>& volume() = 0;

    virtual const core::Property<core::ubuntu::media::Player::AudioStreamRole>& audio_stream_role() const = 0;
    virtual core::Property<core::ubuntu::media::Player::AudioStreamRole>& audio_stream_role() = 0;

    virtual const core::Property<core::ubuntu::media::Player::Orientation>& orientation() const = 0;

    virtual const core::Property<core::ubuntu::media::Player::Lifetime>& lifetime() const = 0;
    virtual core::Property<core::ubuntu::media::Player::Lifetime>& lifetime() = 0;

    virtual const core::Property<std::tuple<Track::UriType, Track::MetaData>>& track_meta_data() const = 0;

    virtual const core::Signal<void>& about_to_finish_signal() const = 0;
    virtual const core::Signal<uint64_t>& seeked_to_signal() const = 0;
    virtual const core::Signal<void>& client_disconnected_signal() const = 0;
    virtual const core::Signal<void>& end_of_stream_signal() const = 0;
    virtual const core::Signal<core::ubuntu::media::Player::PlaybackStatus>& playback_status_changed_signal() const = 0;
    virtual const core::Signal<video::Dimensions>& video_dimension_changed_signal() const = 0;
    virtual const core::Signal<core::ubuntu::media::Player::Error>& error_signal() const = 0;
    virtual const core::Signal<int>& on_buffering_changed_signal() const = 0;

    virtual void reset() = 0;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_ENGINE_H_
