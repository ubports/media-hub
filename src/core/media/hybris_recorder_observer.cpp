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

namespace media = core::ubuntu::media;

#if defined(MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
#include <hybris/media/media_recorder_layer.h>

struct media::HybrisRecorderObserver::Private
{
    struct Holder
    {
        std::weak_ptr<media::HybrisRecorderObserver::Private> wp;
    };

    // The fringe that we hand over to hybris.
    static void on_media_recording_state_changed(bool started, void* context)
    {
        if (auto holder = static_cast<Holder*>(context))
        {
            if (auto sp = holder->wp.lock())
            {
                sp->recording_state = started ? media::RecordingState::started : media::RecordingState::stopped;
            }
        }
    }

    // TODO: We have no way of freeing the observer and thus leak
    // an instance here.
    MediaRecorderObserver* observer
    {
        android_media_recorder_observer_new()
    };

    core::Property<media::RecordingState> recording_state
    {
        media::RecordingState::stopped
    };

    Holder* holder
    {
        nullptr
    };
};

media::HybrisRecorderObserver::HybrisRecorderObserver() : d{new Private{}}
{
    android_media_recorder_observer_set_cb(
                d->observer,
                &Private::on_media_recording_state_changed,
                d->holder = new Private::Holder{d});
}

media::HybrisRecorderObserver::~HybrisRecorderObserver()
{
    // We first reset the context of the callback.
    android_media_recorder_observer_set_cb(
                d->observer,
                &Private::on_media_recording_state_changed,
                nullptr);

    delete d->holder;
}

const core::Property<media::RecordingState>& media::HybrisRecorderObserver::recording_state() const
{
    return d->recording_state;
}

media::RecorderObserver::Ptr media::HybrisRecorderObserver::create()
{
    return media::RecorderObserver::Ptr{new media::HybrisRecorderObserver{}};
}
#else  // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER
media::RecorderObserver::Ptr media::HybrisRecorderObserver::create()
{
    throw std::logic_error
    {
        "Hybris-based recorder observer implementation not supported on this platform."
    };
}
#endif // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER
