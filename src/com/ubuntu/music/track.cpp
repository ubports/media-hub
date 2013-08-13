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

#include "com/ubuntu/music/track.h"

#include <map>

namespace music = com::ubuntu::music;

const music::Track::MetaData::KeyType& music::Track::MetaData::track_id_key()
{
    static const KeyType key = "test";
    return key;
}

struct music::Track::MetaData::Private
{
    std::map<music::Track::MetaData::KeyType, music::Track::MetaData::ValueType> lut; 

    bool operator==(const Private& rhs) const
    {
        return lut == rhs.lut;
    }
};

music::Track::MetaData::MetaData(const music::Track::MetaData& rhs) : d(new Private(*rhs.d))
{
}

music::Track::MetaData::~MetaData()
{
}

music::Track::MetaData& music::Track::MetaData::operator=(const music::Track::MetaData& rhs)
{
    *d = *rhs.d;
    return *this;
}

bool music::Track::MetaData::operator==(const music::Track::MetaData& rhs) const
{
    return *d == *rhs.d;
}
        
bool music::Track::MetaData::has_value_for_key(const music::Track::MetaData::KeyType& key) const
{
    return d->lut.count(key) > 0;
}

const music::Track::MetaData::ValueType& music::Track::MetaData::value_for_key(
    const music::Track::MetaData::KeyType& key) const
{
    return d->lut.at(key);
}

void music::Track::MetaData::for_each(
    const std::function<void(const music::Track::MetaData::KeyType&, const music::Track::MetaData::ValueType&)>& f)
{
    for (auto p : d->lut)
    {
        f(p.first, p.second);
    }
}

music::Track::MetaData::MetaData() : d(new Private())
{
}
        
struct music::Track::Private
{
    Private(const MetaData& meta_data,
            const Track::UriType& uri) : meta_data(meta_data),
                                         uri(uri)
    {
    }

    bool operator==(const Private& rhs) const
    {
        return
                meta_data == rhs.meta_data &&
                uri == rhs.uri;
    }

    MetaData meta_data;
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

const music::Track::MetaData& music::Track::meta_data() const
{
    return d->meta_data;
}

music::Connection music::Track::on_meta_data_changed(const std::function<void(const music::Track::MetaData&)>& f)
{
    (void) f;
    return Connection(nullptr);
}
    
music::Track::Track(const music::Track::UriType& uri, const music::Track::MetaData& meta_data) : d(new Private(meta_data, uri))
{
}
