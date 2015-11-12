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

#ifndef MPRIS_TRACK_LIST_H_
#define MPRIS_TRACK_LIST_H_

#include <core/dbus/macros.h>

#include <core/dbus/types/any.h>
#include <core/dbus/macros.h>
#include <core/dbus/types/object_path.h>
#include <core/dbus/object.h>
#include <core/dbus/property.h>
#include <core/dbus/types/variant.h>

#include <boost/utility/identity_type.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace dbus = core::dbus;

namespace mpris
{
struct TrackList
{
    typedef std::map<std::string, core::dbus::types::Variant> Dictionary;

    static const std::string& name()
    {
        static const std::string s{"org.mpris.MediaPlayer2.TrackList"};
        return s;
    }

    struct Error
    {
        struct InsufficientPermissionsToAddTrack
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.InsufficientPermissionsToAddTrack"
            };
        };

        struct FailedToMoveTrack
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.FailedToMoveTrack"
            };
        };

        struct FailedToFindMoveTrackSource
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.FailedToFindMoveTrackSource"
            };
        };

        struct FailedToFindMoveTrackDest
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.FailedToFindMoveTrackDest"
	    };
	};

        struct TrackNotFound
        {
            static constexpr const char* name
            {
                "mpris.TrackList.Error.TrackNotFound"
            };
        };
    };

    DBUS_CPP_METHOD_DEF(GetTracksMetadata, TrackList)
    DBUS_CPP_METHOD_DEF(GetTracksUri, TrackList)
    DBUS_CPP_METHOD_DEF(AddTrack, TrackList)
    DBUS_CPP_METHOD_DEF(AddTracks, TrackList)
    DBUS_CPP_METHOD_DEF(MoveTrack, TrackList)
    DBUS_CPP_METHOD_DEF(RemoveTrack, TrackList)
    DBUS_CPP_METHOD_DEF(GoTo, TrackList)
    DBUS_CPP_METHOD_DEF(Reset, TrackList)

    struct Signals
    {
        Signals() = delete;

        DBUS_CPP_SIGNAL_DEF
        (
            TrackListReplaced,
            TrackList,
            BOOST_IDENTITY_TYPE((std::tuple<std::vector<core::ubuntu::media::Track::Id>, core::ubuntu::media::Track::Id>))
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackAdded,
            TrackList,
            core::ubuntu::media::Track::Id
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TracksAdded,
            TrackList,
            core::ubuntu::media::TrackList::ContainerURI
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackMoved,
            TrackList,
            BOOST_IDENTITY_TYPE((std::tuple<core::ubuntu::media::Track::Id, core::ubuntu::media::Track::Id>))
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackRemoved,
            TrackList,
            core::ubuntu::media::Track::Id
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackChanged,
            TrackList,
            core::ubuntu::media::Track::Id
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackListReset,
            TrackList,
            void
        )

        DBUS_CPP_SIGNAL_DEF
        (
            TrackMetadataChanged,
            TrackList,
            BOOST_IDENTITY_TYPE((std::tuple<std::map<std::string, dbus::types::Variant>, dbus::types::ObjectPath>))
        )
    };

    struct Properties
    {
        Properties() = delete;

        DBUS_CPP_READABLE_PROPERTY_DEF(Tracks, TrackList, std::vector<core::ubuntu::media::Track::Id>)
        DBUS_CPP_READABLE_PROPERTY_DEF(CanEditTracks, TrackList, bool)
    };

    struct Skeleton
    {
        static const std::vector<std::string>& the_empty_list_of_invalidated_properties()
        {
            static const std::vector<std::string> instance; return instance;
        }

        // Object instance creation time properties go here.
        struct Configuration
        {
            // The dbus object that should implement org.mpris.MediaPlayer2
            core::dbus::Object::Ptr object;
            // Default values assigned to exported dbus interface properties on construction
            struct Defaults
            {
                Properties::Tracks::ValueType tracks{std::vector<core::ubuntu::media::Track::Id>()};
                Properties::CanEditTracks::ValueType can_edit_tracks{true};
            } defaults;
        };

        Skeleton(const Configuration& configuration)
            : configuration(configuration),
              properties
              {
                  configuration.object->template get_property<Properties::Tracks>(),
                  configuration.object->template get_property<Properties::CanEditTracks>(),
              },
              signals
              {
                  configuration.object->template get_signal<Signals::TrackListReplaced>(),
                  configuration.object->template get_signal<Signals::TrackAdded>(),
                  configuration.object->template get_signal<Signals::TracksAdded>(),
                  configuration.object->template get_signal<Signals::TrackMoved>(),
                  configuration.object->template get_signal<Signals::TrackRemoved>(),
                  configuration.object->template get_signal<Signals::TrackChanged>(),
                  configuration.object->template get_signal<Signals::TrackListReset>(),
                  configuration.object->template get_signal<Signals::TrackMetadataChanged>(),
                  configuration.object->template get_signal<core::dbus::interfaces::Properties::Signals::PropertiesChanged>()
              }
        {
            // Set the default value of the properties on the MPRIS TrackList dbus interface
            properties.tracks->set(configuration.defaults.tracks);
            properties.can_edit_tracks->set(configuration.defaults.can_edit_tracks);
        }

        template<typename Property>
        void on_property_value_changed(const typename Property::ValueType& value)
        {
            Dictionary dict;
            dict[Property::name()] = dbus::types::Variant::encode(value);

            signals.properties_changed->emit(std::make_tuple(
                            dbus::traits::Service<TrackList>::interface_name(),
                            dict,
                            the_empty_list_of_invalidated_properties()));
        }

        std::map<std::string, core::dbus::types::Variant> get_all_properties()
        {
            std::map<std::string, core::dbus::types::Variant> dict;
            dict[Properties::Tracks::name()] = core::dbus::types::Variant::encode(properties.tracks->get());
            dict[Properties::CanEditTracks::name()] = core::dbus::types::Variant::encode(properties.can_edit_tracks->get());

            return dict;
        }

        Configuration configuration;

        struct
        {
            std::shared_ptr<core::dbus::Property<Properties::Tracks>> tracks;
            std::shared_ptr<core::dbus::Property<Properties::CanEditTracks>> can_edit_tracks;
        } properties;

        struct
        {
            core::dbus::Signal<Signals::TrackListReplaced, Signals::TrackListReplaced::ArgumentType>::Ptr tracklist_replaced;
            core::dbus::Signal<Signals::TrackAdded, Signals::TrackAdded::ArgumentType>::Ptr track_added;
            core::dbus::Signal<Signals::TracksAdded, Signals::TracksAdded::ArgumentType>::Ptr tracks_added;
            core::dbus::Signal<Signals::TrackMoved, Signals::TrackMoved::ArgumentType>::Ptr track_moved;
            core::dbus::Signal<Signals::TrackRemoved, Signals::TrackRemoved::ArgumentType>::Ptr track_removed;
            core::dbus::Signal<Signals::TrackChanged, Signals::TrackChanged::ArgumentType>::Ptr track_changed;
            core::dbus::Signal<Signals::TrackListReset, Signals::TrackListReset::ArgumentType>::Ptr track_list_reset;
            core::dbus::Signal<Signals::TrackMetadataChanged, Signals::TrackMetadataChanged::ArgumentType>::Ptr track_metadata_changed;

            dbus::Signal <core::dbus::interfaces::Properties::Signals::PropertiesChanged,
                core::dbus::interfaces::Properties::Signals::PropertiesChanged::ArgumentType
            >::Ptr properties_changed;
        } signals;
    };
};
}

#endif // MPRIS_TRACK_LIST_H_
