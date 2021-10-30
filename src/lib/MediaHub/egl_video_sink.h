/*
 * Copyright © 2021 UBports Foundation.
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

#ifndef LOMIRI_MEDIAHUB_EGL_VIDEO_SINK_H
#define LOMIRI_MEDIAHUB_EGL_VIDEO_SINK_H

#include "player.h"
#include "video_sink_p.h"

namespace lomiri {
namespace MediaHub {

class EglVideoSinkPrivate;
class EglVideoSink: public VideoSink
{
    Q_OBJECT

public:
    static VideoSinkFactory createFactory(PlayerKey playerKey);
    virtual ~EglVideoSink();

    bool swapBuffers() override;

private:
    EglVideoSink(uint32_t gl_texture, PlayerKey key, QObject *parent);
    Q_DECLARE_PRIVATE(EglVideoSink);
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_EGL_VIDEO_SINK_H
