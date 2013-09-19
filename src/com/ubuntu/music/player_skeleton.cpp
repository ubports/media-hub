/*
 * Copyright © 2013 Canonical Ltd.
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

#include <com/ubuntu/music/property.h>

#include "player_skeleton.h"
#include "the_session_bus.h"

#include "mpris/player.h"

#include <org/freedesktop/dbus/stub.h>
#include <org/freedesktop/dbus/traits/service.h>

namespace dbus = org::freedesktop::dbus;
namespace music = com::ubuntu::music;

namespace
{
template<typename T, typename DBusProperty>
struct PropertyStub : public music::Property<T>
{
    typedef music::Property<T> super;

    PropertyStub(const std::shared_ptr<dbus::Property<DBusProperty>>& dbus_property)
            : super(),
              dbus_property(dbus_property)
    {
    }

    const T& get() const
    {
        super::mutable_get() = dbus_property->value();
        return super::get();
    }

    void set(const T& value)
    {
        dbus_property->value(value);
        super::set(value);
    }

    std::shared_ptr<dbus::Property<DBusProperty>> dbus_property;
};

template<>
struct PropertyStub<music::Player::PlaybackStatus, mpris::Player::Properties::PlaybackStatus> : public music::Property<music::Player::PlaybackStatus>
{
    typedef music::Property<music::Player::PlaybackStatus> super;

    static const std::map<music::Player::PlaybackStatus, std::string>& playback_status_lut()
    {
        static const std::map<music::Player::PlaybackStatus, std::string> lut =
        {
            {music::Player::PlaybackStatus::playing, "playing"},
            {music::Player::PlaybackStatus::paused, "paused"},
            {music::Player::PlaybackStatus::stopped, "stopped"}
        };

        return lut;
    }

    static const std::map<std::string, music::Player::PlaybackStatus>& reverse_playback_status_lut()
    {
        static const std::map<std::string, music::Player::PlaybackStatus> lut =
        {
            {"playing", music::Player::PlaybackStatus::playing},
            {"paused", music::Player::PlaybackStatus::paused},
            {"stopped", music::Player::PlaybackStatus::stopped}
        };

        return lut;
    }

    PropertyStub(const std::shared_ptr<dbus::Property<mpris::Player::Properties::PlaybackStatus>>& dbus_property)
            : super(),
              dbus_property(dbus_property)
    {
    }

    const music::Player::PlaybackStatus& get() const
    {
        auto value = dbus_property->value();
        super::mutable_get() = reverse_playback_status_lut().at(value);
        return super::get();
    }

    void set(const music::Player::PlaybackStatus& value)
    {
        dbus_property->value(playback_status_lut().at(value));
        super::set(value);
    }

    std::shared_ptr<dbus::Property<mpris::Player::Properties::PlaybackStatus>> dbus_property;
};

template<>
struct PropertyStub<music::Player::LoopStatus, mpris::Player::Properties::LoopStatus> : public music::Property<music::Player::LoopStatus>
{
    typedef music::Property<music::Player::LoopStatus> super;

    static const std::map<music::Player::LoopStatus, std::string>& loop_status_lut()
    {
        static const std::map<music::Player::LoopStatus, std::string> lut =
        {
            {music::Player::LoopStatus::none, "none"},
            {music::Player::LoopStatus::track, "track"},
            {music::Player::LoopStatus::playlist, "playlist"}
        };

        return lut;
    }

    static const std::map<std::string, music::Player::LoopStatus>& reverse_loop_status_lut()
    {
        static const std::map<std::string, music::Player::LoopStatus> lut =
        {
            {"none", music::Player::LoopStatus::none},
            {"track", music::Player::LoopStatus::track},
            {"playlist", music::Player::LoopStatus::playlist}
        };

        return lut;
    }

    PropertyStub(const std::shared_ptr<dbus::Property<mpris::Player::Properties::LoopStatus>>& dbus_property)
            : super(),
              dbus_property(dbus_property)
    {
    }

    const music::Player::LoopStatus& get() const
    {
        auto value = dbus_property->value();
        super::mutable_get() = reverse_loop_status_lut().at(value);
        return super::get();
    }

    void set(const music::Player::LoopStatus& value)
    {
        dbus_property->value(loop_status_lut().at(value));
        super::set(value);
    }

    std::shared_ptr<dbus::Property<mpris::Player::Properties::LoopStatus>> dbus_property;
};
}

struct music::PlayerSkeleton::Private
{
    void handle_next(DBusMessage*)
    {
        impl->next();
    }

    void handle_previous(DBusMessage*)
    {
        impl->previous();
    }

    void handle_pause(DBusMessage*)
    {
        impl->pause();
    }

    void handle_playpause(DBusMessage*)
    {
    }

    void handle_stop(DBusMessage*)
    {
        impl->stop();
    }

    void handle_play(DBusMessage*)
    {
        impl->play();
    }

    void handle_seek(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        uint64_t ticks;
        in->reader() >> ticks; 
        impl->seek_to(std::chrono::microseconds(ticks));
    }

    void handle_set_position(DBusMessage*)
    {
    }

    void handle_open_uri(DBusMessage* msg)
    {
        auto in = dbus::Message::from_raw_message(msg);
        Track::UriType uri;
        in->reader() >> uri;

        auto reply = dbus::Message::make_method_return(msg);
        reply->writer() << impl->open_uri(uri);
        bus->send(reply->get());
    }

    music::Player* impl;
    dbus::Bus::Ptr bus;
    dbus::Object::Ptr object;
    struct
    {
        PropertyStub<bool, mpris::Player::Properties::CanPlay> can_play;
        PropertyStub<bool, mpris::Player::Properties::CanPause> can_pause;
        PropertyStub<bool, mpris::Player::Properties::CanSeek> can_seek;
        PropertyStub<bool, mpris::Player::Properties::CanControl> can_control;
        PropertyStub<bool, mpris::Player::Properties::CanGoNext> can_go_next;
        PropertyStub<bool, mpris::Player::Properties::CanGoPrevious> can_go_previous;
        
        PropertyStub<Player::PlaybackStatus, mpris::Player::Properties::PlaybackStatus> playback_status;
        PropertyStub<Player::LoopStatus, mpris::Player::Properties::LoopStatus> loop_status;
        PropertyStub<Player::PlaybackRate, mpris::Player::Properties::PlaybackRate> playback_rate;
        PropertyStub<bool, mpris::Player::Properties::Shuffle> is_shuffle;
        PropertyStub<Track::MetaData, mpris::Player::Properties::MetaData> meta_data_for_current_track;
        PropertyStub<Player::Volume, mpris::Player::Properties::Volume> volume;
        PropertyStub<Player::PlaybackRate, mpris::Player::Properties::MinimumRate> minimum_playback_rate;
        PropertyStub<Player::PlaybackRate, mpris::Player::Properties::MaximumRate> maximum_playback_rate;        
    } properties;

    /*struct
    {
        std::shared_ptr<dbus::Signal<mpris::Player::Signals::Seeked, uint64_t>> seeked;
        } signals;*/

};

music::PlayerSkeleton::PlayerSkeleton(
    const org::freedesktop::dbus::types::ObjectPath& session_path)
        : dbus::Skeleton<Player>(the_session_bus()),
          d(new Private{
                  this, 
                  the_session_bus(), 
                  access_service()->add_object_for_path(session_path),
                  {                      
                      d->object->get_property<mpris::Player::Properties::CanPlay>(),
                              d->object->get_property<mpris::Player::Properties::CanPause>(),
                              d->object->get_property<mpris::Player::Properties::CanSeek>(),
                              d->object->get_property<mpris::Player::Properties::CanControl>(),
                              d->object->get_property<mpris::Player::Properties::CanGoNext>(),
                              d->object->get_property<mpris::Player::Properties::CanGoPrevious>(),
                              d->object->get_property<mpris::Player::Properties::PlaybackStatus>(),
                              d->object->get_property<mpris::Player::Properties::LoopStatus>(),
                              d->object->get_property<mpris::Player::Properties::PlaybackRate>(),
                              d->object->get_property<mpris::Player::Properties::Shuffle>(),
                              d->object->get_property<mpris::Player::Properties::MetaData>(),
                              d->object->get_property<mpris::Player::Properties::Volume>(),
                              d->object->get_property<mpris::Player::Properties::MinimumRate>(),
                              d->object->get_property<mpris::Player::Properties::MaximumRate>()
                              }

                  })
{
    d->object->install_method_handler<mpris::Player::Next>(
        std::bind(&Private::handle_next,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Previous>(
        std::bind(&Private::handle_previous,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Pause>(
        std::bind(&Private::handle_pause,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Stop>(
        std::bind(&Private::handle_stop,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Play>(
        std::bind(&Private::handle_play,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::Seek>(
        std::bind(&Private::handle_seek,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::SetPosition>(
        std::bind(&Private::handle_set_position,
                  std::ref(d),
                  std::placeholders::_1));
    d->object->install_method_handler<mpris::Player::OpenUri>(
        std::bind(&Private::handle_open_uri,
                  std::ref(d),
                  std::placeholders::_1));
}

music::PlayerSkeleton::~PlayerSkeleton()
{
}

const music::Property<bool>& music::PlayerSkeleton::can_play() const 
{
    return d->properties.can_play;
}

const music::Property<bool>& music::PlayerSkeleton::can_pause() const
{
    return d->properties.can_pause;
}

const music::Property<bool>& music::PlayerSkeleton::can_seek() const
{
    return d->properties.can_seek;
}

const music::Property<bool>& music::PlayerSkeleton::can_go_previous() const
{
    return d->properties.can_go_previous;
}

const music::Property<bool>& music::PlayerSkeleton::can_go_next() const
{
    return d->properties.can_go_next;
}

const music::Property<music::Player::PlaybackStatus>& music::PlayerSkeleton::playback_status() const
{
    return d->properties.playback_status;
}

const music::Property<music::Player::LoopStatus>& music::PlayerSkeleton::loop_status() const
{
    return d->properties.loop_status;
}

const music::Property<music::Player::PlaybackRate>& music::PlayerSkeleton::playback_rate() const
{
    return d->properties.playback_rate;
}

const music::Property<bool>& music::PlayerSkeleton::is_shuffle() const
{
    return d->properties.is_shuffle;
}

const music::Property<music::Track::MetaData>& music::PlayerSkeleton::meta_data_for_current_track() const
{
    return d->properties.meta_data_for_current_track;
}

const music::Property<music::Player::Volume>& music::PlayerSkeleton::volume() const
{
    return d->properties.volume;
}

const music::Property<music::Player::PlaybackRate>& music::PlayerSkeleton::minimum_playback_rate() const
{
    return d->properties.minimum_playback_rate;
}

const music::Property<music::Player::PlaybackRate>& music::PlayerSkeleton::maximum_playback_rate() const
{
    return d->properties.maximum_playback_rate;
}

music::Property<music::Player::LoopStatus>& music::PlayerSkeleton::loop_status()
{
    return d->properties.loop_status;
}

music::Property<music::Player::PlaybackRate>& music::PlayerSkeleton::playback_rate()
{
    return d->properties.playback_rate;
}

music::Property<bool>& music::PlayerSkeleton::is_shuffle()
{
    return d->properties.is_shuffle;
}

music::Property<music::Player::Volume>& music::PlayerSkeleton::volume()
{
    return d->properties.volume;
}

music::Property<bool>& music::PlayerSkeleton::can_play()
{
    return d->properties.can_play;
}

music::Property<bool>& music::PlayerSkeleton::can_pause()
{
    return d->properties.can_pause;
}

music::Property<bool>& music::PlayerSkeleton::can_seek()
{
    return d->properties.can_seek;
}

music::Property<bool>& music::PlayerSkeleton::can_go_previous()
{
    return d->properties.can_go_previous;
}

music::Property<bool>& music::PlayerSkeleton::can_go_next()
{
    return d->properties.can_go_next;
}

music::Property<music::Track::MetaData>& music::PlayerSkeleton::meta_data_for_current_track()
{
    return d->properties.meta_data_for_current_track;
}

music::Property<music::Player::PlaybackRate>& music::PlayerSkeleton::minimum_playback_rate()
{
    return d->properties.minimum_playback_rate;
}

music::Property<music::Player::PlaybackRate>& music::PlayerSkeleton::maximum_playback_rate()
{
    return d->properties.maximum_playback_rate;
}
