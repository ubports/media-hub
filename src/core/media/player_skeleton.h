/**
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

#ifndef CORE_UBUNTU_MEDIA_PLAYER_SKELETON_H_
#define CORE_UBUNTU_MEDIA_PLAYER_SKELETON_H_

#include <core/media/player.h>

#include "player_traits.h"

#include "mpris/player.h"

#include <core/dbus/skeleton.h>
#include <core/dbus/types/object_path.h>

#include <memory>

namespace core
{
namespace ubuntu
{
namespace media
{
class Service;

class PlayerSkeleton : public core::dbus::Skeleton<core::ubuntu::media::Player>
{
  public:
    ~PlayerSkeleton();

    virtual const core::Property<bool>& can_play() const;
    virtual const core::Property<bool>& can_pause() const;
    virtual const core::Property<bool>& can_seek() const;
    virtual const core::Property<bool>& can_go_previous() const;
    virtual const core::Property<bool>& can_go_next() const;
    virtual const core::Property<PlaybackStatus>& playback_status() const;
    virtual const core::Property<LoopStatus>& loop_status() const;
    virtual const core::Property<PlaybackRate>& playback_rate() const;
    virtual const core::Property<bool>& is_shuffle() const;
    virtual const core::Property<Track::MetaData>& meta_data_for_current_track() const;
    virtual const core::Property<Volume>& volume() const;
    virtual const core::Property<PlaybackRate>& minimum_playback_rate() const;
    virtual const core::Property<PlaybackRate>& maximum_playback_rate() const;
    virtual const core::Property<uint64_t>& position() const;

    virtual core::Property<LoopStatus>& loop_status();
    virtual core::Property<PlaybackRate>& playback_rate();
    virtual core::Property<bool>& is_shuffle();
    virtual core::Property<Volume>& volume();

    virtual const core::Signal<uint64_t>& seeked_to() const;

  protected:
    PlayerSkeleton(const core::dbus::types::ObjectPath& session_path);

    virtual core::Property<PlaybackStatus>& playback_status();
    virtual core::Property<bool>& can_play();
    virtual core::Property<bool>& can_pause();
    virtual core::Property<bool>& can_seek();
    virtual core::Property<bool>& can_go_previous();
    virtual core::Property<bool>& can_go_next();
    virtual core::Property<Track::MetaData>& meta_data_for_current_track();
    virtual core::Property<PlaybackRate>& minimum_playback_rate();
    virtual core::Property<PlaybackRate>& maximum_playback_rate();
    virtual core::Property<uint64_t>& position();

  private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // CORE_UBUNTU_MEDIA_PLAYER_SKELETON_H_
