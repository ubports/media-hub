/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
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
 */

#ifndef CORE_UBUNTU_MEDIA_HYBRIS_RECORDER_OBSERVER_PRIVATE_H_
#define CORE_UBUNTU_MEDIA_HYBRIS_RECORDER_OBSERVER_PRIVATE_H_

#include <core/media/recorder_observer.h>

namespace core {
namespace ubuntu {
namespace media {

class RecorderObserverPrivate
{
    Q_DECLARE_PUBLIC(RecorderObserver)

public:
    RecorderObserverPrivate(RecorderObserver *q):
        q_ptr(q)
    {
    }

    RecordingState recordingState() const {
        return m_recordingState;
    }

protected:
    void setRecordingState(RecordingState state) {
        Q_Q(RecorderObserver);
        if (state == m_recordingState) return;
        m_recordingState = state;
        Q_EMIT q->recordingStateChanged();
    }

private:
    RecordingState m_recordingState;
    RecorderObserver *q_ptr;
};

}}} // namespace

#endif // CORE_UBUNTU_MEDIA_HYBRIS_RECORDER_OBSERVER_PRIVATE_H_
