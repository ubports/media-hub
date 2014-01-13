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

#ifndef CORE_UBUNTU_MEDIA_PROPERTY_STUB_H_
#define CORE_UBUNTU_MEDIA_PROPERTY_STUB_H_

#include <core/media/player.h>
#include <core/media/property.h>
#include <core/media/track_list.h>

#include <org/freedesktop/dbus/service.h>
#include <org/freedesktop/dbus/types/stl/vector.h>

#include "mpris/player.h"
#include "mpris/track_list.h"

namespace core
{
namespace ubuntu
{
namespace media
{

template<typename T, typename DBusProperty>
struct PropertyStub : public Property<T>
{
    typedef media::Property<T> super;

    PropertyStub(const std::shared_ptr<org::freedesktop::dbus::Property<DBusProperty>>& dbus_property)
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

    std::shared_ptr<org::freedesktop::dbus::Property<DBusProperty>> dbus_property;
};

template<>
struct PropertyStub<media::Player::PlaybackStatus, mpris::Player::Properties::PlaybackStatus> : public media::Property<media::Player::PlaybackStatus>
{
    typedef media::Property<Player::PlaybackStatus> super;

    static const std::map<Player::PlaybackStatus, std::string>& playback_status_lut()
    {
        static const std::map<Player::PlaybackStatus, std::string> lut =
        {
            {Player::PlaybackStatus::null, "null"},
            {Player::PlaybackStatus::ready, "ready"},
            {Player::PlaybackStatus::playing, "playing"},
            {Player::PlaybackStatus::paused, "paused"},
            {Player::PlaybackStatus::stopped, "stopped"}
        };

        return lut;
    }

    static const std::map<std::string, Player::PlaybackStatus>& reverse_playback_status_lut()
    {
        static const std::map<std::string, Player::PlaybackStatus> lut =
        {
            {"null", Player::PlaybackStatus::null},
            {"ready", Player::PlaybackStatus::ready},
            {"playing", Player::PlaybackStatus::playing},
            {"paused", Player::PlaybackStatus::paused},
            {"stopped", Player::PlaybackStatus::stopped}
        };

        return lut;
    }

    PropertyStub(const std::shared_ptr<org::freedesktop::dbus::Property<mpris::Player::Properties::PlaybackStatus>>& dbus_property)
            : super(),
              dbus_property(dbus_property)
    {
    }

    const Player::PlaybackStatus& get() const
    {
        auto value = dbus_property->value();
        super::mutable_get() = reverse_playback_status_lut().at(value);
        return super::get();
    }

    void set(const Player::PlaybackStatus& value)
    {
        dbus_property->value(playback_status_lut().at(value));
        super::set(value);
    }

    std::shared_ptr<org::freedesktop::dbus::Property<mpris::Player::Properties::PlaybackStatus>> dbus_property;
};

template<>
struct PropertyStub<Player::LoopStatus, mpris::Player::Properties::LoopStatus>
        : public Property<Player::LoopStatus>
{
    typedef Property<Player::LoopStatus> super;

    static const std::map<Player::LoopStatus, std::string>& loop_status_lut()
    {
        static const std::map<Player::LoopStatus, std::string> lut =
        {
            {Player::LoopStatus::none, "none"},
            {Player::LoopStatus::track, "track"},
            {Player::LoopStatus::playlist, "playlist"}
        };

        return lut;
    }

    static const std::map<std::string, Player::LoopStatus>& reverse_loop_status_lut()
    {
        static const std::map<std::string, Player::LoopStatus> lut =
        {
            {"none", Player::LoopStatus::none},
            {"track", Player::LoopStatus::track},
            {"playlist", Player::LoopStatus::playlist}
        };

        return lut;
    }

    PropertyStub(
            const std::shared_ptr<org::freedesktop::dbus::Property<mpris::Player::Properties::LoopStatus>>& dbus_property)
            : super(),
              dbus_property(dbus_property)
    {
    }

    const Player::LoopStatus& get() const
    {
        auto value = dbus_property->value();
        super::mutable_get() = reverse_loop_status_lut().at(value);
        return super::get();
    }

    void set(const Player::LoopStatus& value)
    {
        dbus_property->value(loop_status_lut().at(value));
        super::set(value);
    }

    std::shared_ptr<org::freedesktop::dbus::Property<mpris::Player::Properties::LoopStatus>> dbus_property;
};

template<>
struct PropertyStub<TrackList::Container, mpris::TrackList::Properties::Tracks>
        : public Property<TrackList::Container>
{
    typedef Property<TrackList::Container> super;

    PropertyStub(
            const std::shared_ptr<org::freedesktop::dbus::Property<mpris::TrackList::Properties::Tracks>>& dbus_property)
            : super(),
              dbus_property(dbus_property)
    {
    }

    const TrackList::Container& get() const
    {
        auto remote_value = dbus_property->value();

        super::mutable_get().clear();
        for (auto path : remote_value)
        {
            super::mutable_get().push_back(
                        media::Track::Id{path});
        }
        return super::get();
    }

    void set(const TrackList::Container& value)
    {
        mpris::TrackList::Properties::Tracks::ValueType dbus_tracks;

        for (auto track : value)
        {
            dbus_tracks.push_back(track);
        }

        dbus_property->value(dbus_tracks);

        super::set(value);
    }

    std::shared_ptr<org::freedesktop::dbus::Property<mpris::TrackList::Properties::Tracks>> dbus_property;
};

template<>
struct PropertyStub<Track::MetaData, mpris::Player::Properties::MetaData>
        : public Property<Track::MetaData>
{
    typedef Property<Track::MetaData> super;

    PropertyStub(
            const std::shared_ptr<org::freedesktop::dbus::Property<mpris::Player::Properties::MetaData>>& dbus_property)
            : super(),
              dbus_property(dbus_property)
    {
    }

    const Track::MetaData& get() const
    {
        return super::get();
    }

    void set(const Track::MetaData& value)
    {
        super::set(value);
    }

    std::shared_ptr<org::freedesktop::dbus::Property<mpris::Player::Properties::MetaData>> dbus_property;
};
}
}
}
#endif // CORE_UBUNTU_MEDIA_PROPERTY_STUB_H_
