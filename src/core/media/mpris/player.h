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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */

#ifndef MPRIS_PLAYER_H_
#define MPRIS_PLAYER_H_

#include <core/media/player.h>
#include <core/media/track.h>

#include "core/media/codec.h"

#include <core/dbus/bus.h>
#include <core/dbus/macros.h>
#include <core/dbus/object.h>
#include <core/dbus/property.h>
#include <core/dbus/types/any.h>
#include <core/dbus/types/object_path.h>
#include <core/dbus/types/variant.h>

#include <boost/utility/identity_type.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace dbus = core::dbus;

namespace mpris
{
struct Player
{
    static const std::string& name()
    {
        static const std::string s{"org.mpris.MediaPlayer2.Player"};
        return s;
    }

    struct LoopStatus
    {
        LoopStatus() = delete;

        static constexpr const char* none{"None"};
        static constexpr const char* track{"Track"};
        static constexpr const char* playlist{"Playlist"};
    };

    struct PlaybackStatus
    {
        PlaybackStatus() = delete;

        static const char* from(core::ubuntu::media::Player::PlaybackStatus status)
        {
            switch(status)
            {
            case core::ubuntu::media::Player::PlaybackStatus::null:
            case core::ubuntu::media::Player::PlaybackStatus::ready:
            case core::ubuntu::media::Player::PlaybackStatus::stopped:
                return PlaybackStatus::stopped;

            case core::ubuntu::media::Player::PlaybackStatus::playing:
                return PlaybackStatus::playing;
            case core::ubuntu::media::Player::PlaybackStatus::paused:
                return PlaybackStatus::paused;
            }

            return nullptr;
        }

        static constexpr const char* playing{"Playing"};
        static constexpr const char* paused{"Paused"};
        static constexpr const char* stopped{"Stopped"};
    };

    typedef std::map<std::string, core::dbus::types::Variant> Dictionary;

    DBUS_CPP_METHOD_DEF(Next, Player)
    DBUS_CPP_METHOD_DEF(Previous, Player)
    DBUS_CPP_METHOD_DEF(Pause, Player)
    DBUS_CPP_METHOD_DEF(PlayPause, Player)
    DBUS_CPP_METHOD_DEF(Stop, Player)
    DBUS_CPP_METHOD_DEF(Play, Player)
    DBUS_CPP_METHOD_DEF(Seek, Player)
    DBUS_CPP_METHOD_DEF(SetPosition, Player)
    DBUS_CPP_METHOD_DEF(CreateVideoSink, Player)
    DBUS_CPP_METHOD_DEF(Key, Player)
    DBUS_CPP_METHOD_DEF(OpenUri, Player)

    struct Signals
    {
        DBUS_CPP_SIGNAL_DEF(Seeked, Player, std::uint64_t)
        DBUS_CPP_SIGNAL_DEF(EndOfStream, Player, void)
        DBUS_CPP_SIGNAL_DEF(PlaybackStatusChanged, Player, core::ubuntu::media::Player::PlaybackStatus)
    };

    struct Properties
    {
        DBUS_CPP_READABLE_PROPERTY_DEF(PlaybackStatus, Player, std::string)
        DBUS_CPP_READABLE_PROPERTY_DEF(TypedPlaybackStatus, Player, core::ubuntu::media::Player::PlaybackStatus)

        DBUS_CPP_WRITABLE_PROPERTY_DEF(LoopStatus, Player, std::string)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(TypedLoopStatus, Player, core::ubuntu::media::Player::LoopStatus)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(AudioStreamRole, Player, core::ubuntu::media::Player::AudioStreamRole)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(PlaybackRate, Player, double)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Rate, Player, double)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Shuffle, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(MetaData, Player, Dictionary)
        DBUS_CPP_READABLE_PROPERTY_DEF(TypedMetaData, Player, core::ubuntu::media::Track::MetaData)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Volume, Player, double)
        DBUS_CPP_READABLE_PROPERTY_DEF(Position, Player, std::uint64_t)
        DBUS_CPP_READABLE_PROPERTY_DEF(Duration, Player, std::uint64_t)
        DBUS_CPP_READABLE_PROPERTY_DEF(MinimumRate, Player, double)
        DBUS_CPP_READABLE_PROPERTY_DEF(MaximumRate, Player, double)
        DBUS_CPP_READABLE_PROPERTY_DEF(IsVideoSource, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(IsAudioSource, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanGoNext, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanGoPrevious, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanPlay, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanPause, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanSeek, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanControl, Player, bool)
    };

    // Convenience struct to create a skeleton implementation for org.mpris.MediaPlayer2.Player
    struct Skeleton
    {
        // Creation time options go here.
        struct Configuration
        {
            // The bus connection that should be used
            core::dbus::Bus::Ptr bus;
            // The dbus object that should implement org.mpris.MediaPlayer2
            core::dbus::Object::Ptr object;
        };

        Skeleton(const Configuration& configuration)
            : configuration(configuration),
              properties
              {
                  configuration.object->template get_property<mpris::Player::Properties::CanPlay>(),
                  configuration.object->template get_property<mpris::Player::Properties::CanPause>(),
                  configuration.object->template get_property<mpris::Player::Properties::CanSeek>(),
                  configuration.object->template get_property<mpris::Player::Properties::CanControl>(),
                  configuration.object->template get_property<mpris::Player::Properties::CanGoNext>(),
                  configuration.object->template get_property<mpris::Player::Properties::CanGoPrevious>(),
                  configuration.object->template get_property<mpris::Player::Properties::IsVideoSource>(),
                  configuration.object->template get_property<mpris::Player::Properties::IsAudioSource>(),
                  configuration.object->template get_property<mpris::Player::Properties::PlaybackStatus>(),
                  configuration.object->template get_property<mpris::Player::Properties::TypedPlaybackStatus>(),
                  configuration.object->template get_property<mpris::Player::Properties::LoopStatus>(),
                  configuration.object->template get_property<mpris::Player::Properties::TypedLoopStatus>(),
                  configuration.object->template get_property<mpris::Player::Properties::AudioStreamRole>(),
                  configuration.object->template get_property<mpris::Player::Properties::PlaybackRate>(),
                  configuration.object->template get_property<mpris::Player::Properties::Shuffle>(),
                  configuration.object->template get_property<mpris::Player::Properties::TypedMetaData>(),
                  configuration.object->template get_property<mpris::Player::Properties::Volume>(),
                  configuration.object->template get_property<mpris::Player::Properties::Position>(),
                  configuration.object->template get_property<mpris::Player::Properties::Duration>(),
                  configuration.object->template get_property<mpris::Player::Properties::MinimumRate>(),
                  configuration.object->template get_property<mpris::Player::Properties::MaximumRate>()
              },
              signals
              {
                  configuration.object->template get_signal<mpris::Player::Signals::Seeked>(),
                  configuration.object->template get_signal<mpris::Player::Signals::EndOfStream>(),
                  configuration.object->template get_signal<mpris::Player::Signals::PlaybackStatusChanged>()
              }
        {            
        }

        // We just store creation time properties
        Configuration configuration;
        // All the properties exposed to the bus go here.
        struct
        {
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanPlay>> can_play;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanPause>> can_pause;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanSeek>> can_seek;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanControl>> can_control;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanGoNext>> can_go_next;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::CanGoPrevious>> can_go_previous;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::IsVideoSource>> is_video_source;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::IsAudioSource>> is_audio_source;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::PlaybackStatus>> playback_status;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::TypedPlaybackStatus>> typed_playback_status;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::LoopStatus>> loop_status;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::TypedLoopStatus>> typed_loop_status;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::AudioStreamRole>> audio_stream_role;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::PlaybackRate>> playback_rate;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Shuffle>> is_shuffle;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::TypedMetaData>> meta_data_for_current_track;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Volume>> volume;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Position>> position;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::Duration>> duration;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::MinimumRate>> minimum_playback_rate;
            std::shared_ptr<core::dbus::Property<mpris::Player::Properties::MaximumRate>> maximum_playback_rate;
        } properties;

        struct
        {
            typename core::dbus::Signal<mpris::Player::Signals::Seeked, mpris::Player::Signals::Seeked::ArgumentType>::Ptr seeked_to;
            typename core::dbus::Signal<mpris::Player::Signals::EndOfStream, mpris::Player::Signals::EndOfStream::ArgumentType>::Ptr end_of_stream;
            typename core::dbus::Signal<mpris::Player::Signals::PlaybackStatusChanged, mpris::Player::Signals::PlaybackStatusChanged::ArgumentType>::Ptr playback_status_changed;
        } signals;
    };
};
}

#endif // MPRIS_PLAYER_H_
