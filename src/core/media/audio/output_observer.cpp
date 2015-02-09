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

#include <core/media/audio/output_observer.h>

#include <core/media/audio/pulse_audio_output_observer.h>
#include <core/media/audio/ostream_reporter.h>

#include <iostream>

namespace audio = core::ubuntu::media::audio;

std::ostream& audio::operator<<(std::ostream& out, audio::OutputState state)
{
    switch (state)
    {
    case audio::OutputState::connected:
        return out << "OutputState::connected";
    case audio::OutputState::disconnected:
        return out << "OutputState::disconnected";
    }

    return out;
}

audio::OutputObserver::Ptr audio::make_platform_default_output_observer()
{
    audio::PulseAudioOutputObserver::Configuration config;
    config.output_port_patterns = {std::regex{"output-wired_head.*"}};
    config.reporter = std::make_shared<audio::OStreamReporter>();
    return std::make_shared<audio::PulseAudioOutputObserver>(config);
}
