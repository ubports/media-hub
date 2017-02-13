/*
 * Copyright © 2014 Canonical Ltd.
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

#include <core/media/video/hybris_gl_sink.h>

// Hybris
#include <hybris/media/media_codec_layer.h>
#include <hybris/media/surface_texture_client_hybris.h>

namespace media = core::ubuntu::media;
namespace video = core::ubuntu::media::video;

struct video::HybrisGlSink::Private
{
    static void on_frame_available_callback(GLConsumerWrapperHybris, void* context)
    {
        if (not context)
            return;

        auto thiz = static_cast<Private*>(context);

        thiz->frame_available();
    }

    Private(std::uint32_t gl_texture)
        : gl_texture{gl_texture},
          graphics_buffer_consumer{decoding_service_get_igraphicbufferconsumer()},
          gl_texture_consumer{gl_consumer_create_by_id_with_igbc(gl_texture, graphics_buffer_consumer)}
    {
        if (not graphics_buffer_consumer) throw std::runtime_error
        {
            "video::HybrisGlSink: Could not connect to remote buffer queue."
        };

        if (not gl_texture_consumer) throw std::runtime_error
        {
            "video::HybrisGlSink: Could not associate local texture id with remote buffer streak."
        };

        gl_consumer_set_frame_available_cb(gl_texture_consumer, Private::on_frame_available_callback, this);
    }

    ~Private()
    {
        gl_consumer_set_frame_available_cb(gl_texture_consumer, Private::on_frame_available_callback, nullptr);
    }

    std::uint32_t gl_texture;
    core::Signal<void> frame_available;
    IGBCWrapperHybris graphics_buffer_consumer;
    GLConsumerWrapperHybris gl_texture_consumer;
};

std::function<video::Sink::Ptr(std::uint32_t)> video::HybrisGlSink::factory_for_key(const media::Player::PlayerKey& key)
{
    // It's okay-ish to use static map here. Point being that we currently have no way
    // of terminating the session with the decoding service anyway.
    static std::map<media::Player::PlayerKey, DSSessionWrapperHybris> lut;
    static std::mutex lut_guard;

    // Scoping access to the lut to ensure that the lock is kept for as short as possible.
    {
        std::lock_guard<std::mutex> lg{lut_guard};
        if (lut.count(key) == 0)
            lut[key] = decoding_service_create_session(key);
    }

    return [](std::uint32_t texture)
    {
        return video::Sink::Ptr{new video::HybrisGlSink{texture}};
    };
}

video::HybrisGlSink::HybrisGlSink(std::uint32_t gl_texture) : d{new Private{gl_texture}}
{
}

video::HybrisGlSink::~HybrisGlSink()
{
}

const core::Signal<void>& video::HybrisGlSink::frame_available() const
{
    return d->frame_available;
}

bool video::HybrisGlSink::transformation_matrix(float* matrix) const
{
    // TODO: The underlying API really should tell us if everything is ok.
    gl_consumer_get_transformation_matrix(d->gl_texture_consumer, matrix);
    return true;
}

bool video::HybrisGlSink::swap_buffers() const
{
    // TODO: The underlying API really should tell us if everything is ok.
    gl_consumer_update_texture(d->gl_texture_consumer);
    return true;
}
