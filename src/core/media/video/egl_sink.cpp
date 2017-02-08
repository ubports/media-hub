/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Alfonso Sanchez-Beato <alfonso.sanchez-beato@canonical.com>
 */

#include <core/media/video/egl_sink.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sstream>
#include <thread>
#include <future>
#include <cstring>

namespace media = core::ubuntu::media;
namespace video = core::ubuntu::media::video;

namespace {
struct BufferMetadata
{
    int width;
    int height;
    int fourcc;
    int stride;
    int offset;
};

struct BufferData
{
    int fd;
    BufferMetadata meta;
};
}

struct video::EglSink::Private
{

    static bool receive_buff(int socket, BufferData *data)
    {
        struct msghdr msg{};
        struct iovec io = { .iov_base = &data->meta,
                            .iov_len = sizeof data->meta };
        char c_buffer[256];

        msg.msg_iov = &io;
        msg.msg_iovlen = 1;

        msg.msg_control = c_buffer;
        msg.msg_controllen = sizeof c_buffer;

        if (recvmsg(socket, &msg, 0) < 0) {
            printf("Failed to receive message\n");
            return false;
        }

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

        memmove(&data->fd, CMSG_DATA(cmsg), sizeof data->fd);

        printf("Extracted fd %d\n", data->fd);
        printf("width    %d\n", data->meta.width);
        printf("height   %d\n", data->meta.height);
        printf("fourcc 0x%X\n", data->meta.fourcc);
        printf("stride   %d\n", data->meta.stride);
        printf("offset   %d\n", data->meta.offset);

        return true;
    }

    static void read_sock_events(const media::Player::PlayerKey key,
                                 std::promise<BufferData>& prom_buff,
                                 core::Signal<void>& frame_available)
    {
        static const char *consumer_socket = "media-consumer";

        struct sockaddr_un local;
        int sock_fd, len;
        BufferData buff_data;

        // Bind to socket

        if ((sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
            perror("Cannot create buffer consumer socket");
            return;
        }

        std::ostringstream sock_name_ss;
        sock_name_ss << consumer_socket << key;
        local.sun_family = AF_UNIX;
        local.sun_path[0] = '\0';
        strcpy(local.sun_path + 1, sock_name_ss.str().c_str());
        len = sizeof(local.sun_family) + sock_name_ss.str().length() + 2;
        if (bind(sock_fd, (struct sockaddr *) &local, len) == -1) {
            perror("Cannot bind consumer socket");
            return;
        }

        // Wait for buffer descriptions, pass them to rendering thread
        if (!receive_buff(sock_fd, &buff_data))
            return;

        prom_buff.set_value(buff_data);

        // Now signal frame syncs
        while(true) {
            char c;

            if (recv(sock_fd, &c, sizeof c, 0) == -1) {
                perror("while waiting sync");
                return;
            }

            frame_available();
        }
    }

    Private(std::uint32_t gl_texture, const media::Player::PlayerKey key)
        : gl_texture{gl_texture},
          prom_buff{},
          fut_buff{prom_buff.get_future()},
          sock_thread{read_sock_events, key,
                  std::ref(prom_buff), std::ref(frame_available)},
          egl_image{EGL_NO_IMAGE_KHR}
    {
        // TODO check if extensions are available
        // Import functions from extensions
        _eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)
            eglGetProcAddress("eglCreateImageKHR");
        _glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)
            eglGetProcAddress("glEGLImageTargetTexture2DOES");
    }

    ~Private()
    {
        // TODO destroy thread
    }

    bool import_buffer(const BufferData *buf_data)
    {
        EGLDisplay egl_display = eglGetCurrentDisplay();
        EGLint image_attrs[] = {
            EGL_WIDTH, buf_data->meta.width,
            EGL_HEIGHT, buf_data->meta.height,
            EGL_LINUX_DRM_FOURCC_EXT, buf_data->meta.fourcc,
            EGL_DMA_BUF_PLANE0_FD_EXT, buf_data->fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, buf_data->meta.offset,
            EGL_DMA_BUF_PLANE0_PITCH_EXT, buf_data->meta.stride,
            EGL_NONE
        };

        egl_image = _eglCreateImageKHR(egl_display, EGL_NO_CONTEXT,
                                       EGL_LINUX_DMA_BUF_EXT, NULL, image_attrs);
        if (egl_image == EGL_NO_IMAGE_KHR) {
            printf("eglCreateImageKHR error 0x%x\n", eglGetError());
            return false;
        }

        // TODO Do this when swapping if we end up importing more than one buffer
        glBindTexture(GL_TEXTURE_2D, gl_texture);
        _glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image);

        printf("Image successfully imported\n");

        return true;
    }

    std::uint32_t gl_texture;
    std::promise<BufferData> prom_buff;
    std::future<BufferData> fut_buff;
    core::Signal<void> frame_available;
    std::thread sock_thread;
    EGLImageKHR egl_image;
    PFNEGLCREATEIMAGEKHRPROC _eglCreateImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC _glEGLImageTargetTexture2DOES;
};

std::function<video::Sink::Ptr(std::uint32_t)>
video::EglSink::factory_for_key(const media::Player::PlayerKey& key)
{
    return [key](std::uint32_t texture)
    {
        return video::Sink::Ptr{new video::EglSink{texture, key}};
    };
}

video::EglSink::EglSink(std::uint32_t gl_texture,
                        const media::Player::PlayerKey key)
    : d{new Private{gl_texture, key}}
{
}

video::EglSink::~EglSink()
{
}

const core::Signal<void>& video::EglSink::frame_available() const
{
    return d->frame_available;
}

bool video::EglSink::transformation_matrix(float *matrix) const
{
    // TODO: Can we get orientation on unity8 desktop somehow?
    static const float identity_4x4[] = { 1, 0, 0, 0,
                                          0, 1, 0, 0,
                                          0, 0, 1, 0,
                                          0, 0, 0, 1 };

    memcpy(matrix, identity_4x4, sizeof identity_4x4);
    return true;
}

bool video::EglSink::swap_buffers() const
{
    // First time called, import buffers
    if (d->egl_image == EGL_NO_IMAGE_KHR) {
        BufferData buf_data = d->fut_buff.get();
        if (!d->import_buffer(&buf_data))
            return false;
    }

    // We need to do nothing here, as the only buffer has already been mapped
    // Unless maybe the active texture changes?

    return true;
}
