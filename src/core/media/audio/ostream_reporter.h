/*
 * Copyright © 2014 Canonical Ltd.
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
#ifndef CORE_UBUNTU_MEDIA_AUDIO_OSTREAM_REPORTER_H_
#define CORE_UBUNTU_MEDIA_AUDIO_OSTREAM_REPORTER_H_

#include <core/media/audio/pulse_audio_output_observer.h>

#include <iosfwd>
#include <iostream>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace audio
{
// A PulseAudioOutputObserver::Reporter implementation printing events to
// the configured output stream.
class OStreamReporter : public PulseAudioOutputObserver::Reporter
{
public:
    // Constructs a new reporter instance, outputting events to the given stream.
    OStreamReporter(std::ostream& out = std::cout);

    void connected_to_pulse_audio() override;
    void query_for_default_sink_failed() override;
    void query_for_default_sink_finished(const std::string& sink_name) override;
    void query_for_sink_info_finished(const std::string& name, std::uint32_t index, const std::set<Port>& known_ports) override;
    void sink_event_with_index(std::uint32_t index) override;

private:
    std::ostream& out;
};
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_AUDIO_OUTPUT_OSTREAM_REPORTER_H_
