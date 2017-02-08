/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Alfonso Sanchez-Beato <alfonso.sanchez-beato@canonical.com>
 */
#ifndef CORE_UBUNTU_MEDIA_VIDEO_EGL_SINK_H_
#define CORE_UBUNTU_MEDIA_VIDEO_EGL_SINK_H_

#include <core/media/video/sink.h>

#include <core/media/player.h>

#include <functional>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace video
{
class EglSink : public video::Sink
{
public:
    // Returns a factory functor that allows for creating actual sink instances.
    static std::function<video::Sink::Ptr(std::uint32_t)>
        factory_for_key(const media::Player::PlayerKey&);

    // Need this to avoid std::unique_ptr complaining about forward-declared Private.
    ~EglSink();

    // The signal is emitted whenever a new frame is available and a subsequent
    // call to swap_buffers will not block and return true.
    const core::Signal<void>& frame_available() const override;

    // Queries the transformation matrix for the current frame, placing the data into 'matrix'
    // and returns true or returns false and leaves 'matrix' unchanged in case
    // of issues.
    bool transformation_matrix(float* matrix) const override;

    // Releases the current buffer, and consumes the next buffer in the queue,
    // making it available for consumption by consumers of this API in an
    // implementation-specific way. Clients will usually rely on a GL texture
    // to receive the latest buffer.
    bool swap_buffers() const override;

private:
    // Creates a new instance for the given gl texture, connecting to the remote part known
    // under the given key or throw in case of issues.
    EglSink(std::uint32_t gl_texture, const media::Player::PlayerKey key);

    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_VIDEO_EGL_SINK_H_
