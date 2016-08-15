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

#ifndef CORE_UBUNTU_MEDIA_STUB_RECORDER_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_STUB_RECORDER_OBSERVER_H_

#include <core/media/recorder_observer.h>

namespace core
{
namespace ubuntu
{
namespace media
{
class StubRecorderObserver : public RecorderObserver
{
public:
    // Creates a new instance of the StubRecorderObserver
    static RecorderObserver::Ptr create();

    ~StubRecorderObserver();

    // Getable/observable property describing the recording state of the system.
    const core::Property<RecordingState>& recording_state() const override;

private:
    StubRecorderObserver();

    core::Property<media::RecordingState> current_recording_state;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_STUB_RECORDER_OBSERVER_H_
