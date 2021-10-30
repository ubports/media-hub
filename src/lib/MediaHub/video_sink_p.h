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

#ifndef LOMIRI_MEDIAHUB_VIDEO_SINK_P_H
#define LOMIRI_MEDIAHUB_VIDEO_SINK_P_H

#include "video_sink.h"

#include "dbus_constants.h"
#include "player.h"

#include <functional>

namespace lomiri {
namespace MediaHub {

struct AVBackend
{
    enum Backend
    {
        None = DBusConstants::Backend::None,
        Hybris = DBusConstants::Backend::Hybris,
        Mir = DBusConstants::Backend::Mir,
    };
};

typedef std::function<VideoSink *(uint32_t textureId, QObject *parent)>
    VideoSinkFactory;
typedef uint32_t PlayerKey;

VideoSinkFactory createVideoSinkFactory(PlayerKey key,
                                        AVBackend::Backend backend);

class VideoSinkPrivate
{
public:
    QMatrix4x4 m_transformationMatrix;
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_VIDEO_SINK_P_H
