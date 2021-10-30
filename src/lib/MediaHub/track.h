/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
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
 */

#ifndef LOMIRI_MEDIAHUB_TRACK_H
#define LOMIRI_MEDIAHUB_TRACK_H

#include "global.h"

#include <QSharedData>
#include <QSharedDataPointer>
#include <QUrl>
#include <QVariantMap>

namespace lomiri {
namespace MediaHub {

class Track;

class TrackData: public QSharedData
{
public:
    TrackData(const QUrl &uri): m_uri(uri) {}

private:
    friend class Track;
    QUrl m_uri;
};

class MH_EXPORT Track
{
public:
    struct MetaData: public QVariantMap
    {
    public:
        using QVariantMap::QVariantMap;
        MetaData(const QVariantMap &map = {}): QVariantMap(map) {}
        static constexpr const char* TrackArtlUrlKey = "mpris:artUrl";
        static constexpr const char* TrackLengthKey = "mpris:length";
        static constexpr const char* TrackIdKey = "mpris:trackid";
    };

    Track(const QUrl &uri = QUrl()): d(new TrackData(uri)) {}

    QUrl uri() const { return d->m_uri; }

private:
    QSharedDataPointer<TrackData> d;
};

} // namespace MediaHub
} // namespace lomiri

Q_DECLARE_METATYPE(lomiri::MediaHub::Track::MetaData)

#endif // LOMIRI_MEDIAHUB_TRACK_H
