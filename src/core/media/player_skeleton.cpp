/*
 * Copyright © 2013-2015 Canonical Ltd.
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

#include "player_skeleton.h"

#include "dbus_property_notifier.h"
#include "engine.h"
#include "logging.h"
#include "mpris.h"
#include "player_implementation.h"
#include "xesam.h"

#include "apparmor/ubuntu.h"

#include "util/uri_check.h"

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDateTime>
#include <QMetaEnum>
#include <QTimer>

using namespace core::ubuntu::media;

namespace core {
namespace ubuntu {
namespace media {

enum class OpenUriCall {
    OnlyUri,
    UriWithHeaders,
};

class PlayerSkeletonPrivate
{
public:
    PlayerSkeletonPrivate(const PlayerSkeleton::Configuration &conf,
                          media::PlayerSkeleton *q);

    void openUri(const QDBusMessage &in, const QDBusConnection &bus,
                 OpenUriCall callType);

private:
    friend class PlayerSkeleton;
    PlayerImplementation *m_player;
    QDBusConnection m_connection;
    media::apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
    media::apparmor::ubuntu::RequestAuthenticator::Ptr request_authenticator;
    QTimer m_bufferingTimer;
    QDateTime m_bufferingLastEmission;
    int m_bufferingValue;
    PlayerSkeleton *q_ptr;
};

}}} // namespace

PlayerSkeletonPrivate::PlayerSkeletonPrivate(
        const PlayerSkeleton::Configuration &conf,
        media::PlayerSkeleton *q):
    m_player(conf.player),
    m_connection(conf.connection),
    request_context_resolver{conf.request_context_resolver},
    request_authenticator{conf.request_authenticator},
    q_ptr(q)
{
    auto impl = m_player;
    QObject::connect(impl, &PlayerImplementation::seekedTo,
                     q, &PlayerSkeleton::Seeked);
    QObject::connect(impl, &PlayerImplementation::aboutToFinish,
                     q, &PlayerSkeleton::AboutToFinish);
    QObject::connect(impl, &PlayerImplementation::endOfStream,
                     q, [q, impl]() {
        /* For compatibility with the old media-hub (and the music
         * app), do not emit the signal unless this is the very
         * last track of the playlist.
         */
        if (!impl->canGoNext()) {
            Q_EMIT q->EndOfStream();
        }
    });
    QObject::connect(impl, &PlayerImplementation::playbackStatusChanged,
                     q, [q,impl]() {
        Q_EMIT q->PlaybackStatusChanged(impl->playbackStatus());
    });
    QObject::connect(impl, &PlayerImplementation::videoDimensionChanged,
                     q, [q,impl]() {
        const QSize size = impl->videoDimension();
        Q_EMIT q->VideoDimensionChanged(size.height(), size.width());
    });
    QObject::connect(impl, &PlayerImplementation::errorOccurred,
                     q, [q](Player::Error error) {
        Q_EMIT q->Error(error);
    });
    QObject::connect(impl, &PlayerImplementation::bufferingChanged,
                     q, [this, q](int buffering) {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_bufferingLastEmission.isValid() ||
            m_bufferingLastEmission.msecsTo(now) > 500) {
            /* More than 0.5 seconds have passed, emit immediately */
            Q_EMIT q->Buffering(buffering);
            m_bufferingLastEmission = now;
        } else {
            /* wait 0.5 seconds since the previous emission */
            QDateTime nextEmission = m_bufferingLastEmission.addMSecs(500);
            m_bufferingValue = buffering;
            m_bufferingTimer.start(now.msecsTo(nextEmission));
        }
    });
    m_bufferingTimer.setSingleShot(true);
    m_bufferingTimer.callOnTimeout(q, [this, q]() {
        Q_EMIT q->Buffering(m_bufferingValue);
        m_bufferingLastEmission = QDateTime::currentDateTime();
    });

    QObject::connect(impl, &PlayerImplementation::volumeChanged,
                     q, &PlayerSkeleton::volumeChanged);

    /* Property signals */
    QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                     q, &PlayerSkeleton::canPlayChanged);
    QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                     q, &PlayerSkeleton::canPauseChanged);
    QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                     q, &PlayerSkeleton::canGoPreviousChanged);
    QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                     q, &PlayerSkeleton::canGoNextChanged);
    QObject::connect(impl, &PlayerImplementation::isVideoSourceChanged,
                     q, &PlayerSkeleton::isVideoSourceChanged);
    QObject::connect(impl, &PlayerImplementation::isAudioSourceChanged,
                     q, &PlayerSkeleton::isAudioSourceChanged);
    QObject::connect(impl, &PlayerImplementation::metadataForCurrentTrackChanged,
                     q, &PlayerSkeleton::metadataChanged);
    QObject::connect(impl, &PlayerImplementation::orientationChanged,
                     q, &PlayerSkeleton::orientationChanged);
}

void PlayerSkeletonPrivate::openUri(const QDBusMessage &in,
                                    const QDBusConnection &bus,
                                    OpenUriCall callType)
{
    in.setDelayedReply(true);
    request_context_resolver->resolve_context_for_dbus_name_async(in.service(),
        [=](const media::apparmor::ubuntu::Context& context)
    {
        using Headers = Player::HeadersType;

        const auto args = in.arguments();
        QUrl uri = QUrl::fromUserInput(args.value(0).toString());
        Headers headers;
        if (callType == OpenUriCall::UriWithHeaders) {
            const QDBusArgument dbusHeaders = args.value(1).value<QDBusArgument>();
            dbusHeaders >> headers;
        }

        QDBusMessage reply;
        UriCheck uri_check(uri);
        const bool valid_uri = !uri.isEmpty() and
            (!uri_check.is_local_file() or uri_check.file_exists());
        if (!valid_uri)
        {
            const QString err_str = {"Warning: Failed to open uri " + uri.toString() +
                 " because it can't be found."};
            MH_ERROR("%s", qUtf8Printable(err_str));
            reply = in.createErrorReply(
                        mpris::Player::Error::UriNotFound::name,
                        err_str);
        }
        else
        {
            // Make sure the client has adequate apparmor permissions to open the URI
            const auto result = request_authenticator->authenticate_open_uri_request(context, uri);
            if (std::get<0>(result))
            {
                reply = in.createReply();
                reply << (std::get<0>(result) ? m_player->open_uri(uri, headers) : false);
            }
            else
            {
                const QString err_str = {"Warning: Failed to authenticate necessary "
                    "apparmor permissions to open uri: " + std::get<1>(result)};
                MH_ERROR("%s", qUtf8Printable(err_str));
                reply = in.createErrorReply(
                            mpris::Player::Error::InsufficientAppArmorPermissions::name,
                            err_str);
            }
        }

        bus.send(reply);
    });
}

PlayerSkeleton::PlayerSkeleton(const Configuration& configuration,
                               QObject *parent):
    QObject(parent),
    d_ptr(new PlayerSkeletonPrivate(configuration, this))
{
}

PlayerSkeleton::~PlayerSkeleton() = default;

PlayerImplementation *PlayerSkeleton::player() {
    Q_D(PlayerSkeleton);
    return d->m_player;
}

const PlayerImplementation *PlayerSkeleton::player() const {
    Q_D(const PlayerSkeleton);
    return d->m_player;
}

bool PlayerSkeleton::registerAt(const QString &objectPath)
{
    Q_D(PlayerSkeleton);
    new DBusPropertyNotifier(d->m_connection, objectPath, this);
    return d->m_connection.registerObject(
            objectPath,
            this,
            QDBusConnection::ExportAllSlots |
            QDBusConnection::ExportScriptableSignals |
            QDBusConnection::ExportAllProperties);
}

bool PlayerSkeleton::canPlay() const
{
    return player()->canPlay();
}

bool PlayerSkeleton::canPause() const
{
    return player()->canPause();
}

bool PlayerSkeleton::canSeek() const
{
    return player()->canSeek();
}

bool PlayerSkeleton::canGoPrevious() const
{
    return player()->canGoPrevious();
}

bool PlayerSkeleton::canGoNext() const
{
    return player()->canGoNext();
}

bool PlayerSkeleton::isVideoSource() const
{
    return player()->isVideoSource();
}

bool PlayerSkeleton::isAudioSource() const
{
    return player()->isAudioSource();
}

QString PlayerSkeleton::playbackStatus() const
{
    const Player::PlaybackStatus s = player()->playbackStatus();
    switch (s) {
    case Player::PlaybackStatus::playing:
        return QStringLiteral("Playing");
    case Player::PlaybackStatus::paused:
    case Player::PlaybackStatus::ready:
        return QStringLiteral("Paused");
    default:
        return QStringLiteral("Stopped");
    }
}

void PlayerSkeleton::setLoopStatus(const QString &status)
{
    bool ok;
    int value = QMetaEnum::fromType<LoopStatus>().
        keyToValue(status.toUtf8().constData(), &ok);
    if (!ok) {
        MH_ERROR("Invalid loop status: %s", qUtf8Printable(status));
        return;
    }
    player()->setLoopStatus(static_cast<Player::LoopStatus>(value));
}

QString PlayerSkeleton::loopStatus() const
{
    int value = static_cast<LoopStatus>(player()->loopStatus());
    return QMetaEnum::fromType<LoopStatus>().valueToKey(value);
}

void PlayerSkeleton::setTypedLoopStatus(int16_t status)
{
    player()->setLoopStatus(static_cast<Player::LoopStatus>(status));
}

qint16 PlayerSkeleton::typedLoopStatus() const
{
    return static_cast<LoopStatus>(player()->loopStatus());
}

void PlayerSkeleton::setPlaybackRate(double rate)
{
    return player()->setPlaybackRate(rate);
}

double PlayerSkeleton::playbackRate() const
{
    return player()->playbackRate();
}

void PlayerSkeleton::setShuffle(bool shuffle)
{
    return player()->setShuffle(shuffle);
}

bool PlayerSkeleton::shuffle() const
{
    return player()->shuffle();
}

QVariantMap PlayerSkeleton::metadata() const
{
    return player()->metadataForCurrentTrack();
}

void PlayerSkeleton::setVolume(double volume)
{
    player()->setVolume(volume);
}

double PlayerSkeleton::volume() const
{
    return player()->volume();
}

double PlayerSkeleton::minimumRate() const
{
    return player()->minimumRate();
}

double PlayerSkeleton::maximumRate() const
{
    return player()->maximumRate();
}

qint64 PlayerSkeleton::position() const
{
    return player()->position();
}

qint64 PlayerSkeleton::duration() const
{
    return player()->duration();
}

qint16 PlayerSkeleton::backend() const
{
    return player()->backend();
}

qint16 PlayerSkeleton::orientation() const
{
    return player()->orientation();
}

qint16 PlayerSkeleton::lifetime() const
{
    return player()->lifetime();
}

void PlayerSkeleton::setAudioStreamRole(qint16 role)
{
    player()->setAudioStreamRole(static_cast<Player::AudioStreamRole>(role));
}

qint16 PlayerSkeleton::audioStreamRole() const
{
    return player()->audioStreamRole();
}

void PlayerSkeleton::Next()
{
    player()->next();
}

void PlayerSkeleton::Previous()
{
    player()->previous();
}

void PlayerSkeleton::Pause()
{
    player()->pause();
}

void PlayerSkeleton::PlayPause()
{
    Q_D(PlayerSkeleton);

    PlayerImplementation *impl = player();
    switch (impl->playbackStatus()) {
    case core::ubuntu::media::Player::PlaybackStatus::ready:
    case core::ubuntu::media::Player::PlaybackStatus::paused:
    case core::ubuntu::media::Player::PlaybackStatus::stopped:
        impl->play();
        break;
    case core::ubuntu::media::Player::PlaybackStatus::playing:
        impl->pause();
        break;
    default:
        break;
    }
}

void PlayerSkeleton::Stop()
{
    player()->stop();
}

void PlayerSkeleton::Play()
{
    player()->play();
    /* FIXME: workaround for the client library: if we don't emit the signal
     * right away, it gets confused and will pause the playback.
     */
    Q_EMIT PlaybackStatusChanged(Player::PlaybackStatus::playing);
}

void PlayerSkeleton::Seek(quint64 microSeconds)
{
    player()->seek_to(std::chrono::microseconds(microSeconds));
}

void PlayerSkeleton::SetPosition(const QDBusObjectPath &, quint64)
{
    // TODO: implement (this was never implemented in media-hub)
}

void PlayerSkeleton::CreateVideoSink(quint32 textureId)
{
    try
    {
        player()->create_gl_texture_video_sink(textureId);
    }
    catch (const media::Player::Errors::OutOfProcessBufferStreamingNotSupported& e)
    {
        sendErrorReply(
                    mpris::Player::Error::OutOfProcessBufferStreamingNotSupported::name,
                    e.what());
    }
    catch (...)
    {
        sendErrorReply(
                    mpris::Player::Error::OutOfProcessBufferStreamingNotSupported::name,
                    QString());
    }
}

uint32_t PlayerSkeleton::Key() const
{
    return player()->key();
}

void PlayerSkeleton::OpenUri(const QDBusMessage &)
{
    Q_D(PlayerSkeleton);
    d->openUri(message(), connection(), OpenUriCall::OnlyUri);
}

void PlayerSkeleton::OpenUriExtended(const QDBusMessage &)
{
    Q_D(PlayerSkeleton);
    d->openUri(message(), connection(), OpenUriCall::UriWithHeaders);
}
