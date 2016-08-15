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
#ifndef CORE_UBUNTU_MEDIA_VIDEO_PLATFORM_DEFAULT_SINK_H_
#define CORE_UBUNTU_MEDIA_VIDEO_PLATFORM_DEFAULT_SINK_H_

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
// A functor that allows for creating actual sinks given a texture id.
typedef std::function<Sink::Ptr(std::uint32_t)> SinkFactory;
// Returns the platform default sink factory for the player instance identified by the given key.
SinkFactory make_platform_default_sink_factory(const Player::PlayerKey& key,
                                               const AVBackend::Backend b);
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_VIDEO_PLATFORM_DEFAULT_SINK_H_
