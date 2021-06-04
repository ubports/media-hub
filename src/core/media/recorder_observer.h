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

#include <QObject>
#include <QScopedPointer>

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
class RecorderObserverPrivate;
class RecorderObserver: public QObject
{
    Q_OBJECT

public:
    RecorderObserver(QObject *parent = nullptr);
    RecorderObserver(const RecorderObserver&) = delete;
    virtual ~RecorderObserver();
    RecorderObserver& operator=(const RecorderObserver&) = delete;

    RecordingState recordingState() const;

Q_SIGNALS:
    void recordingStateChanged();

private:
    Q_DECLARE_PRIVATE(RecorderObserver);
    QScopedPointer<RecorderObserverPrivate> d_ptr;
};

}
}
}

#endif // CORE_UBUNTU_MEDIA_RECORDER_OBSERVER_H_
