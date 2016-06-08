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
#include <core/dbus/interfaces/properties.h>
#include <core/dbus/types/any.h>
#include <core/dbus/types/object_path.h>
#include <core/dbus/types/variant.h>

#include <core/dbus/types/stl/tuple.h>

#include <boost/utility/identity_type.hpp>

#include <string>
#include <tuple>
#include <vector>

#include <cstdint>

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

        static const char* from(core::ubuntu::media::Player::LoopStatus status)
        {
            switch(status)
            {
            case core::ubuntu::media::Player::LoopStatus::none:
                return LoopStatus::none;
            case core::ubuntu::media::Player::LoopStatus::track:
                return LoopStatus::track;
            case core::ubuntu::media::Player::LoopStatus::playlist:
                return LoopStatus::playlist;
            }

            return nullptr;
        }

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

    struct Error
    {
        struct OutOfProcessBufferStreamingNotSupported
        {
            static constexpr const char* name
            {
                "mpris.Player.Error.OutOfProcessBufferStreamingNotSupported"
            };
        };

        struct InsufficientAppArmorPermissions
        {
            static constexpr const char* name
            {
                "mpris.Player.Error.InsufficientAppArmorPermissions"
            };
        };

        struct UriNotFound
        {
            static constexpr const char* name
            {
                "mpris.Player.Error.UriNotFound"
            };
        };
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
    DBUS_CPP_METHOD_DEF(OpenUriExtended, Player)

    struct Signals
    {
        DBUS_CPP_SIGNAL_DEF(Seeked, Player, std::int64_t)
        DBUS_CPP_SIGNAL_DEF(AboutToFinish, Player, void)
        DBUS_CPP_SIGNAL_DEF(EndOfStream, Player, void)
        DBUS_CPP_SIGNAL_DEF(PlaybackStatusChanged, Player, core::ubuntu::media::Player::PlaybackStatus)
        DBUS_CPP_SIGNAL_DEF(VideoDimensionChanged, Player, core::ubuntu::media::video::Dimensions)
        DBUS_CPP_SIGNAL_DEF(Error, Player, core::ubuntu::media::Player::Error)
        DBUS_CPP_SIGNAL_DEF(Buffering, Player, int)
    };

    struct Properties
    {
        DBUS_CPP_READABLE_PROPERTY_DEF(PlaybackStatus, Player, std::string)
        DBUS_CPP_READABLE_PROPERTY_DEF(TypedPlaybackStatus, Player, core::ubuntu::media::Player::PlaybackStatus)

        DBUS_CPP_WRITABLE_PROPERTY_DEF(LoopStatus, Player, std::string)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(TypedLoopStatus, Player, core::ubuntu::media::Player::LoopStatus)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(AudioStreamRole, Player, core::ubuntu::media::Player::AudioStreamRole)
        DBUS_CPP_READABLE_PROPERTY_DEF(Orientation, Player, core::ubuntu::media::Player::Orientation)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Lifetime, Player, core::ubuntu::media::Player::Lifetime)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(PlaybackRate, Player, double)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Rate, Player, double)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Shuffle, Player, bool)
        DBUS_CPP_READABLE_PROPERTY_DEF(Metadata, Player, Dictionary)
        DBUS_CPP_READABLE_PROPERTY_DEF(TypedMetaData, Player, core::ubuntu::media::Track::MetaData)
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Volume, Player, double)
        DBUS_CPP_READABLE_PROPERTY_DEF(Position, Player, std::int64_t)
        DBUS_CPP_READABLE_PROPERTY_DEF(Duration, Player, std::int64_t)
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
        static const std::vector<std::string>& the_empty_list_of_invalidated_properties()
        {
            static const std::vector<std::string> instance; return instance;
        }

        // Creation time options go here.
        struct Configuration
        {
            // The bus connection that should be used
            core::dbus::Bus::Ptr bus;
            // The dbus object that should implement org.mpris.MediaPlayer2
            core::dbus::Object::Ptr object;

            // Default values for properties
            struct Defaults
            {
                Properties::CanPlay::ValueType can_play{false};
                Properties::CanPause::ValueType can_pause{false};
                Properties::CanSeek::ValueType can_seek{true};
                Properties::CanControl::ValueType can_control{true};
                Properties::CanGoNext::ValueType can_go_next{false};
                Properties::CanGoPrevious::ValueType can_go_previous{false};
                Properties::IsVideoSource::ValueType is_video_source{false};
                Properties::IsAudioSource::ValueType is_audio_source{true};
                Properties::PlaybackStatus::ValueType playback_status{PlaybackStatus::stopped};
                Properties::TypedPlaybackStatus::ValueType typed_playback_status{core::ubuntu::media::Player::PlaybackStatus::null};
                Properties::LoopStatus::ValueType loop_status{LoopStatus::none};
                Properties::TypedLoopStatus::ValueType typed_loop_status{core::ubuntu::media::Player::LoopStatus::none};
                Properties::PlaybackRate::ValueType playback_rate{1.f};
                Properties::Shuffle::ValueType shuffle{false};
                Properties::TypedMetaData::ValueType typed_meta_data{};
                Properties::Volume::ValueType volume{0.f};
                Properties::Position::ValueType position{0};
                Properties::Duration::ValueType duration{0};
                Properties::MinimumRate::ValueType minimum_rate{1.f};
                Properties::MaximumRate::ValueType maximum_rate{1.f};
                Properties::Orientation::ValueType orientation{core::ubuntu::media::Player::Orientation::rotate0};
            } defaults;
        };

        Skeleton(const Configuration& configuration)
            : configuration(configuration),
              properties
              {
                  configuration.object->template get_property<Properties::CanPlay>(),
                  configuration.object->template get_property<Properties::CanPause>(),
                  configuration.object->template get_property<Properties::CanSeek>(),
                  configuration.object->template get_property<Properties::CanControl>(),
                  configuration.object->template get_property<Properties::CanGoNext>(),
                  configuration.object->template get_property<Properties::CanGoPrevious>(),
                  configuration.object->template get_property<Properties::IsVideoSource>(),
                  configuration.object->template get_property<Properties::IsAudioSource>(),
                  configuration.object->template get_property<Properties::PlaybackStatus>(),
                  configuration.object->template get_property<Properties::TypedPlaybackStatus>(),
                  configuration.object->template get_property<Properties::LoopStatus>(),
                  configuration.object->template get_property<Properties::TypedLoopStatus>(),
                  configuration.object->template get_property<Properties::AudioStreamRole>(),
                  configuration.object->template get_property<Properties::Orientation>(),
                  configuration.object->template get_property<Properties::Lifetime>(),
                  configuration.object->template get_property<Properties::PlaybackRate>(),
                  configuration.object->template get_property<Properties::Shuffle>(),
                  configuration.object->template get_property<Properties::TypedMetaData>(),
                  configuration.object->template get_property<Properties::Volume>(),
                  configuration.object->template get_property<Properties::Position>(),
                  configuration.object->template get_property<Properties::Duration>(),
                  configuration.object->template get_property<Properties::MinimumRate>(),
                  configuration.object->template get_property<Properties::MaximumRate>()
              },
              signals
              {
                  configuration.object->template get_signal<Signals::Seeked>(),
                  configuration.object->template get_signal<Signals::AboutToFinish>(),
                  configuration.object->template get_signal<Signals::EndOfStream>(),
                  configuration.object->template get_signal<Signals::PlaybackStatusChanged>(),
                  configuration.object->template get_signal<Signals::VideoDimensionChanged>(),
                  configuration.object->template get_signal<Signals::Error>(),
                  configuration.object->template get_signal<Signals::Buffering>(),
                  configuration.object->template get_signal<core::dbus::interfaces::Properties::Signals::PropertiesChanged>()
              }
        {
            properties.can_play->set(configuration.defaults.can_play);
            properties.can_pause->set(configuration.defaults.can_pause);
            properties.can_seek->set(configuration.defaults.can_seek);
            properties.can_control->set(configuration.defaults.can_control);
            properties.can_go_next->set(configuration.defaults.can_go_next);
            properties.can_go_previous->set(configuration.defaults.can_go_previous);
            properties.is_video_source->set(configuration.defaults.is_video_source);
            properties.is_audio_source->set(configuration.defaults.is_audio_source);
            properties.playback_status->set(configuration.defaults.playback_status);
            properties.typed_playback_status->set(configuration.defaults.typed_playback_status);
            properties.loop_status->set(configuration.defaults.loop_status);
            properties.typed_loop_status->set(configuration.defaults.typed_loop_status);
            properties.audio_stream_role->set(core::ubuntu::media::Player::AudioStreamRole::multimedia);
            properties.orientation->set(core::ubuntu::media::Player::Orientation::rotate0);
            properties.lifetime->set(core::ubuntu::media::Player::Lifetime::normal);
            properties.playback_rate->set(configuration.defaults.playback_rate);
            properties.shuffle->set(configuration.defaults.shuffle);
            properties.position->set(configuration.defaults.position);
            properties.duration->set(configuration.defaults.duration);
            properties.minimum_playback_rate->set(configuration.defaults.minimum_rate);
            properties.maximum_playback_rate->set(configuration.defaults.maximum_rate);

            // Make sure the Orientation Property gets sent over DBus to the client
            properties.orientation->changed().connect([this](const core::ubuntu::media::Player::Orientation& o)
            {
                on_property_value_changed<Properties::Orientation>(o);
            });

            properties.position->changed().connect([this](std::int64_t position)
            {
                on_property_value_changed<Properties::Position>(position);
            });

            properties.duration->changed().connect([this](std::int64_t duration)
            {
                on_property_value_changed<Properties::Duration>(duration);
            });

            properties.playback_status->changed().connect([this](const std::string& status)
            {
                on_property_value_changed<Properties::PlaybackStatus>(status);
            });

            properties.loop_status->changed().connect([this](const std::string& status)
            {
                on_property_value_changed<Properties::LoopStatus>(status);
            });

            properties.shuffle->changed().connect([this](bool shuffle)
            {
                on_property_value_changed<Properties::Shuffle>(shuffle);
            });

            properties.can_play->changed().connect([this](bool can_play)
            {
                on_property_value_changed<Properties::CanPlay>(can_play);
            });

            properties.can_pause->changed().connect([this](bool can_pause)
            {
                on_property_value_changed<Properties::CanPause>(can_pause);
            });

            properties.can_go_next->changed().connect([this](bool can_go_next)
            {
                on_property_value_changed<Properties::CanGoNext>(can_go_next);
            });

            properties.can_go_previous->changed().connect([this](bool can_go_previous)
            {
                on_property_value_changed<Properties::CanGoPrevious>(can_go_previous);
            });
        }

        template<typename Property>
        void on_property_value_changed(const typename Property::ValueType& value)
        {
            Dictionary dict; dict[Property::name()] = dbus::types::Variant::encode(value);

            signals.properties_changed->emit(std::make_tuple(
                            dbus::traits::Service<Player>::interface_name(),
                            dict,
                            the_empty_list_of_invalidated_properties()));
        }

        Dictionary get_all_properties()
        {
            Dictionary dict;
            dict[Properties::CanPlay::name()] = dbus::types::Variant::encode(properties.can_play->get());
            dict[Properties::CanPause::name()] = dbus::types::Variant::encode(properties.can_pause->get());
            dict[Properties::CanSeek::name()] = dbus::types::Variant::encode(properties.can_seek->get());
            dict[Properties::CanControl::name()] = dbus::types::Variant::encode(properties.can_control->get());
            dict[Properties::CanGoNext::name()] = dbus::types::Variant::encode(properties.can_go_next->get());
            dict[Properties::CanGoPrevious::name()] = dbus::types::Variant::encode(properties.can_go_previous->get());
            dict[Properties::PlaybackStatus::name()] = dbus::types::Variant::encode(properties.playback_status->get());
            dict[Properties::TypedPlaybackStatus::name()] = dbus::types::Variant::encode(properties.typed_playback_status->get());
            dict[Properties::LoopStatus::name()] = dbus::types::Variant::encode(properties.loop_status->get());
            dict[Properties::TypedLoopStatus::name()] = dbus::types::Variant::encode(properties.typed_loop_status->get());
            dict[Properties::AudioStreamRole::name()] = dbus::types::Variant::encode(properties.audio_stream_role->get());
            dict[Properties::Orientation::name()] = dbus::types::Variant::encode(properties.orientation->get());
            dict[Properties::Lifetime::name()] = dbus::types::Variant::encode(properties.lifetime->get());
            dict[Properties::PlaybackRate::name()] = dbus::types::Variant::encode(properties.playback_rate->get());
            dict[Properties::Shuffle::name()] = dbus::types::Variant::encode(properties.shuffle->get());
            dict[Properties::Duration::name()] = dbus::types::Variant::encode(properties.duration->get());
            dict[Properties::Position::name()] = dbus::types::Variant::encode(properties.position->get());
            dict[Properties::MinimumRate::name()] = dbus::types::Variant::encode(properties.minimum_playback_rate->get());
            dict[Properties::MaximumRate::name()] = dbus::types::Variant::encode(properties.maximum_playback_rate->get());

            return dict;
        }

        // We just store creation time properties
        Configuration configuration;
        // All the properties exposed to the bus go here.
        struct
        {
            std::shared_ptr<core::dbus::Property<Properties::CanPlay>> can_play;
            std::shared_ptr<core::dbus::Property<Properties::CanPause>> can_pause;
            std::shared_ptr<core::dbus::Property<Properties::CanSeek>> can_seek;
            std::shared_ptr<core::dbus::Property<Properties::CanControl>> can_control;
            std::shared_ptr<core::dbus::Property<Properties::CanGoNext>> can_go_next;
            std::shared_ptr<core::dbus::Property<Properties::CanGoPrevious>> can_go_previous;
            std::shared_ptr<core::dbus::Property<Properties::IsVideoSource>> is_video_source;
            std::shared_ptr<core::dbus::Property<Properties::IsAudioSource>> is_audio_source;

            std::shared_ptr<core::dbus::Property<Properties::PlaybackStatus>> playback_status;
            std::shared_ptr<core::dbus::Property<Properties::TypedPlaybackStatus>> typed_playback_status;
            std::shared_ptr<core::dbus::Property<Properties::LoopStatus>> loop_status;
            std::shared_ptr<core::dbus::Property<Properties::TypedLoopStatus>> typed_loop_status;
            std::shared_ptr<core::dbus::Property<Properties::AudioStreamRole>> audio_stream_role;
            std::shared_ptr<core::dbus::Property<Properties::Orientation>> orientation;
            std::shared_ptr<core::dbus::Property<Properties::Lifetime>> lifetime;
            std::shared_ptr<core::dbus::Property<Properties::PlaybackRate>> playback_rate;
            std::shared_ptr<core::dbus::Property<Properties::Shuffle>> shuffle;
            std::shared_ptr<core::dbus::Property<Properties::TypedMetaData>> typed_meta_data_for_current_track;
            std::shared_ptr<core::dbus::Property<Properties::Volume>> volume;
            std::shared_ptr<core::dbus::Property<Properties::Position>> position;
            std::shared_ptr<core::dbus::Property<Properties::Duration>> duration;
            std::shared_ptr<core::dbus::Property<Properties::MinimumRate>> minimum_playback_rate;
            std::shared_ptr<core::dbus::Property<Properties::MaximumRate>> maximum_playback_rate;
        } properties;

        struct
        {
            typename core::dbus::Signal<Signals::Seeked, Signals::Seeked::ArgumentType>::Ptr seeked_to;
            typename core::dbus::Signal<Signals::AboutToFinish, Signals::AboutToFinish::ArgumentType>::Ptr about_to_finish;
            typename core::dbus::Signal<Signals::EndOfStream, Signals::EndOfStream::ArgumentType>::Ptr end_of_stream;
            typename core::dbus::Signal<Signals::PlaybackStatusChanged, Signals::PlaybackStatusChanged::ArgumentType>::Ptr playback_status_changed;
            typename core::dbus::Signal<Signals::VideoDimensionChanged, Signals::VideoDimensionChanged::ArgumentType>::Ptr video_dimension_changed;
            typename core::dbus::Signal<Signals::Error, Signals::Error::ArgumentType>::Ptr error;
            typename core::dbus::Signal<Signals::Buffering, Signals::Buffering::ArgumentType>::Ptr buffering_changed;

            dbus::Signal
            <
                core::dbus::interfaces::Properties::Signals::PropertiesChanged,
                core::dbus::interfaces::Properties::Signals::PropertiesChanged::ArgumentType
            >::Ptr properties_changed;
        } signals;
    };
};
}

#endif // MPRIS_PLAYER_H_
