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

#ifndef MPRIS_PLAYLISTS_H_
#define MPRIS_PLAYLISTS_H_

#include <core/dbus/macros.h>
#include <core/dbus/object.h>
#include <core/dbus/property.h>
#include <core/dbus/interfaces/properties.h>
#include <core/dbus/types/struct.h>
#include <core/dbus/types/variant.h>

#include <string>
#include <vector>

namespace core
{
namespace dbus
{
namespace types
{
template<typename T>
bool operator !=(const Struct<T>& lhs, const Struct<T>& rhs)
{
    return lhs.value != rhs.value;
}
}
}
}

namespace mpris
{
// Models interface org.mpris.MediaPlayer2.Playlists, see:
//   http://specifications.freedesktop.org/mpris-spec/latest/Playlists_Interface.html
// for detailed documentation
struct Playlists
{
    static const std::string& name()
    {
        static const std::string s{"org.mpris.MediaPlayer2.Playlists"}; return s;
    }

    // All known orderings
    struct Orderings
    {
        // Alphabetical ordering by name, ascending.
        static constexpr const char* alphabetical{"Alphabetical"};
        // Ordering by creation date, oldest first.
        static constexpr const char* creation_date{"CreationDate"};
        // Ordering by last modified date, oldest first.
        static constexpr const char* modified_date{"ModifiedDate"};
        // Ordering by date of last playback, oldest first.
        static constexpr const char* last_play_date{"LastPlayDate"};
        // A user-defined ordering.
        static constexpr const char* user_defined{"UserDefined"};
    };

    // A datastructure describing a playlist.
    typedef core::dbus::types::Struct
    <
        std::tuple
        <
            // A unique identifier for the playlist.
            // This should remain the same if the playlist is renamed.
            core::dbus::types::ObjectPath,
            // The name of the playlist, typically given by the user.
            std::string,
            // The URI of an (optional) icon.
            std::string
        >
    > Playlist;

    // A data structure describing a playlist, or nothing.
    typedef core::dbus::types::Struct
    <
        std::tuple
        <
            // Whether this structure refers to a valid playlist.
            bool,
            // The playlist, providing Valid is true, otherwise undefined.
            Playlist
        >
    > MaybePlaylist;

    struct Methods
    {
        Methods() = delete;

        // Starts playing the given playlist.
        // Note that this must be implemented. If the media player does not
        // allow clients to change the playlist, it should not implement this
        // interface at all.
        DBUS_CPP_METHOD_DEF(ActivatePlaylist, Playlists)

        // Gets a set of playlists.
        // Parameters
        // Index — u
        //   The index of the first playlist to be fetched (according to the ordering).
        // MaxCount — u
        //   The maximum number of playlists to fetch.
        // Order — s (Playlist_Ordering)
        //   The ordering that should be used.
        // ReverseOrder — b
        //   Whether the order should be reversed.
        //
        // Returns
        // Playlists — a(oss) (Playlist_List)
        //   A list of (at most MaxCount) playlists.
        DBUS_CPP_METHOD_DEF(GetPlaylists, Playlists)
    };

    struct Signals
    {
        Signals() = delete;

        // Indicates that either the Name or Icon attribute of a playlist has changed.
        // Client implementations should be aware that this signal may not be implemented.
        DBUS_CPP_SIGNAL_DEF(PlaylistsChanged, Playlists, Playlist)
    };

    struct Properties
    {
        Properties() = delete;

        // The number of playlists available.
        DBUS_CPP_READABLE_PROPERTY_DEF(PlaylistCount, Playlists, std::uint32_t)
        // The available orderings. At least one must be offered.
        DBUS_CPP_READABLE_PROPERTY_DEF(Orderings, Playlists, std::vector<std::string>)
        // The currently-active playlist.
        DBUS_CPP_READABLE_PROPERTY_DEF(ActivePlaylist, Playlists, MaybePlaylist)
    };

    struct Skeleton
    {
        // Creation time properties go here.
        struct Configuration
        {
            // The bus connection that should be used
            core::dbus::Bus::Ptr bus;
            // The dbus object that should implement org.mpris.MediaPlayer2
            core::dbus::Object::Ptr object;
            // Default values assigned to properties on construction
            struct Defaults
            {
                Properties::PlaylistCount::ValueType playlist_count{0};
                Properties::Orderings::ValueType orderings{{Orderings::alphabetical}};
                Properties::ActivePlaylist::ValueType active_playlist
                {
                    std::make_tuple(false, Playlist
                    {
                        std::make_tuple(
                            core::dbus::types::ObjectPath{"/"},
                            std::string{},
                            std::string{})
                    })
                };
            } defaults;
        };

        Skeleton(const Configuration& configuration)
            : configuration(configuration),
              properties
              {
                  configuration.object->get_property<Properties::PlaylistCount>(),
                  configuration.object->get_property<Properties::Orderings>()
              },
              signals
              {
                  configuration.object->get_signal<Signals::PlaylistsChanged>()
              }
        {
            properties.playlist_count->set(configuration.defaults.playlist_count);
            properties.orderings->set(configuration.defaults.orderings);
        }

        std::map<std::string, core::dbus::types::Variant> get_all_properties()
        {
            std::map<std::string, core::dbus::types::Variant> dict;
            dict[Properties::PlaylistCount::name()] = core::dbus::types::Variant::encode(properties.playlist_count->get());
            dict[Properties::Orderings::name()] = core::dbus::types::Variant::encode(properties.orderings->get());

            return dict;
        }

        Configuration configuration;

        struct
        {
            std::shared_ptr<core::dbus::Property<Properties::PlaylistCount>> playlist_count;
            std::shared_ptr<core::dbus::Property<Properties::Orderings>> orderings;
        } properties;

        struct
        {
            core::dbus::Signal<Signals::PlaylistsChanged, Signals::PlaylistsChanged::ArgumentType>::Ptr playlist_changed;
        } signals;
    };
};
}
#endif // MPRIS_PLAYLISTS_H_
