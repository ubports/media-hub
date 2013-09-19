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

#include <com/ubuntu/music/track.h>
#include <com/ubuntu/music/property.h>

#include <map>

namespace music = com::ubuntu::music;

struct music::Track::Private
{
    Private(const MetaData& meta_data,
            const Track::UriType& uri) : meta_data(meta_data),
                                         uri(uri)
    {
    }

    Private(const Private& rhs) : meta_data(rhs.meta_data),
                                  uri(rhs.uri)
    {
    }

    bool operator==(const Private& rhs) const
    {
        return meta_data == rhs.meta_data &&
               uri == rhs.uri;
    }

    Property<MetaData> meta_data;
    Track::UriType uri;
};

music::Track::Track(const music::Track& rhs) : d(new Private(*rhs.d))
{
}

music::Track::~Track()
{
}

music::Track& music::Track::operator=(const music::Track& rhs)
{
    *d = *rhs.d;
    return *this;
}

bool music::Track::operator==(const music::Track& rhs) const
{
    return *d == *rhs.d;
}

const music::Track::UriType& music::Track::uri() const
{
    return d->uri;
}

const music::Property<music::Track::MetaData>& music::Track::meta_data() const
{
    return d->meta_data;
}
    
music::Track::Track(const music::Track::UriType& uri, const music::Track::MetaData& meta_data) : d(new Private(meta_data, uri))
{
}
