/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#include "track.h"
#include "xesam.h"

namespace media = core::ubuntu::media;

void media::Track::MetaData::setAlbum(const QString &album)
{
    insert(xesam::Album::name, album);
}

void media::Track::MetaData::setArtist(const QString &artist)
{
    insert(xesam::Artist::name, artist);
}

void media::Track::MetaData::setTitle(const QString &title)
{
    insert(xesam::Title::name, title);
}

void media::Track::MetaData::setTrackId(const QString &id)
{
    insert(media::Track::MetaData::TrackIdKey, id);
}

void media::Track::MetaData::setTrackLength(int64_t length)
{
    insert(media::Track::MetaData::TrackLengthKey,
           QVariant(qint64(length)));
}

void media::Track::MetaData::setArtUrl(const QUrl &url)
{
    insert(media::Track::MetaData::TrackArtlUrlKey, url.toString());
}

void media::Track::MetaData::setLastUsed(const QString &datetime)
{
    insert(xesam::LastUsed::name, datetime);
}

QString media::Track::MetaData::album() const
{
    return value(xesam::Album::name).toString();
}

QString media::Track::MetaData::artist() const
{
    return value(xesam::Artist::name).toString();
}

QString media::Track::MetaData::title() const
{
    return value(xesam::Title::name).toString();
}

QString media::Track::MetaData::trackId() const
{
    return value(media::Track::MetaData::TrackIdKey).toString();
}

int64_t media::Track::MetaData::trackLength() const
{
    return value(media::Track::MetaData::TrackLengthKey).value<int64_t>();
}

QUrl media::Track::MetaData::artUrl() const
{
    return value(media::Track::MetaData::TrackArtlUrlKey).toUrl();
}

QString media::Track::MetaData::lastUsed() const
{
    return value(xesam::LastUsed::name).toString();
}
