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

#ifndef CORE_UBUNTU_MEDIA_AUDIO_OUTPUT_OBSERVER_P_H_
#define CORE_UBUNTU_MEDIA_AUDIO_OUTPUT_OBSERVER_P_H_

#include <core/media/audio/output_observer.h>

namespace core {
namespace ubuntu {
namespace media {
namespace audio {

class OutputObserverPrivate
{
    Q_DECLARE_PUBLIC(OutputObserver)

public:
    OutputObserverPrivate(OutputObserver *q):
        m_outputState(OutputState::Speaker),
        q_ptr(q)
    {
    }

    OutputState outputState() const { return m_outputState; }

protected:
    void setOutputState(OutputState state) {
        Q_Q(OutputObserver);
        if (state == m_outputState) return;
        m_outputState = state;
        Q_EMIT q->outputStateChanged();
    }

private:
    OutputState m_outputState;
    OutputObserver *q_ptr;
};

}}}} // namespace

#endif // CORE_UBUNTU_MEDIA_AUDIO_OUTPUT_OBSERVER_P_H_
