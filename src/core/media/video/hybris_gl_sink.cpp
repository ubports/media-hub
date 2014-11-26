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

#if defined(MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
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

    Private(std::uint32_t gl_texture, const media::Player::PlayerKey& player_key)
        : gl_texture{gl_texture},
          player_key{player_key},
          decoding_service_session{decoding_service_create_session(player_key)},
          graphics_buffer_consumer{decoding_service_get_igraphicbufferconsumer()},
          gl_texture_consumer{gl_consumer_create_by_id_with_igbc(gl_texture, graphics_buffer_consumer)}
    {
        if (not decoding_service_session) throw std::runtime_error
        {
            "video::HybrisGlSink: Could not connect to the remote decoding service and establish a session."
        };

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
    media::Player::PlayerKey player_key;
    core::Signal<void> frame_available;
    DSSessionWrapperHybris decoding_service_session;
    IGBCWrapperHybris graphics_buffer_consumer;
    GLConsumerWrapperHybris gl_texture_consumer;
};

video::HybrisGlSink::HybrisGlSink(std::uint32_t gl_texture, const media::Player::PlayerKey& key) : d{new Private{gl_texture, key}}
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
    gl_consumer_get_transformation_matrix(d->gl_texture_consumer, matrix.to_float());
    return true;
}

bool video::HybrisGlSink::swap_buffers() const
{
    // TODO: The underlying API really should tell us if everything is ok.
    gl_consumer_update_texture(d->gl_texture_consumer);
    return true;
}
#endif // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER
