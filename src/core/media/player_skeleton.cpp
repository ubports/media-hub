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

#include "engine.h"
#include "logging.h"
#include "mpris.h"
#include "player_implementation.h"
#include "xesam.h"

#include "apparmor/ubuntu.h"

#include "util/uri_check.h"

#include <QDBusMessage>
#include <QMetaEnum>

using namespace core::ubuntu::media;

namespace core {
namespace ubuntu {
namespace media {

class PlayerSkeletonPrivate
{
public:
    PlayerSkeletonPrivate(const QDBusConnection &connection,
                          const apparmor::ubuntu::RequestContextResolver::Ptr &request_context_resolver,
                          const apparmor::ubuntu::RequestAuthenticator::Ptr &request_authenticator,
                          media::PlayerSkeleton *q):
        m_player(nullptr),
        m_connection(connection),
        request_context_resolver{request_context_resolver},
        request_authenticator{request_authenticator},
        q_ptr(q)
    {
    }

private:
    friend class PlayerSkeleton;
    PlayerImplementation *m_player;
    QDBusConnection m_connection;
    media::apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
    media::apparmor::ubuntu::RequestAuthenticator::Ptr request_authenticator;
    PlayerSkeleton *q_ptr;
};

}}} // namespace

PlayerSkeleton::PlayerSkeleton(const Configuration& configuration,
                               QObject *parent):
    QDBusAbstractAdaptor(parent),
    d_ptr(new PlayerSkeletonPrivate(configuration.connection,
                                    configuration.request_context_resolver,
                                    configuration.request_authenticator,
                                    this))
{
}

PlayerSkeleton::~PlayerSkeleton() = default;

void PlayerSkeleton::setPlayer(PlayerImplementation *impl)
{
    Q_D(PlayerSkeleton);

    if (d->m_player == impl) return;

    if (d->m_player) {
        d->m_player->disconnect(this);
    }

    d->m_player = impl;
    if (impl) {
        QObject::connect(impl, &PlayerImplementation::seekedTo,
                         this, &PlayerSkeleton::Seeked);
        QObject::connect(impl, &PlayerImplementation::aboutToFinish,
                         this, &PlayerSkeleton::AboutToFinish);
        QObject::connect(impl, &PlayerImplementation::endOfStream,
                         this, &PlayerSkeleton::EndOfStream);
        QObject::connect(impl, &PlayerImplementation::playbackStatusChanged,
                         this, [this,impl]() {
            Q_EMIT PlaybackStatusChanged(impl->playbackStatus());
        });
        QObject::connect(impl, &PlayerImplementation::videoDimensionChanged,
                         this, [this,impl]() {
            const QSize size = impl->videoDimension();
            Q_EMIT VideoDimensionChanged(size.height(), size.width());
        });
        QObject::connect(impl, &PlayerImplementation::errorOccurred,
                         this, [this,impl](Player::Error error) {
            Q_EMIT Error(error);
        });
        QObject::connect(impl, &PlayerImplementation::bufferingChanged,
                         this, &PlayerSkeleton::Buffering);

        QObject::connect(impl, &PlayerImplementation::volumeChanged,
                         this, &PlayerSkeleton::volumeChanged);

        /* Property signals */
        QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                         this, &PlayerSkeleton::canPlayChanged);
        QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                         this, &PlayerSkeleton::canPauseChanged);
        QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                         this, &PlayerSkeleton::canGoPreviousChanged);
        QObject::connect(impl, &PlayerImplementation::mprisPropertiesChanged,
                         this, &PlayerSkeleton::canGoNextChanged);
        QObject::connect(impl, &PlayerImplementation::metadataForCurrentTrackChanged,
                         this, &PlayerSkeleton::metadataChanged);
        QObject::connect(impl, &PlayerImplementation::orientationChanged,
                         this, &PlayerSkeleton::orientationChanged);
    }

    Q_EMIT canControlChanged();
}

PlayerImplementation *PlayerSkeleton::player() {
    Q_D(PlayerSkeleton);
    return d->m_player;
}

const PlayerImplementation *PlayerSkeleton::player() const {
    Q_D(const PlayerSkeleton);
    return d->m_player;
}

bool PlayerSkeleton::canPlay() const
{
    return player() ? player()->canPlay() : false;
}

bool PlayerSkeleton::canPause() const
{
    return player() ? player()->canPause() : false;
}

bool PlayerSkeleton::canSeek() const
{
    return player() ? player()->canSeek() : false;
}

bool PlayerSkeleton::canGoPrevious() const
{
    return player() ? player()->canGoPrevious() : false;
}

bool PlayerSkeleton::canGoNext() const
{
    return player() ? player()->canGoNext() : false;
}

bool PlayerSkeleton::canControl() const
{
    return player() ? true : false;
}

bool PlayerSkeleton::isVideoSource() const
{
    return player() ? player()->isVideoSource() : false;
}

bool PlayerSkeleton::isAudioSource() const
{
    return player() ? player()->isAudioSource() : false;
}

QString PlayerSkeleton::playbackStatus() const
{
    const Player::PlaybackStatus s = player() ?
        player()->playbackStatus() : Player::PlaybackStatus::stopped;
    switch (s) {
    case Player::PlaybackStatus::playing:
        return QStringLiteral("Playing");
    case Player::PlaybackStatus::paused:
        return QStringLiteral("Paused");
    default:
        return QStringLiteral("Stopped");
    }
}

void PlayerSkeleton::setLoopStatus(const QString &status)
{
    if (!player()) return;
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
    int value = player() ? static_cast<LoopStatus>(player()->loopStatus()) : None;
    return QMetaEnum::fromType<LoopStatus>().valueToKey(value);
}

void PlayerSkeleton::setTypedLoopStatus(int16_t status)
{
    if (!player()) return;
    player()->setLoopStatus(static_cast<Player::LoopStatus>(status));
}

int16_t PlayerSkeleton::typedLoopStatus() const
{
    return player() ? static_cast<LoopStatus>(player()->loopStatus()) : None;
}

void PlayerSkeleton::setPlaybackRate(double rate)
{
    if (!player()) return;
    return player()->setPlaybackRate(rate);
}

double PlayerSkeleton::playbackRate() const
{
    return player() ? player()->playbackRate() : 1.0;
}

void PlayerSkeleton::setShuffle(bool shuffle)
{
    if (!player()) return;
    return player()->setShuffle(shuffle);
}

bool PlayerSkeleton::shuffle() const
{
    return player() ? player()->shuffle() : false;
}

QVariantMap PlayerSkeleton::metadata() const
{
    if (!player()) return QVariantMap();
    return player()->metadataForCurrentTrack();
}

void PlayerSkeleton::setVolume(double volume)
{
    if (!player()) return;
    return player()->setVolume(volume);
}

double PlayerSkeleton::volume() const
{
    return player() ? player()->volume() : 1.0;
}

double PlayerSkeleton::minimumRate() const
{
    return player() ? player()->minimumRate() : 1.0;
}

double PlayerSkeleton::maximumRate() const
{
    return player() ? player()->maximumRate() : 1.0;
}

int64_t PlayerSkeleton::position() const
{
    return player() ? player()->position() : 0;
}

int64_t PlayerSkeleton::duration() const
{
    return player() ? player()->duration() : 0;
}

int16_t PlayerSkeleton::backend() const
{
    return player() ? player()->backend() : 0;
}

int16_t PlayerSkeleton::orientation() const
{
    return player() ? player()->orientation() : 0;
}

int16_t PlayerSkeleton::lifetime() const
{
    return player() ? player()->lifetime() : 0;
}

int16_t PlayerSkeleton::audioStreamRole() const
{
    return player() ? player()->audioStreamRole() : 0;
}

void PlayerSkeleton::Next()
{
    if (!player()) return;
    player()->next();
}

void PlayerSkeleton::Previous()
{
    if (!player()) return;
    player()->previous();
}

void PlayerSkeleton::Pause()
{
    if (!player()) return;
    player()->pause();
}

void PlayerSkeleton::PlayPause()
{
    Q_D(PlayerSkeleton);

    if (!player()) return;

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
    if (!player()) return;
    player()->stop();
}

void PlayerSkeleton::Play()
{
    if (!player()) return;
    player()->play();
}

void PlayerSkeleton::Seek(uint64_t microSeconds)
{
    if (!player()) return;
    player()->seek_to(std::chrono::microseconds(microSeconds));
}

void PlayerSkeleton::SetPosition(const QDBusObjectPath &, uint64_t)
{
    // TODO: implement (this was never implemented in media-hub)
}

void PlayerSkeleton::CreateVideoSink(uint32_t textureId)
{
    if (!player()) return;

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
    if (!player()) return 0;
    return player()->key();
}

void PlayerSkeleton::OpenUri(const QDBusMessage &)
{
    Q_D(PlayerSkeleton);
    QDBusMessage in = message();
    QDBusConnection bus = connection();
    in.setDelayedReply(true);
    d->request_context_resolver->resolve_context_for_dbus_name_async(in.service(),
        [=](const media::apparmor::ubuntu::Context& context)
    {
        QUrl uri = QUrl::fromUserInput(in.arguments()[0].toString());

        QDBusMessage reply;
        UriCheck uri_check(uri);
        const bool valid_uri = !uri_check.is_local_file() or
                (uri_check.is_local_file() and uri_check.file_exists());
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
            const auto result = d->request_authenticator->authenticate_open_uri_request(context, uri);
            if (std::get<0>(result))
            {
                reply = in.createReply();
                reply << (std::get<0>(result) ? player()->open_uri(uri) : false);
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

void PlayerSkeleton::OpenUriExtended(const QDBusMessage &)
{
    Q_D(PlayerSkeleton);
    QDBusMessage in = message();
    QDBusConnection bus = connection();
    in.setDelayedReply(true);
    d->request_context_resolver->resolve_context_for_dbus_name_async(in.service(),
        [=](const media::apparmor::ubuntu::Context& context)
    {
        using Headers = Player::HeadersType;

        const auto args = in.arguments();
        QUrl uri = QUrl::fromUserInput(args[0].toString());
        Headers headers = args[1].value<Headers>();

        QDBusMessage reply;
        UriCheck uri_check(uri);
        const bool valid_uri = !uri_check.is_local_file() or
                (uri_check.is_local_file() and uri_check.file_exists());
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
            const auto result = d->request_authenticator->authenticate_open_uri_request(context, uri);
            if (std::get<0>(result))
            {
                reply = in.createReply();
                reply << (std::get<0>(result) ? player()->open_uri(uri, headers) : false);
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
