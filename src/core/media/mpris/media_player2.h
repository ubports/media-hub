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

#include <QDBusContext>
#include <QObject>

namespace core {
namespace ubuntu {
namespace media {

class PlayerImplementation;

}}}

namespace mpris
{

/* Adaptor class exposing the media-hub service through the D-Bus MPRIS
 * interface.
 */
class MediaPlayer2Private;
class MediaPlayer2: public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    MediaPlayer2(QObject *parent = nullptr);
    virtual ~MediaPlayer2();

    void setPlayer(core::ubuntu::media::PlayerImplementation *impl);
    core::ubuntu::media::PlayerImplementation *player();
    const core::ubuntu::media::PlayerImplementation *player() const;

    bool registerObject();

private:
    Q_DECLARE_PRIVATE(MediaPlayer2)
    QScopedPointer<MediaPlayer2Private> d_ptr;
};

} // namespace

#endif // MPRIS_MEDIA_PLAYER2_H_
