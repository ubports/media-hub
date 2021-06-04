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
#ifndef CORE_UBUNTU_MEDIA_TRACK_H_
#define CORE_UBUNTU_MEDIA_TRACK_H_

#include <QString>
#include <QScopedPointer>
#include <QUrl>
#include <QVariantMap>

namespace core
{
namespace ubuntu
{
namespace media
{

class Track
{
public:
    typedef QString Id;
    typedef QVariantMap MetaDataType;

    class MetaData: public MetaDataType
    {
    public:
        using MetaDataType::MetaDataType;
        static constexpr const char* TrackArtlUrlKey = "mpris:artUrl";
        static constexpr const char* TrackLengthKey = "mpris:length";
        static constexpr const char* TrackIdKey = "mpris:trackid";

        template<typename Tag>
        bool contains() const
        {
            return contains(Tag::name());
        }

        bool isSet(const QString &key) const
        {
            return !value(key).isNull();
        }

        void setAlbum(const QString &album);
        void setArtist(const QString &artist);
        void setTitle(const QString &title);
        void setTrackId(const QString &id);
        void setTrackLength(int64_t id);
        void setArtUrl(const QUrl &url);
        void setLastUsed(const QString &datetime);
        QString album() const;
        QString artist() const;
        QString title() const;
        QString trackId() const;
        int64_t trackLength() const;
        QUrl artUrl() const;
        QString lastUsed() const;
    };

    Track(const Id& id);
    Track(const Track&) = delete;
    virtual ~Track();

    Track& operator=(const Track&);
    bool operator==(const Track&) const;

    const Id& id() const;
    QUrl uri() const;

private:
    struct Private;
    QScopedPointer<Private> d;
};

}
}
}

#endif // CORE_UBUNTU_MEDIA_TRACK_H_
