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

#ifndef COM_UBUNTU_MUSIC_PLAYER_SKELETON_H_
#define COM_UBUNTU_MUSIC_PLAYER_SKELETON_H_

#include <core/media/player.h>

#include "player_traits.h"

#include "mpris/player.h"

#include <org/freedesktop/dbus/skeleton.h>
#include <org/freedesktop/dbus/types/object_path.h>

#include <memory>

namespace com
{
namespace ubuntu
{
namespace music
{
class Service;

class PlayerSkeleton : public org::freedesktop::dbus::Skeleton<com::ubuntu::music::Player>
{
  public:
    ~PlayerSkeleton();

    virtual const Property<bool>& can_play() const;
    virtual const Property<bool>& can_pause() const;
    virtual const Property<bool>& can_seek() const;
    virtual const Property<bool>& can_go_previous() const;
    virtual const Property<bool>& can_go_next() const;
    virtual const Property<PlaybackStatus>& playback_status() const;    
    virtual const Property<LoopStatus>& loop_status() const;
    virtual const Property<PlaybackRate>& playback_rate() const;
    virtual const Property<bool>& is_shuffle() const;
    virtual const Property<Track::MetaData>& meta_data_for_current_track() const;
    virtual const Property<Volume>& volume() const;
    virtual const Property<PlaybackRate>& minimum_playback_rate() const;
    virtual const Property<PlaybackRate>& maximum_playback_rate() const;
        
    virtual Property<LoopStatus>& loop_status();
    virtual Property<PlaybackRate>& playback_rate();
    virtual Property<bool>& is_shuffle();
    virtual Property<Volume>& volume();
    
    virtual const Signal<uint64_t>& seeked_to() const;

  protected:
    PlayerSkeleton(const org::freedesktop::dbus::types::ObjectPath& session_path);

    virtual Property<PlaybackStatus>& playback_status();
    virtual Property<bool>& can_play();
    virtual Property<bool>& can_pause();
    virtual Property<bool>& can_seek();
    virtual Property<bool>& can_go_previous();
    virtual Property<bool>& can_go_next();
    virtual Property<Track::MetaData>& meta_data_for_current_track();
    virtual Property<PlaybackRate>& minimum_playback_rate();
    virtual Property<PlaybackRate>& maximum_playback_rate();

  private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
}
}

#endif // COM_UBUNTU_MUSIC_PLAYER_SKELETON_H_
