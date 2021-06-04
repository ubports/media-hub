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

#include "engine.h"

#include <QSize>

#include <exception>
#include <stdexcept>

namespace media = core::ubuntu::media;

using namespace media;

namespace core {
namespace ubuntu {
namespace media {

class EnginePrivate
{
    Q_DECLARE_PUBLIC(Engine)

public:
    EnginePrivate(Engine *q);

private:
    QSharedPointer<Engine::MetaDataExtractor> m_metadataExtractor;
    Engine::State m_state;
    bool m_isVideoSource;
    bool m_isAudioSource;
    Player::AudioStreamRole m_audioStreamRole;
    Player::Lifetime m_lifetime;
    Player::Orientation m_orientation;
    double m_volume;
    QPair<QUrl,Track::MetaData> m_trackMetadata;
    Player::PlaybackStatus m_playbackStatus;
    QSize m_videoDimension;
    Engine *q_ptr;
};

}}} // namespace

EnginePrivate::EnginePrivate(Engine *q):
    m_state(Engine::State::no_media),
    m_isVideoSource(false),
    m_isAudioSource(false),
    m_audioStreamRole(Player::AudioStreamRole::multimedia),
    m_lifetime(Player::Lifetime::normal),
    m_orientation(Player::Orientation::rotate0),
    m_volume(1.0),
    q_ptr(q)
{
}

Engine::Engine(QObject *parent):
    QObject(parent),
    d_ptr(new EnginePrivate(this))
{
}

Engine::~Engine() = default;

void Engine::setMetadataExtractor(const QSharedPointer<MetaDataExtractor> &extractor)
{
    Q_D(Engine);
    d->m_metadataExtractor = extractor;
}

const QSharedPointer<Engine::MetaDataExtractor> &Engine::metadataExtractor() const
{
    Q_D(const Engine);
    return d->m_metadataExtractor;
}

void Engine::setState(State state)
{
    Q_D(Engine);
    if (state == d->m_state) return;
    d->m_state = state;
    Q_EMIT stateChanged();
}

Engine::State Engine::state() const
{
    Q_D(const Engine);
    return d->m_state;
}

void Engine::setIsVideoSource(bool value)
{
    Q_D(Engine);
    if (value == d->m_isVideoSource) return;
    d->m_isVideoSource = value;
    Q_EMIT isVideoSourceChanged();
}

bool Engine::isVideoSource() const
{
    Q_D(const Engine);
    return d->m_isVideoSource;
}

void Engine::setIsAudioSource(bool value)
{
    Q_D(Engine);
    if (value == d->m_isAudioSource) return;
    d->m_isAudioSource = value;
    Q_EMIT isAudioSourceChanged();
}

bool Engine::isAudioSource() const
{
    Q_D(const Engine);
    return d->m_isAudioSource;
}

void Engine::setOrientation(Player::Orientation o)
{
    Q_D(Engine);
    if (o == d->m_orientation) return;
    d->m_orientation = o;
    Q_EMIT orientationChanged();
}

Player::Orientation Engine::orientation() const
{
    Q_D(const Engine);
    return d->m_orientation;
}

void Engine::setAudioStreamRole(Player::AudioStreamRole role)
{
    Q_D(Engine);
    if (role == d->m_audioStreamRole) return;
    d->m_audioStreamRole = role;
    doSetAudioStreamRole(role);
}

Player::AudioStreamRole Engine::audioStreamRole() const
{
    Q_D(const Engine);
    return d->m_audioStreamRole;
}

void Engine::setLifetime(Player::Lifetime lifetime)
{
    Q_D(Engine);
    if (lifetime == d->m_lifetime) return;
    d->m_lifetime = lifetime;
    doSetLifetime(lifetime);
}

Player::Lifetime Engine::lifetime() const
{
    Q_D(const Engine);
    return d->m_lifetime;
}

void Engine::setTrackMetadata(const QPair<QUrl,Track::MetaData> &metadata)
{
    Q_D(Engine);
    d->m_trackMetadata = metadata;
    Q_EMIT trackMetadataChanged();
}

QPair<QUrl,Track::MetaData> Engine::trackMetadata() const
{
    Q_D(const Engine);
    return d->m_trackMetadata;
}

void Engine::setPlaybackStatus(Player::PlaybackStatus status)
{
    Q_D(Engine);
    if (status == d->m_playbackStatus) return;
    d->m_playbackStatus = status;
    Q_EMIT playbackStatusChanged();
}

Player::PlaybackStatus Engine::playbackStatus() const
{
    Q_D(const Engine);
    return d->m_playbackStatus;
}

void Engine::setVideoDimension(const QSize &size)
{
    Q_D(Engine);
    if (size == d->m_videoDimension) return;
    d->m_videoDimension = size;
    Q_EMIT videoDimensionChanged();
}

QSize Engine::videoDimension() const
{
    Q_D(const Engine);
    return d->m_videoDimension;
}

void Engine::setVolume(double volume)
{
    Q_D(Engine);
    d->m_volume = qMax(qMin(volume, 1.0), 0.0);
    doSetVolume(d->m_volume);
}

double Engine::volume() const
{
    Q_D(const Engine);
    return d->m_volume;
}
