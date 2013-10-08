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

#ifndef GSTREAMER_META_DATA_EXTRACTOR_H_
#define GSTREAMER_META_DATA_EXTRACTOR_H_

#include "../engine.h"

#include "bus.h"

#include <gst/gst.h>

#include <exception>
#include <future>

namespace gstreamer
{
class MetaDataExtractor : public com::ubuntu::music::Engine::MetaDataExtractor
{
public:
    MetaDataExtractor()
        : pipe(gst_pipeline_new("meta_data_extractor_pipeline")),
          decoder(gst_element_factory_make ("uridecodebin", NULL)),
          bus(GST_ELEMENT_BUS(pipe))
    {
        gst_bin_add(GST_BIN(pipe), decoder);

        auto sink = gst_element_factory_make ("fakesink", NULL);
        gst_bin_add (GST_BIN (pipe), sink);

        g_signal_connect (decoder, "pad-added", G_CALLBACK (on_new_pad), sink);
    }

    ~MetaDataExtractor()
    {
        gst_element_set_state(pipe, GST_STATE_NULL);
        // gst_object_unref(pipe);
    }

    com::ubuntu::music::Track::MetaData meta_data_for_track_with_uri(const com::ubuntu::music::Track::UriType& uri)
    {
        if (!gst_uri_is_valid(uri.c_str()))
            throw std::runtime_error("Invalid uri");

        com::ubuntu::music::Track::MetaData meta_data;
        std::promise<com::ubuntu::music::Track::MetaData> promise;
        std::future<com::ubuntu::music::Track::MetaData> future{promise.get_future()};

        com::ubuntu::music::ScopedConnection on_new_message_connection
        {
            bus.on_new_message.connect(
                    [&](const gstreamer::Bus::Message& msg)
                    {
                        std::cout << __PRETTY_FUNCTION__ << gst_message_type_get_name(msg.type) << std::endl;
                        if (msg.type == GST_MESSAGE_TAG)
                        {
                            MetaDataExtractor::on_tag_available(msg.detail.tag, meta_data);
                        } else if (msg.type == GST_MESSAGE_ASYNC_DONE)
                        {
                            promise.set_value(meta_data);
                        }
                    })
        };

        g_object_set(decoder, "uri", uri.c_str(), NULL);
        gst_element_set_state(pipe, GST_STATE_PAUSED);

        if (std::future_status::ready != future.wait_for(std::chrono::seconds(2)))
        {
            gst_element_set_state(pipe, GST_STATE_NULL);
            throw std::runtime_error("Problem extracting meta data for track");
        } else
        {
            gst_element_set_state(pipe, GST_STATE_NULL);
        }

        return future.get();
    }

private:
    static void on_new_pad(GstElement*, GstPad* pad, GstElement* fakesink)
    {
        GstPad *sinkpad;

        sinkpad = gst_element_get_static_pad (fakesink, "sink");

        if (!gst_pad_is_linked (sinkpad)) {
            if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)
                g_error ("Failed to link pads!");
        }

        gst_object_unref (sinkpad);
    }

    static void on_tag_available(
            const gstreamer::Bus::Message::Detail::Tag& tag,
            com::ubuntu::music::Track::MetaData& md)
    {
        namespace music = com::ubuntu::music;

        gst_tag_list_foreach(
                    tag.tag_list,
                    [](const GstTagList *list,
                    const gchar* tag,
                    gpointer user_data)
        {
            (void) list;

            static const std::map<std::string, std::string> gstreamer_to_mpris_tag_lut =
            {
                {GST_TAG_ALBUM, music::Engine::Xesam::album()},
                {GST_TAG_ALBUM_ARTIST, music::Engine::Xesam::album_artist()},
                {GST_TAG_ARTIST, music::Engine::Xesam::artist()},
                {GST_TAG_LYRICS, music::Engine::Xesam::as_text()},
                {GST_TAG_COMMENT, music::Engine::Xesam::comment()},
                {GST_TAG_COMPOSER, music::Engine::Xesam::composer()},
                {GST_TAG_DATE, music::Engine::Xesam::content_created()},
                {GST_TAG_ALBUM_VOLUME_NUMBER, music::Engine::Xesam::disc_number()},
                {GST_TAG_GENRE, music::Engine::Xesam::genre()},
                {GST_TAG_TITLE, music::Engine::Xesam::title()},
                {GST_TAG_TRACK_NUMBER, music::Engine::Xesam::track_number()},
                {GST_TAG_USER_RATING, music::Engine::Xesam::user_rating()}
            };

            auto md = static_cast<music::Track::MetaData*>(user_data);
            std::stringstream ss;

            switch(gst_tag_get_type(tag))
            {
            case G_TYPE_BOOLEAN:
            {
                gboolean value;
                if (gst_tag_list_get_boolean(list, tag, &value))
                    ss << value;
                break;
            }
            case G_TYPE_INT:
            {
                gint value;
                if (gst_tag_list_get_int(list, tag, &value))
                    ss << value;
                break;
            }
            case G_TYPE_UINT:
            {
                guint value;
                if (gst_tag_list_get_uint(list, tag, &value))
                    ss << value;
                break;
            }
            case G_TYPE_INT64:
            {
                gint64 value;
                if (gst_tag_list_get_int64(list, tag, &value))
                    ss << value;
                break;
            }
            case G_TYPE_UINT64:
            {
                guint64 value;
                if (gst_tag_list_get_uint64(list, tag, &value))
                    ss << value;
                break;
            }
            case G_TYPE_FLOAT:
            {
                gfloat value;
                if (gst_tag_list_get_float(list, tag, &value))
                    ss << value;
                break;
            }
            case G_TYPE_DOUBLE:
            {
                double value;
                if (gst_tag_list_get_double(list, tag, &value))
                    ss << value;
                break;
            }
            case G_TYPE_STRING:
            {
                gchar* value;
                if (gst_tag_list_get_string(list, tag, &value))
                {
                    ss << value;
                    g_free(value);
                }
                break;
            }
            default:
                break;
            }

            (*md).set(
                        (gstreamer_to_mpris_tag_lut.count(tag) > 0 ? gstreamer_to_mpris_tag_lut.at(tag) : tag),
                        ss.str());
        },
        &md);
    }

    GstElement* pipe;
    GstElement* decoder;
    Bus bus;
};
}

#endif // GSTREAMER_META_DATA_EXTRACTOR_H_
