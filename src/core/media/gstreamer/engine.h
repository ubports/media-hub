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
#ifndef CORE_UBUNTU_MEDIA_GSTREAMER_ENGINE_H_
#define CORE_UBUNTU_MEDIA_GSTREAMER_ENGINE_H_

#include "../engine.h"

#include <QScopedPointer>

class QUrl;

namespace gstreamer
{
class EnginePrivate;
class Engine : public core::ubuntu::media::Engine
{
public:
    Engine(const core::ubuntu::media::Player::PlayerKey key);
    ~Engine();

    const QSharedPointer<MetaDataExtractor>& metaDataExtractor() const;

    bool open_resource_for_uri(const QUrl &uri, bool do_pipeline_reset);
    bool open_resource_for_uri(const QUrl &uri, const core::ubuntu::media::Player::HeadersType& headers);
    void create_video_sink(uint32_t texture_id);

    // use_main_thread will set the pipeline's new state in the main thread context
    bool play();
    bool stop();
    bool pause();
    bool seek_to(const std::chrono::microseconds& ts);

    uint64_t position() const;
    uint64_t duration() const;

    void reset();

protected:
    void doSetAudioStreamRole(core::ubuntu::media::Player::AudioStreamRole role) override;
    void doSetLifetime(core::ubuntu::media::Player::Lifetime lifetime) override;
    void doSetVolume(double volume) override;

private:
    Q_DECLARE_PRIVATE(Engine)
    QScopedPointer<EnginePrivate> d_ptr;
};
}

#endif // CORE_UBUNTU_MEDIA_GSTREAMER_ENGINE_H_
