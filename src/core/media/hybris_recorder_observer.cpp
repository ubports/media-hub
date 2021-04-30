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

#include <core/media/hybris_recorder_observer.h>

#include <hybris/media/media_recorder_layer.h>

namespace media = core::ubuntu::media;

struct media::HybrisRecorderObserver::Private
{
    // The fringe that we hand over to hybris.
    static void on_media_recording_state_changed(bool started, void* context)
    {
        if (auto thiz = static_cast<HybrisRecorderObserver*>(context))
        {
            thiz->setRecordingState(started ? media::RecordingState::started : media::RecordingState::stopped);
        }
    }

    // TODO: We have no way of freeing the observer and thus leak
    // an instance here.
    MediaRecorderObserver* observer
    {
        android_media_recorder_observer_new()
    };
};

media::HybrisRecorderObserver::HybrisRecorderObserver(RecorderObserver *q):
    RecorderObserverPrivate(q),
    d{new Private{}}
{
    android_media_recorder_observer_set_cb(
                d->observer,
                &Private::on_media_recording_state_changed,
                this);
}

media::HybrisRecorderObserver::~HybrisRecorderObserver()
{
    // We first reset the context of the callback.
    android_media_recorder_observer_set_cb(
                d->observer,
                &Private::on_media_recording_state_changed,
                nullptr);
}
