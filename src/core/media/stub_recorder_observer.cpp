/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#include <core/media/stub_recorder_observer.h>

namespace media = core::ubuntu::media;

media::StubRecorderObserver::StubRecorderObserver()
 : current_recording_state(media::RecordingState::stopped)
{
}

media::StubRecorderObserver::~StubRecorderObserver()
{
}

const core::Property<media::RecordingState>& media::StubRecorderObserver::recording_state() const
{
    return current_recording_state;
}

media::RecorderObserver::Ptr media::StubRecorderObserver::create()
{
    return media::RecorderObserver::Ptr{new media::StubRecorderObserver{}};
}
