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
#ifndef CORE_UBUNTU_MEDIA_ENGINE_H_
#define CORE_UBUNTU_MEDIA_ENGINE_H_

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
        ready,
        busy,
        playing,
        paused,
        stopped
    };

    struct Xesam
    {
        static const std::string& album();
        static const std::string& album_artist();
        static const std::string& artist();
        static const std::string& as_text();
        static const std::string& audio_bpm();
        static const std::string& auto_rating();
        static const std::string& comment();
        static const std::string& composer();
        static const std::string& content_created();
        static const std::string& disc_number();
        static const std::string& first_used();
        static const std::string& genre();
        static const std::string& last_used();
        static const std::string& lyricist();
        static const std::string& title();
        static const std::string& track_number();
        static const std::string& url();
        static const std::string& use_count();
        static const std::string& user_rating();
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

    virtual bool open_resource_for_uri(const Track::UriType& uri) = 0;

    virtual bool play() = 0;
    virtual bool stop()  = 0;
    virtual bool pause() = 0;
    virtual bool seek_to(const std::chrono::microseconds& ts) = 0;

    virtual const core::Property<uint64_t>& position() const = 0;

    virtual const core::Property<Volume>& volume() const = 0;
    virtual core::Property<Volume>& volume() = 0;

    virtual const core::Property<std::tuple<Track::UriType, Track::MetaData>>& track_meta_data() const = 0;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_ENGINE_H_
