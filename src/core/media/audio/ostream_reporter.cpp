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

#include <core/media/audio/ostream_reporter.h>

namespace audio = core::ubuntu::media::audio;

audio::OStreamReporter::OStreamReporter(std::ostream &out) : out{out}
{
}

void audio::OStreamReporter::connected_to_pulse_audio()
{
    out << "Connection to PulseAudio has been successfully established." << std::endl;
}

void audio::OStreamReporter::query_for_default_sink_failed()
{
    out << "Query for default sink failed." << std::endl;
}

void audio::OStreamReporter::query_for_default_sink_finished(const std::string& sink_name)
{
    out << "Default PulseAudio sync has been identified: " << sink_name << std::endl;
}

void audio::OStreamReporter::query_for_sink_info_finished(const std::string& name, std::uint32_t index, const std::set<Port>& known_ports)
{
    out << "PulseAudio sink details for " << name << " with index " << index << " is available:" << std::endl;
    for (const auto& port : known_ports)
    {
        if (port.is_monitored)
            out << "  " << port.description << ": " << std::boolalpha << port.is_available << "\n";
    }
}
void audio::OStreamReporter::sink_event_with_index(std::uint32_t index)
{
    out << "PulseAudio event for sink with index " << index << " received." << std::endl;
}
