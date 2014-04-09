/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef GSTREAMER_PLAYBIN_H_
#define GSTREAMER_PLAYBIN_H_

#include "bus.h"

#include <hybris/media/media_codec_layer.h>
#include <hybris/media/surface_texture_client_hybris.h>

#include <gio/gio.h>
#include <gst/gst.h>

#include <chrono>
#include <string>

namespace gstreamer
{
struct Playbin
{
    enum PlayFlags
    {
        GST_PLAY_FLAG_VIDEO = (1 << 0),
        GST_PLAY_FLAG_AUDIO = (1 << 1),
        GST_PLAY_FLAG_TEXT = (1 << 2)
    };

    enum MediaFileType
    {
        MEDIA_FILE_TYPE_NONE,
        MEDIA_FILE_TYPE_AUDIO,
        MEDIA_FILE_TYPE_VIDEO
    };

    static const std::string& pipeline_name()
    {
        static const std::string s{"playbin"};
        return s;
    }

    static void about_to_finish(GstElement*,
                                gpointer user_data)
    {
        auto thiz = static_cast<Playbin*>(user_data);
        thiz->signals.about_to_finish();
    }

    Playbin()
        : pipeline(gst_element_factory_make("playbin", pipeline_name().c_str())),
          bus{gst_element_get_bus(pipeline)},
          file_type(MEDIA_FILE_TYPE_NONE),
          on_new_message_connection(
              bus.on_new_message.connect(
                  std::bind(
                      &Playbin::on_new_message,
                      this,
                      std::placeholders::_1))),
          is_seeking(false)
    {
        if (!pipeline)
            throw std::runtime_error("Could not create pipeline for playbin.");

        // Add audio and/or video sink elements depending on environment variables
        // being set or not set
        setup_pipeline_for_audio_video();

        g_signal_connect(
                    pipeline,
                    "about-to-finish",
                    G_CALLBACK(about_to_finish),
                    this
                    );

        // When a client of media-hub dies, call on_client_died
        decoding_service_set_client_death_cb(&Playbin::on_client_died_cb, static_cast<void*>(this));
    }

    ~Playbin()
    {
        if (pipeline)
            gst_object_unref(pipeline);
    }

    static void on_client_died_cb(void *context)
    {
        if (context)
        {
            Playbin *pb = static_cast<Playbin*>(context);
            pb->on_client_died();
        }
    }

    void on_client_died()
    {
        std::cout << "Client died, resetting pipeline" << std::endl;
        // When the client dies, tear down the current pipeline and get it
        // in a state that is ready for the next client that connects to the
        // service
        reset_pipeline();
    }

    void reset_pipeline()
    {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        auto ret = gst_element_set_state(pipeline, GST_STATE_NULL);
        switch(ret)
        {
        case GST_STATE_CHANGE_FAILURE:
            std::cout << "Failed to reset the pipeline state. Client reconnect may not function properly." << std::endl;
            break;
        case GST_STATE_CHANGE_NO_PREROLL:
        case GST_STATE_CHANGE_SUCCESS:
        case GST_STATE_CHANGE_ASYNC:
            break;
        default:
            std::cout << "Failed to reset the pipeline state. Client reconnect may not function properly." << std::endl;
        }
        file_type = MEDIA_FILE_TYPE_NONE;
    }

    void on_new_message(const Bus::Message& message)
    {
        switch(message.type)
        {
        case GST_MESSAGE_ERROR:
            signals.on_error(message.detail.error_warning_info);
            break;
        case GST_MESSAGE_WARNING:
            signals.on_warning(message.detail.error_warning_info);
            break;
        case GST_MESSAGE_INFO:
            signals.on_info(message.detail.error_warning_info);
            break;
        case GST_MESSAGE_TAG:
            signals.on_tag_available(message.detail.tag);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            signals.on_state_changed(message.detail.state_changed);
            break;
        case GST_MESSAGE_ASYNC_DONE:
            if (is_seeking)
            {
                // FIXME: Pass the actual playback time position to the signal call
                signals.on_seeked_to(0);
                is_seeking = false;
            }
            break;
        case GST_MESSAGE_EOS:
            signals.on_end_of_stream();
        default:
            break;
        }
    }

    gstreamer::Bus& message_bus()
    {
        return bus;
    }

    void setup_pipeline_for_audio_video()
    {
        gint flags;
        g_object_get (pipeline, "flags", &flags, nullptr);
        flags |= GST_PLAY_FLAG_AUDIO;
        flags |= GST_PLAY_FLAG_VIDEO;
        flags &= ~GST_PLAY_FLAG_TEXT;
        g_object_set (pipeline, "flags", flags, nullptr);

        if (::getenv("CORE_UBUNTU_MEDIA_SERVICE_AUDIO_SINK_NAME") != nullptr)
        {
            auto audio_sink = gst_element_factory_make (
                        ::getenv("CORE_UBUNTU_MEDIA_SERVICE_AUDIO_SINK_NAME"),
                        "audio-sink");

            std::cout << "audio_sink: " << ::getenv("CORE_UBUNTU_MEDIA_SERVICE_AUDIO_SINK_NAME") << std::endl;

            g_object_set (
                        pipeline,
                        "audio-sink",
                        audio_sink,
                        NULL);
        }

        if (::getenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME") != nullptr)
        {
            auto video_sink = gst_element_factory_make (
                    ::getenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME"),
                    "video-sink");

            std::cout << "video_sink: " << ::getenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME") << std::endl;

            g_object_set (
                    pipeline,
                    "video-sink",
                    video_sink,
                    NULL);
        }
    }

    void create_video_sink(uint32_t texture_id)
    {
        std::cout << "Creating video sink for texture_id: " << texture_id << std::endl;

        if (::getenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME") != nullptr)
        {
            GstElement *video_sink = NULL;
            g_object_get (pipeline, "video_sink", &video_sink, NULL);

            // Get the service-side BufferQueue (IGraphicBufferProducer) and associate it with
            // the SurfaceTextureClientHybris instance
            IGBPWrapperHybris igbp = decoding_service_get_igraphicbufferproducer();
            SurfaceTextureClientHybris stc = surface_texture_client_create_by_igbp(igbp);
            // Because mirsink is being loaded, we are definitely doing * hardware rendering.
            surface_texture_client_set_hardware_rendering (stc, TRUE);
            g_object_set (G_OBJECT (video_sink), "surface", static_cast<gpointer>(stc), static_cast<char*>(NULL));
        }
    }

    void set_volume(double new_volume)
    {
        g_object_set(pipeline, "volume", new_volume, NULL);
    }

    uint64_t position() const
    {
        int64_t pos = 0;
        gst_element_query_position (pipeline, GST_FORMAT_TIME, &pos);

        // FIXME: this should be int64_t, but dbus-cpp doesn't seem to handle it correctly
        return static_cast<uint64_t>(pos);
    }

    uint64_t duration() const
    {
        int64_t dur = 0;
        gst_element_query_duration (pipeline, GST_FORMAT_TIME, &dur);

        // FIXME: this should be int64_t, but dbus-cpp doesn't seem to handle it correctly
        return static_cast<uint64_t>(dur);
    }

    void set_uri(const std::string& uri)
    {
        g_object_set(pipeline, "uri", uri.c_str(), NULL);
        if (is_video_file(uri))
            file_type = MEDIA_FILE_TYPE_VIDEO;
        else if (is_audio_file(uri))
            file_type = MEDIA_FILE_TYPE_AUDIO;
    }

    std::string uri() const
    {
        gchar* data = nullptr;
        g_object_get(pipeline, "current-uri", &data, nullptr);

        std::string result((data == nullptr ? "" : data));
        g_free(data);

        return result;
    }

    bool set_state_and_wait(GstState new_state)
    {
        static const std::chrono::nanoseconds state_change_timeout
        {
            // We choose a quite high value here as tests are run under valgrind
            // and gstreamer pipeline setup/state changes take longer in that scenario.
            // The value does not negatively impact runtime performance.
            std::chrono::milliseconds{5000}
        };

        auto ret = gst_element_set_state(pipeline, new_state);
        bool result = false; GstState current, pending;
        switch(ret)
        {
        case GST_STATE_CHANGE_FAILURE:
            result = false; break;
        case GST_STATE_CHANGE_NO_PREROLL:
        case GST_STATE_CHANGE_SUCCESS:
            result = true; break;
        case GST_STATE_CHANGE_ASYNC:
            result = GST_STATE_CHANGE_SUCCESS == gst_element_get_state(
                        pipeline,
                        &current,
                        &pending,
                        state_change_timeout.count());
            break;
        }

        return result;
    }

    bool seek(const std::chrono::microseconds& ms)
    {
        is_seeking = true;
        return gst_element_seek_simple(
                    pipeline,
                    GST_FORMAT_TIME,
                    (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT |
                                    GST_SEEK_FLAG_SNAP_BEFORE),
                    ms.count() * 1000);
    }

    std::string get_file_content_type(const std::string& uri) const
    {
        if (uri.empty())
            return std::string();

        std::string filename(uri);
        size_t pos = uri.find("file://") + 7;
        if (pos)
            filename = uri.substr(pos, std::string::npos);

        std::cout << "filename: " << filename << std::endl;

        GError *error = nullptr;
        std::unique_ptr<GFile, void(*)(void *)> file(
                g_file_new_for_path(filename.c_str()), g_object_unref);
        std::unique_ptr<GFileInfo, void(*)(void *)> info(
                g_file_query_info(
                    file.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE ","
                    G_FILE_ATTRIBUTE_ETAG_VALUE, G_FILE_QUERY_INFO_NONE,
                    /* cancellable */ NULL, &error),
                g_object_unref);
        if (!info)
        {
            std::string error_str(error->message);
            g_error_free(error);

            std::cout << "Failed to query the URI for the presence of video content: "
                << error_str << std::endl;
            return std::string();
        }

        std::string content_type(g_file_info_get_attribute_string(
                    info.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE));

        return content_type;
    }

    bool is_audio_file(const std::string& uri) const
    {
        if (uri.empty())
            return false;

        if (get_file_content_type(uri).find("audio/") == 0)
        {
            std::cout << "Found audio content" << std::endl;
            return true;
        }

        return false;
    }

    bool is_video_file(const std::string& uri) const
    {
        if (uri.empty())
            return false;

        if (get_file_content_type(uri).find("video/") == 0)
        {
            std::cout << "Found video content" << std::endl;
            return true;
        }

        return false;
    }

    MediaFileType media_file_type() const
    {
        return file_type;
    }

    GstElement* pipeline;
    gstreamer::Bus bus;
    MediaFileType file_type;
    SurfaceTextureClientHybris stc_hybris;
    core::Connection on_new_message_connection;
    bool is_seeking;
    struct
    {
        core::Signal<void> about_to_finish;
        core::Signal<Bus::Message::Detail::ErrorWarningInfo> on_error;
        core::Signal<Bus::Message::Detail::ErrorWarningInfo> on_warning;
        core::Signal<Bus::Message::Detail::ErrorWarningInfo> on_info;
        core::Signal<Bus::Message::Detail::Tag> on_tag_available;
        core::Signal<Bus::Message::Detail::StateChanged> on_state_changed;
        core::Signal<uint64_t> on_seeked_to;
        core::Signal<void> on_end_of_stream;
    } signals;
};
}

#endif // GSTREAMER_PLAYBIN_H_
