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
#ifndef CORE_UBUNTU_MEDIA_VIDEO_SINK_H_
#define CORE_UBUNTU_MEDIA_VIDEO_SINK_H_

#include <core/signal.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
namespace video
{
/** @brief A video sink abstracts a queue of buffers, that receives
  * a stream of decoded video buffers from an arbitrary source.
  */
struct Sink
{
    /** @brief To save us some typing. */
    typedef std::shared_ptr<Sink> Ptr;

    /** @cond */
    Sink() = default;
    Sink(const Sink&) = delete;
    virtual ~Sink() = default;

    Sink& operator=(const Sink&) = delete;
    /** @endcond */

    /**
     * @brief The signal is emitted whenever a new frame is available and a subsequent
     * call to swap_buffers will not block and return true.
     */
    virtual const core::Signal<void>& frame_available() const = 0;

    /**
     * @brief Queries the 4x4 transformation matrix for the current frame, placing the data into 'matrix'.
     * @param matrix [out] The destination array representing the matrix in column-major format.
     *                     We expect at least 4*4 float elements in the array.
     * @returns true iff the data has been set. Returns false and leaves 'matrix' unchanged in case of issues.
     */
    virtual bool transformation_matrix(float* matrix) const = 0;

    /**
     * @brief Releases the current buffer, and consumes the next buffer in the queue,
     * making it available for consumption by consumers of this API in an
     * implementation-specific way. Clients will usually rely on a GL texture
     * to receive the latest buffer.
     */
    virtual bool swap_buffers() const = 0;
};
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_VIDEO_SINK_H_
