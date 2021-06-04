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
#ifndef CORE_UBUNTU_MEDIA_AUDIO_OUTPUT_OBSERVER_H_
#define CORE_UBUNTU_MEDIA_AUDIO_OUTPUT_OBSERVER_H_

#include <QObject>

#include <iosfwd>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace audio
{
// All known states of an audio output.
enum class OutputState
{
    // The output is via a private earpiece (i.e. headphones, headset)
    Earpiece,
    // The output is via the internal speaker.
    Speaker,
    // The output is via an external device (a2dp, etc)
    External,
};

// Models observation of audio outputs of a device.
// Right now, we are only interested in monitoring the
// state of external outputs to react accordingly if
// wired or bluetooth outputs are connected/disconnected.
class OutputObserverPrivate;
class OutputObserver: public QObject
{
    Q_OBJECT

public:
    OutputObserver(QObject *parent = nullptr);
    virtual ~OutputObserver();

    OutputState outputState() const;

Q_SIGNALS:
    void outputStateChanged();

protected:
    OutputObserver(const OutputObserver&) = delete;
    OutputObserver& operator=(const OutputObserver&) = delete;

private:
    Q_DECLARE_PRIVATE(OutputObserver)
    QScopedPointer<OutputObserverPrivate> d_ptr;
};

// Pretty prints the given state to the given output stream.
std::ostream& operator<<(std::ostream&, OutputState);

}
}
}
}

#endif // CORE_UBUNTU_MEDIA_AUDIO_OUTPUT_OBSERVER_H_
