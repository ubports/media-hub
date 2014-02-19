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

    static const std::string& pipeline_name()
    {
        static const std::string s{"playbin"};
        return s;
    }

    static void about_to_finish(GstElement*,
                                gpointer user_data)
    {        auto thiz = static_cast<Playbin*>(user_data);

        thiz->signals.about_to_finish();
    }

    Playbin()
        : pipeline(gst_element_factory_make("playbin", pipeline_name().c_str())),
          bus{gst_element_get_bus(pipeline)},
          on_new_message_connection(
              bus.on_new_message.connect(
                  std::bind(
                      &Playbin::on_new_message,
                      this,
                      std::placeholders::_1)))
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

    }

    ~Playbin()
    {
        if (pipeline)
            gst_object_unref(pipeline);
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

    void set_volume(double new_volume)
    {
        g_object_set(
                    pipeline,
                    "volume",
                    new_volume,
                    NULL);
    }

    void set_uri(const std::string& uri)
    {
        g_object_set(
                    pipeline,
                    "uri",
                    uri.c_str(),
                    NULL);
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

    bool seek(const std::chrono::nanoseconds& ns)
    {
        return gst_element_seek_simple(
                    pipeline,
                    GST_FORMAT_TIME,
                    (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                    ns.count());
    }

    GstElement* pipeline;
    gstreamer::Bus bus;
    core::Connection on_new_message_connection;
    struct
    {
        core::Signal<void> about_to_finish;
        core::Signal<Bus::Message::Detail::ErrorWarningInfo> on_error;
        core::Signal<Bus::Message::Detail::ErrorWarningInfo> on_warning;
        core::Signal<Bus::Message::Detail::ErrorWarningInfo> on_info;
        core::Signal<Bus::Message::Detail::Tag> on_tag_available;
        core::Signal<Bus::Message::Detail::StateChanged> on_state_changed;
    } signals;
};
}

#endif // GSTREAMER_PLAYBIN_H_
