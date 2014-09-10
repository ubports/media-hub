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

#ifndef MPRIS_MEDIA_PLAYER2_H_
#define MPRIS_MEDIA_PLAYER2_H_

#include <core/dbus/macros.h>
#include <core/dbus/object.h>
#include <core/dbus/property.h>

#include <string>
#include <vector>

namespace mpris
{
// Models interface org.mpris.MediaPlayer2, see:
//   http://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html
// for detailed documentation
struct MediaPlayer2
{
    static const std::string& name()
    {
        static const std::string s{"org.mpris.MediaPlayer2"};
        return s;
    }

    struct Methods
    {
        // Brings the media player's user interface to the front using any appropriate
        // mechanism available.
        // The media player may be unable to control how its user interface is displayed,
        // or it may not have a graphical user interface at all. In this case,
        // the CanRaise property is false and this method does nothing.
        DBUS_CPP_METHOD_DEF(Raise, MediaPlayer2)

        // Causes the media player to stop running.
        // The media player may refuse to allow clients to shut it down. In this case, the
        // CanQuit property is false and this method does nothing.
        DBUS_CPP_METHOD_DEF(Quit, MediaPlayer2)
    };

    struct Properties
    {
        // If false, calling Quit will have no effect, and may raise a NotSupported error.
        // If true, calling Quit will cause the media application to attempt to quit
        // (although it may still be prevented from quitting by the user, for example).
        DBUS_CPP_READABLE_PROPERTY_DEF(CanQuit, MediaPlayer2, bool)

        // Whether the media player is occupying the fullscreen.
        // This property is optional. Clients should handle its absence gracefully.
        DBUS_CPP_WRITABLE_PROPERTY_DEF(Fullscreen, MediaPlayer2, bool)

        // If false, attempting to set Fullscreen will have no effect, and may raise an error.
        // If true, attempting to set Fullscreen will not raise an error, and (if it is different
        // from the current value) will cause the media player to attempt to enter or exit fullscreen mode.
        // This property is optional. Clients should handle its absence gracefully.
        DBUS_CPP_READABLE_PROPERTY_DEF(CanSetFullscreen, MediaPlayer2, bool)

        // If false, calling Raise will have no effect, and may raise a NotSupported error. If true, calling Raise
        // will cause the media application to attempt to bring its user interface to the front,
        // although it may be prevented from doing so (by the window manager, for example).
        DBUS_CPP_READABLE_PROPERTY_DEF(CanRaise, MediaPlayer2, bool)

        // Indicates whether the /org/mpris/MediaPlayer2 object implements the
        // org.mpris.MediaPlayer2.TrackList interface.
        DBUS_CPP_READABLE_PROPERTY_DEF(HasTrackList, MediaPlayer2, bool)

        // A friendly name to identify the media player to users.
        DBUS_CPP_READABLE_PROPERTY_DEF(Identity, MediaPlayer2, std::string)

        // The basename of an installed .desktop file which complies with the Desktop entry specification,
        // with the ".desktop" extension stripped.
        // This property is optional. Clients should handle its absence gracefully.
        DBUS_CPP_READABLE_PROPERTY_DEF(DesktopEntry, MediaPlayer2, std::string)

        // The URI schemes supported by the media player.
        DBUS_CPP_READABLE_PROPERTY_DEF(SupportedUriSchemes, MediaPlayer2, std::vector<std::string>)

        // The mime-types supported by the media player.
        DBUS_CPP_READABLE_PROPERTY_DEF(SupportedMimeTypes, MediaPlayer2, std::vector<std::string>)
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
        };

        // Creates a new instance, sets up player properties and installs method handlers.
        Skeleton(const Configuration& configuration)
            : configuration(configuration),
              properties
              {
                  configuration.object->get_property<Properties::CanQuit>(),
                  configuration.object->get_property<Properties::Fullscreen>(),
                  configuration.object->get_property<Properties::CanSetFullscreen>(),
                  configuration.object->get_property<Properties::CanRaise>(),
                  configuration.object->get_property<Properties::HasTrackList>(),
                  configuration.object->get_property<Properties::Identity>(),
                  configuration.object->get_property<Properties::DesktopEntry>(),
                  configuration.object->get_property<Properties::SupportedUriSchemes>(),
                  configuration.object->get_property<Properties::SupportedMimeTypes>()
              }
        {            
        }

        // We just store creation time properties here.
        Configuration configuration;

        // All property instances go here.
        struct
        {
            std::shared_ptr<core::dbus::Property<Properties::CanQuit>> can_quit;
            std::shared_ptr<core::dbus::Property<Properties::Fullscreen>> fullscreen;
            std::shared_ptr<core::dbus::Property<Properties::CanSetFullscreen>> can_set_fullscreen;
            std::shared_ptr<core::dbus::Property<Properties::CanRaise>> can_raise;
            std::shared_ptr<core::dbus::Property<Properties::HasTrackList>> has_track_list;
            std::shared_ptr<core::dbus::Property<Properties::Identity>> identity;
            std::shared_ptr<core::dbus::Property<Properties::DesktopEntry>> desktop_entry;
            std::shared_ptr<core::dbus::Property<Properties::SupportedUriSchemes>> supported_uri_schemes;
            std::shared_ptr<core::dbus::Property<Properties::SupportedMimeTypes>> supported_mime_types;
        } properties;
    };
};
}

#endif // MPRIS_MEDIA_PLAYER2_H_
