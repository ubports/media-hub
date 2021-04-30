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

#include "recorder_observer.h"

#include "logging.h"

#include <core/media/player.h>

#include <core/media/hybris_recorder_observer.h>
#include <core/media/stub_recorder_observer.h>

namespace media = core::ubuntu::media;

using namespace media;

namespace {

RecorderObserverPrivate *
make_platform_default_recorder_observer(RecorderObserver *q)
{
    const media::AVBackend::Backend b {media::AVBackend::get_backend_type()};
    switch (b)
    {
        case media::AVBackend::Backend::hybris:
            return new media::HybrisRecorderObserver(q);
        case media::AVBackend::Backend::mir:
        case media::AVBackend::Backend::none:
            MH_WARNING(
                "No video backend selected. Video recording functionality won't work."
            );
            return new media::StubRecorderObserver(q);
        default:
            MH_INFO("Invalid or no A/V backend specified, using \"hybris\" as a default.");
            return new media::HybrisRecorderObserver(q);
    }
}

} // namespace

RecorderObserver::RecorderObserver(QObject *parent):
    QObject(parent),
    d_ptr(make_platform_default_recorder_observer(this))
{
}

RecorderObserver::~RecorderObserver() = default;

RecordingState RecorderObserver::recordingState() const
{
    Q_D(const RecorderObserver);
    return d->recordingState();
}
