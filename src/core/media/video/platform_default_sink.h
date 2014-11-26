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

namespace core
{
namespace ubuntu
{
namespace media
{
namespace video
{
// Returns the platform default video sink for the given parameters. Never returns null, but
// throws in case of issues.
Sink::Ptr make_platform_default_sink(std::uint32_t gl_texture, const Player::PlayerKey& key);
}
}
}
}

#endif // CORE_UBUNTU_MEDIA_VIDEO_PLATFORM_DEFAULT_SINK_H_
