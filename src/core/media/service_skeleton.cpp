/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */

#include "service_skeleton.h"

#include "dbus_property_notifier.h"
#include "mpris.h"
#include "mpris/media_player2.h"
#include "player_implementation.h"
#include "player_skeleton.h"
#include "service_implementation.h"
#include "track_list_implementation.h"
#include "track_list_skeleton.h"

#include "xesam.h"

#include "logging.h"

#include <QDBusMessage>
#include <QUuid>

namespace media = core::ubuntu::media;

using namespace media;

namespace core {
namespace ubuntu {
namespace media {

struct SessionInfo {
    Player::PlayerKey key;
    QString objectPath;
    QString uuid;
};

struct OwnerInfo {
    QString profile;
    bool isAttached = false;
    QString serviceName;
};

class ServiceSkeletonPrivate
{
    Q_DECLARE_PUBLIC(ServiceSkeleton)

public:
    ServiceSkeletonPrivate(const ServiceSkeleton::Configuration &config,
                           ServiceImplementation *impl,
                           ServiceSkeleton *q);

    void onCurrentPlayerChanged();

    SessionInfo createSessionInfo()
    {
        static unsigned int session_counter = 0;

        const unsigned int current_session = session_counter++;

        return {
            Player::PlayerKey(current_session),
            pathForPlayer(current_session),
            QUuid::createUuid().toString(),
        };
    }

    QString pathForPlayer(Player::PlayerKey key) const;
    void exportPlayer(const SessionInfo &sessionInfo);

    bool playerKeyFromUuid(const QString &uuid, Player::PlayerKey &key) const;
    bool uuidIsValid(const QString &uuid, Player::PlayerKey &key) const;

    media::apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
    media::apparmor::ubuntu::RequestAuthenticator::Ptr request_authenticator;
    QDBusConnection m_connection;
    // We map named/fixed player instances to their respective keys.
    QMap<QString, Player::PlayerKey> named_player_map;
    // We map UUIDs to their respective keys.
    QMap<QString, Player::PlayerKey> uuid_player_map;
    // We keep a list of keys and their respective owners and states.
    QMap<media::Player::PlayerKey, OwnerInfo> player_owner_map;
    mpris::MediaPlayer2 m_mprisAdaptor;

    ServiceImplementation *impl;
    ServiceSkeleton *q_ptr;
};

}}} // namespace

ServiceSkeletonPrivate::ServiceSkeletonPrivate(
        const ServiceSkeleton::Configuration &config,
        ServiceImplementation *impl,
        ServiceSkeleton *q):
    request_context_resolver(QSharedPointer<apparmor::ubuntu::DBusDaemonRequestContextResolver>::create()),
    request_authenticator(QSharedPointer<apparmor::ubuntu::ExistingAuthenticator>::create()),
    m_connection(config.connection),
    impl(impl),
    q_ptr(q)
{
    QObject::connect(impl, &ServiceImplementation::currentPlayerChanged,
                     q, [this]() { onCurrentPlayerChanged(); });
    bool ok = m_mprisAdaptor.registerObject();
    if (!ok) {
        MH_ERROR() << "Failed to register MPRIS object";
    }
}

void ServiceSkeletonPrivate::onCurrentPlayerChanged()
{
    Player::PlayerKey key = impl->currentPlayer();

    if (key != Player::invalidKey) {
        PlayerImplementation *player = impl->playerByKey(key);

        // We only care to allow the MPRIS controls to apply to multimedia player (i.e. audio, video)
        if (player->audioStreamRole() == Player::AudioStreamRole::multimedia) {
            m_mprisAdaptor.setPlayer(player);
        }
    } else {
        m_mprisAdaptor.setPlayer(nullptr);
    }
}

QString ServiceSkeletonPrivate::pathForPlayer(Player::PlayerKey key) const
{
    return "/core/ubuntu/media/Service/sessions/" + QString::number(key);
}

void ServiceSkeletonPrivate::exportPlayer(const SessionInfo &info)
{
    Q_Q(ServiceSkeleton);

    PlayerImplementation *player = impl->playerByKey(info.key);
    if (!player) {
        MH_ERROR("Requested adaptor for non-existing player %d", info.key);
        return;
    }

    PlayerSkeleton::Configuration conf {
        m_connection,
        request_context_resolver,
        request_authenticator,
    };

    /* Use the player as parent object to cause automatic
     * distruction of the adaptor when the player dies.
     */
    auto *adaptor = new PlayerSkeleton(conf, player);
    adaptor->setPlayer(player);
    player->setObjectName(info.objectPath);

    bool ok = adaptor->registerAt(info.objectPath);
    if (!ok) {
        MH_ERROR("Failed to export player %d", info.key);
        return;
    }

    auto trackList = player->trackList();
    auto trackListAdaptor =
        new TrackListSkeleton(m_connection,
                              request_context_resolver,
                              request_authenticator,
                              trackList.data(),
                              trackList.data());
    ok = m_connection.registerObject(trackList->objectName(),
                                     trackListAdaptor,
                                     QDBusConnection::ExportAllSlots |
                                     QDBusConnection::ExportScriptableSignals |
                                     QDBusConnection::ExportAllProperties);
    if (!ok) {
        MH_ERROR("Failed to export TrackList for %d", info.key);
        return;
    }
}

bool ServiceSkeletonPrivate::playerKeyFromUuid(const QString &uuid,
                                               Player::PlayerKey &key) const
{
    const auto i = uuid_player_map.find(uuid);
    if (i == uuid_player_map.end()) return false;
    key = i.value();
    return true;
}

bool ServiceSkeletonPrivate::uuidIsValid(const QString &uuid,
                                         Player::PlayerKey &key) const
{
    bool ok = playerKeyFromUuid(uuid, key);
    if (!ok) return false;
    return impl->playerByKey(key) != nullptr;
}

ServiceSkeleton::ServiceSkeleton(const Configuration &configuration,
                                 ServiceImplementation *impl,
                                 QObject *parent):
    QObject(parent),
    d_ptr(new ServiceSkeletonPrivate(configuration, impl, this))
{
    /* We register all the types used in the D-Bus interfaces here */
    qRegisterMetaType<int16_t>("int16_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<uint64_t>("uint64_t");
}

media::ServiceSkeleton::~ServiceSkeleton()
{
}

void ServiceSkeleton::CreateSession(QDBusObjectPath &op, QString &uuid)
{
    Q_D(ServiceSkeleton);

    QDBusMessage msg = message();
    QDBusConnection bus = connection();

    const auto sessionInfo = d->createSessionInfo();
    const Player::PlayerKey key = sessionInfo.key;

    MH_DEBUG("Session created by request of: %s, key: %d, uuid: %s",
             qUtf8Printable(msg.service()), key, qUtf8Printable(sessionInfo.uuid));

    try
    {
        d->impl->create_session(key);
        d->uuid_player_map[sessionInfo.uuid] = key;

        QString sender = msg.service();
        d->request_context_resolver->resolve_context_for_dbus_name_async(sender,
                [this, key, sender](const media::apparmor::ubuntu::Context& context)
        {
            Q_D(ServiceSkeleton);
            MH_DEBUG(" -- app_name='%s', attached", qUtf8Printable(context.str()));
            d->player_owner_map[key] = OwnerInfo { context.str(), true, sender };
        });
    } catch(const std::runtime_error& e)
    {
        sendErrorReply(
                    mpris::Service::Errors::CreatingSession::name(),
                    e.what());
        return;
    }

    d->exportPlayer(sessionInfo);
    uuid = sessionInfo.uuid;
    op = QDBusObjectPath(sessionInfo.objectPath);
}

void ServiceSkeleton::DetachSession(const QString &uuid)
{
    Q_D(ServiceSkeleton);
    try
    {
        Player::PlayerKey key;
        if (!d->uuidIsValid(uuid, key)) return;
        if (!d->player_owner_map.contains(key)) return;

        auto &info = d->player_owner_map[key];
        // Check if session is attached(1) and that the detachment
        // request comes from the same peer(2) that created the session.
        if (info.isAttached && info.serviceName == message().service()) { // Player is attached
            info.isAttached = false; // Detached
            info.serviceName.clear(); // Clear registered sender/peer

            auto player = d->impl->playerByKey(key);
            player->setLifetime(Player::Lifetime::resumable);
        }
    } catch(const std::runtime_error& e)
    {
        sendErrorReply(
                    mpris::Service::Errors::DetachingSession::name(),
                    e.what());
    }
}

void ServiceSkeleton::ReattachSession(const QString &uuid)
{
    Q_D(ServiceSkeleton);
    Player::PlayerKey key;
    if (!d->uuidIsValid(uuid, key)) {
        sendErrorReply(mpris::Service::Errors::ReattachingSession::name(),
                       "Invalid session");
        return;
    }

    QDBusObjectPath op(d->pathForPlayer(key));

    QDBusMessage msg = message();
    QDBusConnection bus = connection();
    msg.setDelayedReply(true);

    try
    {
        d->request_context_resolver->resolve_context_for_dbus_name_async(msg.service(),
                [this, msg, bus, key, op](const media::apparmor::ubuntu::Context& context)
        {
            Q_D(ServiceSkeleton);
            auto &info = d->player_owner_map[key];
            MH_DEBUG(" -- reattach app_name='%s', info='%s', '%s'",
                     qUtf8Printable(context.str()),
                     qUtf8Printable(info.profile),
                     qUtf8Printable(info.serviceName));
            if (info.profile == context.str()) {
                info.isAttached = true; // Set to Attached
                info.serviceName = msg.service(); // Register new owner

                // Signal player reconnection
                PlayerImplementation *player = d->impl->playerByKey(key);
                player->reconnect();

                auto reply = msg.createReply();
                reply << QVariant::fromValue(op);

                bus.send(reply);
            }
            else {
                auto reply = msg.createErrorReply(
                            mpris::Service::Errors::ReattachingSession::name(),
                            "Invalid permissions for the requested session");
                bus.send(reply);
                return;
            }
        });
    } catch(const std::runtime_error& e)
    {
        sendErrorReply(
                    mpris::Service::Errors::ReattachingSession::name(),
                    e.what());
    }
}

void ServiceSkeleton::DestroySession(const QString &uuid)
{
    Q_D(ServiceSkeleton);

    Player::PlayerKey key;
    if (!d->uuidIsValid(uuid, key)) {
        sendErrorReply(mpris::Service::Errors::DestroyingSession::name(),
                       "Invalid session");
        return;
    }

    try
    {
        QDBusMessage msg = message();
        QDBusConnection bus = connection();
        msg.setDelayedReply(true);

        d->request_context_resolver->resolve_context_for_dbus_name_async(msg.service(),
                [this, msg, bus, uuid, key](const media::apparmor::ubuntu::Context& context)
        {
            Q_D(ServiceSkeleton);
            auto info = d->player_owner_map.value(key);
            MH_DEBUG(" -- Destroying app_name='%s', info='%s', '%s'",
                     qUtf8Printable(context.str()),
                     qUtf8Printable(info.profile),
                     qUtf8Printable(info.serviceName));
            if (info.profile == context.str()) {
                // Remove control entries from the map, at this point
                // the session is no longer usable.
                d->uuid_player_map.remove(uuid);
                d->player_owner_map.remove(key);

                // Reset lifecycle to non-resumable on the now-abandoned session
                PlayerImplementation *player = d->impl->playerByKey(key);

                // Delete player instance by abandonment
                player->setLifetime(media::Player::Lifetime::normal);
                player->abandon();

                bus.send(msg.createReply());
            }
            else {
                auto reply = msg.createErrorReply(
                            mpris::Service::Errors::DestroyingSession::name(),
                            "Invalid permissions for the requested session");
                bus.send(reply);
                return;
            }
        });
    } catch(const std::runtime_error& e)
    {
        sendErrorReply(mpris::Service::Errors::DestroyingSession::name(),
                       e.what());
    }
}

QDBusObjectPath ServiceSkeleton::CreateFixedSession(const QString &name)
{
    Q_D(ServiceSkeleton);

    try
    {
        if (d->named_player_map.contains(name) == 0) {
            // Create new session
            auto sessionInfo = d->createSessionInfo();

            QDBusObjectPath op(sessionInfo.objectPath);
            Player::PlayerKey key = sessionInfo.key;

            auto session = d->impl->create_session(key);
            session->setLifetime(media::Player::Lifetime::resumable);

            d->exportPlayer(sessionInfo);
            d->named_player_map.insert(name, key);
            return op;
        } else {
            // Resume previous session
            const auto player = d->named_player_map.contains(name) ?
                d->impl->playerByKey(d->named_player_map[name]) : nullptr;
            if (not player) {
                sendErrorReply(
                            mpris::Service::Errors::CreatingFixedSession::name(),
                            "Unable to locate player session");
                return {};
            }

            return QDBusObjectPath(d->pathForPlayer(player->key()));
        }
    } catch(const std::runtime_error& e)
    {
        sendErrorReply(mpris::Service::Errors::CreatingFixedSession::name(),
                       e.what());
    }
    return {};
}

QDBusObjectPath ServiceSkeleton::ResumeSession(Player::PlayerKey key)
{
    Q_D(ServiceSkeleton);

    // FIXME This method does nothing, and never did

    auto player = d->impl->playerByKey(key);
    if (not player) {
        sendErrorReply(mpris::Service::Errors::ResumingSession::name(),
                       "Unable to locate player session");
        return {};
    }

    return QDBusObjectPath(d->pathForPlayer(key));
}

void ServiceSkeleton::PauseOtherSessions(Player::PlayerKey key)
{
    Q_D(ServiceSkeleton);

    try {
        d->impl->pause_other_sessions(key);
    }
    catch (const std::out_of_range &e) {
            MH_WARNING("Failed to look up Player instance for key %d\
                , no valid Player instance for that key value and cannot set current player.\
                This most likely means that media-hub-server has crashed and restarted.", key);
        sendErrorReply(
                mpris::Service::Errors::PlayerKeyNotFound::name(),
                "Player key not found");
    }
}
