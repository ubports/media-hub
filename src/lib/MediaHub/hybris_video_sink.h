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

#ifndef LOMIRI_MEDIAHUB_HYBRIS_VIDEO_SINK_H
#define LOMIRI_MEDIAHUB_HYBRIS_VIDEO_SINK_H

#include "player.h"
#include "video_sink_p.h"

namespace lomiri {
namespace MediaHub {

typedef void* GLConsumerWrapperHybris;

class HybrisVideoSinkPrivate;
class HybrisVideoSink: public VideoSink
{
    Q_OBJECT

public:
    static VideoSinkFactory createFactory(PlayerKey playerKey);
    virtual ~HybrisVideoSink();

    bool swapBuffers() override;

private:
    HybrisVideoSink(uint32_t gl_texture, QObject *parent);
    void onFrameAvailable();
    Q_DECLARE_PRIVATE(HybrisVideoSink);
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_HYBRIS_VIDEO_SINK_H
