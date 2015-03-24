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

#ifndef CORE_UBUNTU_MEDIA_RECORDER_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_RECORDER_OBSERVER_H_

#include <core/property.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
// All known states of the recorder
enum class RecordingState
{
    // No active recording
    stopped,
    // We have an active recording session
    started
};

// A RecorderObserver allows for monitoring the recording state
// of the service.
struct RecorderObserver
{
    // To save us some typing.
    typedef std::shared_ptr<RecorderObserver> Ptr;

    RecorderObserver() = default;
    RecorderObserver(const RecorderObserver&) = delete;
    virtual ~RecorderObserver() = default;
    RecorderObserver& operator=(const RecorderObserver&) = delete;

    // Getable/observable property describing the recording state of the system.
    virtual const core::Property<RecordingState>& recording_state() const = 0;
};

// Creates an instance of interface RecorderObserver relying on
// default services offered by the platform we are currently running on.
RecorderObserver::Ptr make_platform_default_recorder_observer();
}
}
}

#endif // CORE_UBUNTU_MEDIA_RECORDER_OBSERVER_H_
