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
 *              Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */
#ifndef CORE_UBUNTU_MEDIA_AUDIO_PULSE_AUDIO_OUTPUT_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_AUDIO_PULSE_AUDIO_OUTPUT_OBSERVER_H_

#include <core/media/audio/output_observer.h>

#include <iosfwd>
#include <memory>
#include <regex>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace audio
{
// Implements the audio::OutputObserver interface
// relying on pulse to query the connected ports
// of the primary card of the system.
class PulseAudioOutputObserver : public OutputObserver
{
public:
    // Save us some typing.
    typedef std::shared_ptr<PulseAudioOutputObserver> Ptr;

    // Construction time arguments go here
    struct Configuration
    {
        // Name of the sink that we should consider.
        std::string sink
        {
            // A special value that requests the implementation to
            // query pulseaudio for the default configured sink.
            "query.from.server"
        };
        // Output port name patterns that should be observed on the configured sink.
        // All patterns have to be valid regular expressions.
        std::vector<std::regex> output_port_patterns
        {
            // Any port is considered with this special value.
            std::regex{".+"}
        };
    };

    // Constructs a new instance, or throws std::runtime_error
    // if connection to pulseaudio fails.
    PulseAudioOutputObserver(const Configuration&);

    // We provide the name of the sink we are connecting to as a
    // getable/observable property. This is specifically meant for
    // consumption by test code.
    const core::Property<std::string>& sink() const;
    // The set of ports that have been identified on the configured sink.
    // Specifically meant for consumption by test code.
    const core::Property<std::set<std::string>>& known_ports() const;
    // Getable/observable property holding the state of external outputs.
    const core::Property<OutputState>& external_output_state() const override;

private:
    struct Private;
    std::shared_ptr<Private> d;
};
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_AUDIO_PULSE_AUDIO_OUTPUT_OBSERVER_H_
