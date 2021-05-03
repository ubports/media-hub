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

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusContext>
#include <QString>
#include <QStringList>

#include <string>
#include <vector>

namespace mpris
{
// Models interface org.mpris.MediaPlayer2, see:
//   http://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html
// for detailed documentation
class RootAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ falseProperty)
    Q_PROPERTY(bool Fullscreen READ falseProperty)
    Q_PROPERTY(bool CanSetFullscreen READ falseProperty)
    Q_PROPERTY(bool CanRaise READ falseProperty)
    Q_PROPERTY(bool HasTrackList READ falseProperty)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)

public:
    RootAdaptor(QObject *parent = nullptr): QDBusAbstractAdaptor(parent) {}
    virtual ~RootAdaptor() = default;

    bool falseProperty() const { return false; }
    QString identity() const { return QStringLiteral("core::media::Hub"); }
    QString desktopEntry() const { return QStringLiteral("mediaplayer-app"); }
    QStringList supportedUriSchemes() const { return {}; }
    QStringList supportedMimeTypes() const {
        return {"audio/mpeg3", "video/mpeg4"};
    }

public Q_SLOTS:
    void Raise() {}
    void Quit() {}
};

// Models interface org.mpris.MediaPlayer2.Playlists, see:
//   http://specifications.freedesktop.org/mpris-spec/latest/Playlists_Interface.html
// for detailed documentation
class PlaylistsAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Playlists")
    Q_PROPERTY(uint32_t PlaylistCount READ playlistCount)
    Q_PROPERTY(QStringList Orderings READ orderings)
    // FIXME: There's also an ActivePlaylist property in the specs

public:
    PlaylistsAdaptor(QObject *parent = nullptr): QDBusAbstractAdaptor(parent) {}
    virtual ~PlaylistsAdaptor() = default;

    uint32_t playlistCount() const { return 0; }
    QStringList orderings() const { return {"Alphabetical"}; }

public Q_SLOTS:
    void ActivePlaylist() {}
    void GetPlaylists() {}
};

}

#endif // MPRIS_MEDIA_PLAYER2_H_
