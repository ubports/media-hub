/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY {} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "track_list_skeleton.h"

#include "apparmor/ubuntu.h"
#include "logging.h"
#include "track_list_implementation.h"

#include "mpris.h"

#include "util/uri_check.h"

#include <QDBusConnection>
#include <QDBusMessage>

#include <limits>
#include <cstdint>

namespace media = core::ubuntu::media;

using namespace media;

namespace core {
namespace ubuntu {
namespace media {

class TrackListSkeletonPrivate
{
    Q_DECLARE_PUBLIC(TrackListSkeleton)

public:
    TrackListSkeletonPrivate(const QDBusConnection &bus,
            const apparmor::ubuntu::RequestContextResolver::Ptr& request_context_resolver,
            const media::apparmor::ubuntu::RequestAuthenticator::Ptr& request_authenticator,
            TrackListImplementation *impl,
            TrackListSkeleton *q):
        m_connection(bus),
        request_context_resolver(request_context_resolver),
        request_authenticator(request_authenticator),
        m_impl(impl),
        q_ptr(q)
    {
    }

private:
    QDBusConnection m_connection;
    media::apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
    media::apparmor::ubuntu::RequestAuthenticator::Ptr request_authenticator;
    TrackListImplementation *m_impl;
    TrackListSkeleton *q_ptr;
};

}}} // namespace

media::TrackListSkeleton::TrackListSkeleton(const QDBusConnection &bus,
        const media::apparmor::ubuntu::RequestContextResolver::Ptr& request_context_resolver,
        const media::apparmor::ubuntu::RequestAuthenticator::Ptr& request_authenticator,
        TrackListImplementation *impl,
        QObject *parent):
    QObject(parent),
    d_ptr(new TrackListSkeletonPrivate(bus,
                                       request_context_resolver,
                                       request_authenticator,
                                       impl, this))
{
    QObject::connect(impl, &TrackListImplementation::trackListReplaced,
                     this, [this](const QVector<media::Track::Id> &tracks,
                                  const Track::Id &currentTrack) {
        QStringList trackList;
        for (const auto id: tracks) {
            trackList.append(id);
        }
        Q_EMIT TrackListReplaced(trackList, currentTrack);
    });
    QObject::connect(impl, &TrackListImplementation::trackAdded,
                     this, &TrackListSkeleton::TrackAdded);
    QObject::connect(impl, &TrackListImplementation::tracksAdded,
                     this, [this](const QVector<QUrl> &tracks) {
        QStringList trackList;
        for (const auto uri: tracks) {
            trackList.append(uri.toString());
        }
        Q_EMIT TracksAdded(trackList);
    });
    QObject::connect(impl, &TrackListImplementation::trackRemoved,
                     this, &TrackListSkeleton::TrackRemoved);
    QObject::connect(impl, &TrackListImplementation::trackMoved,
                     this, &TrackListSkeleton::TrackMoved);
    QObject::connect(impl, &TrackListImplementation::trackChanged,
                     this, &TrackListSkeleton::TrackChanged);
    QObject::connect(impl, &TrackListImplementation::trackListReset,
                     this, &TrackListSkeleton::TrackListReset);
    // FIXME TrackMetadataChanged is never invoked
}

media::TrackListSkeleton::~TrackListSkeleton()
{
}

QStringList TrackListSkeleton::tracks() const
{
    Q_D(const TrackListSkeleton);
    QStringList trackList;
    const auto tracks = d->m_impl->tracks();
    for (const auto id: tracks) {
        trackList.append(id);
    }
    return trackList;
}

bool TrackListSkeleton::canEditTracks() const
{
    Q_D(const TrackListSkeleton);
    return d->m_impl->canEditTracks();
}

QMap<QString,QString> TrackListSkeleton::GetTracksMetadata(const QString &track)
{
    /* FIXME: We should return the metadata, but since the old
     * implementation always returned an empty map, let's continue
     * doing the same. We need to fix the signature anyway.
    Q_D(TrackListSkeleton);
    Track::MetaData md = d->m_impl->query_meta_data_for_track(track);
    */
    Q_UNUSED(track);
    return QMap<QString,QString>();
}

QString TrackListSkeleton::GetTracksUri(const QString &track)
{
    Q_D(TrackListSkeleton);
    return d->m_impl->query_uri_for_track(track).toString();
}

void TrackListSkeleton::AddTrack(const QString &uri, const QString &after,
                                 bool makeCurrent)
{
    Q_D(TrackListSkeleton);

    MH_TRACE("");
    QDBusMessage in = message();
    QDBusConnection bus = connection();
    in.setDelayedReply(true);

    struct Params {
        QString uri;
        QString after;
        bool makeCurrent;
    } params = { uri, after, makeCurrent };

    d->request_context_resolver->resolve_context_for_dbus_name_async
        (in.service(), [this, in, bus, params](const media::apparmor::ubuntu::Context& context)
    {
        Q_D(TrackListSkeleton);
        QUrl uri = QUrl::fromUserInput(params.uri);
        media::Track::Id after = params.after;
        bool makeCurrent = params.makeCurrent;

        // Make sure the client has adequate apparmor permissions to open the URI
        const auto result = d->request_authenticator->authenticate_open_uri_request(context, uri);

        UriCheck uri_check(uri);
        const bool valid_uri = !uri_check.is_local_file() or
                (uri_check.is_local_file() and uri_check.file_exists());
        QDBusMessage reply = in.createReply();
        if (!valid_uri)
        {
            const QString err_str = {"Warning: Not adding track " + uri.toString() +
                 " to TrackList because it can't be found."};
            MH_WARNING() << err_str;
            reply = in.createErrorReply(
                        mpris::Player::Error::UriNotFound::name,
                        err_str);
        }
        else
        {
            // Only add the track to the TrackList if it passes the apparmor permissions check
            if (std::get<0>(result))
            {
                d->m_impl->add_track_with_uri_at(uri, after, makeCurrent);
            }
            else
            {
                const QString err_str = {"Warning: Not adding track " + uri.toString() +
                    " to TrackList because of inadequate client apparmor permissions."};
                MH_WARNING() << err_str;
                reply = in.createErrorReply(
                            mpris::TrackList::Error::InsufficientPermissionsToAddTrack::name,
                            err_str);
            }
        }

        bus.send(reply);
    });
}

void TrackListSkeleton::AddTracks(const QStringList &uris,
                                  const QString &after)
{
    Q_D(TrackListSkeleton);
    MH_TRACE("");
    QDBusMessage in = message();
    QDBusConnection bus = connection();
    in.setDelayedReply(true);

    struct Params {
        QStringList uris;
        QString after;
    } params = { uris, after };

    d->request_context_resolver->resolve_context_for_dbus_name_async
        (in.service(), [this, in, bus, params](const media::apparmor::ubuntu::Context& context)
    {
        Q_D(TrackListSkeleton);
        const QStringList &uris = params.uris;
        const media::Track::Id after = params.after;

        bool valid_uri = false;
        media::apparmor::ubuntu::RequestAuthenticator::Result result;
        QString uri_err_str, err_str;
        QVector<QUrl> trackUris;
        QDBusMessage reply;
        for (const auto uri : uris)
        {
            UriCheck uri_check(uri);
            valid_uri = !uri_check.is_local_file() or
                    (uri_check.is_local_file() and uri_check.file_exists());
            if (!valid_uri)
            {
                uri_err_str = {"Warning: Not adding track " + uri +
                     " to TrackList because it can't be found."};
                MH_WARNING() << uri_err_str;
                reply = in.createErrorReply(
                            mpris::Player::Error::UriNotFound::name,
                            err_str);
            }

            // Make sure the client has adequate apparmor permissions to open the URI
            result = d->request_authenticator->authenticate_open_uri_request(context, uri);
            if (not std::get<0>(result))
            {
                err_str = {"Warning: Not adding track " + uri +
                    " to TrackList because of inadequate client apparmor permissions."};
                break;
            }

            trackUris.append(QUrl::fromUserInput(uri));
        }

        // Only add the track to the TrackList if it passes the apparmor permissions check
        if (std::get<0>(result))
        {
            reply = in.createReply();
            d->m_impl->add_tracks_with_uri_at(trackUris, after);
        }
        else
        {
            MH_WARNING() << err_str;
            reply = in.createErrorReply(
                        mpris::TrackList::Error::InsufficientPermissionsToAddTrack::name,
                        err_str);
        }

        bus.send(reply);
    });
}

void TrackListSkeleton::MoveTrack(const QString &id, const QString &to)
{
    Q_D(TrackListSkeleton);
    try {
        const bool ret = d->m_impl->move_track(id, to);
        if (!ret)
        {
            const QString err_str = {"Error: Not moving track " + id +
                " to destination " + to};
            MH_WARNING() << err_str;
            sendErrorReply(
                    mpris::TrackList::Error::FailedToMoveTrack::name,
                    err_str);
        }
    } catch(media::TrackList::Errors::FailedToMoveTrack& e) {
        sendErrorReply(
                mpris::TrackList::Error::FailedToFindMoveTrackSource::name,
                e.what());
    } catch(media::TrackList::Errors::FailedToFindMoveTrackSource& e) {
        sendErrorReply(
                mpris::TrackList::Error::FailedToFindMoveTrackSource::name,
                e.what());
    } catch(media::TrackList::Errors::FailedToFindMoveTrackDest& e) {
        sendErrorReply(
                mpris::TrackList::Error::FailedToFindMoveTrackDest::name,
                e.what());
    }
}

void TrackListSkeleton::RemoveTrack(const QString &id)
{
    Q_D(TrackListSkeleton);
    try {
        d->m_impl->remove_track(id);
    } catch(media::TrackList::Errors::TrackNotFound& e) {
        sendErrorReply(
                mpris::TrackList::Error::TrackNotFound::name,
                e.what());
    }
}

void TrackListSkeleton::GoTo(const QString &id)
{
    Q_D(TrackListSkeleton);
    d->m_impl->go_to(id);
}

void TrackListSkeleton::Reset()
{
    Q_D(TrackListSkeleton);
    d->m_impl->reset();
}
