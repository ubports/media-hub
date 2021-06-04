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

using namespace audio;

std::ostream& audio::operator<<(std::ostream& out, audio::OutputState state)
{
    switch (state)
    {
    case audio::OutputState::Earpiece:
        return out << "OutputState::Earpiece";
    case audio::OutputState::Speaker:
        return out << "OutputState::Speaker";
    case audio::OutputState::External:
        return out << "OutputState::External";
    }

    return out;
}

OutputObserver::OutputObserver(QObject *parent):
    QObject(parent),
    d_ptr(new PulseAudioOutputObserver("sink.primary",
                                       {"output-wired_head.*|output-a2dp_headphones"},
                                       std::make_shared<audio::OStreamReporter>(),
                                       this))
{
}

OutputObserver::~OutputObserver() = default;

OutputState OutputObserver::outputState() const
{
    Q_D(const OutputObserver);
    return d->outputState();
}
