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

#include "video_sink_p.h"

#include "egl_video_sink.h"
#include "hybris_video_sink.h"

using namespace lomiri::MediaHub;

class NullVideoSink: public VideoSink
{
    Q_OBJECT

public:
    NullVideoSink(QObject *parent):
        VideoSink(new VideoSinkPrivate(), parent)
    {
    }

    bool swapBuffers() override { return true; }
};

VideoSink::VideoSink(VideoSinkPrivate *d, QObject *parent):
    QObject(parent),
    d_ptr(d)
{
}

VideoSink::~VideoSink() = default;

const QMatrix4x4 &VideoSink::transformationMatrix() const
{
    Q_D(const VideoSink);
    return d->m_transformationMatrix;
}

VideoSinkFactory lomiri::MediaHub::createVideoSinkFactory(
    PlayerKey key,
    AVBackend::Backend backend)
{
    switch (backend) {
    case AVBackend::Backend::Hybris:
        qDebug() << "Using Hybris video sink";
        return HybrisVideoSink::createFactory(key);
    case AVBackend::Backend::Mir:
        qDebug() << "Using Mir/EGL video sink";
        return EglVideoSink::createFactory(key);
    case AVBackend::Backend::None:
        qDebug() << "No video backend selected. "
            "Video rendering functionality won't work.";
        return [](uint32_t textureId, QObject *parent) {
            Q_UNUSED(textureId);
            return new NullVideoSink(parent);
        };
    default:
        qWarning() << "Invalid or no A/V backend specified, "
            "using \"hybris\" as a default." << backend;
        return HybrisVideoSink::createFactory(key);
    }
}

#include "video_sink.moc"
