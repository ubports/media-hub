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
#include "../xesam.h"

#include "bus.h"
#include "meta_data_support.h"

#include "core/media/logger/logger.h"

#include <gst/gst.h>

#include <exception>
#include <future>

namespace gstreamer
{
class MetaDataExtractor : public core::ubuntu::media::Engine::MetaDataExtractor
{
public:
    static const std::map<std::string, std::string>& gstreamer_to_mpris_tag_lut()
    {
        static const std::map<std::string, std::string> lut
        {
            {GST_TAG_ALBUM, std::string{xesam::Album::name}},
            {GST_TAG_ALBUM_ARTIST, std::string{xesam::AlbumArtist::name}},
            {GST_TAG_ARTIST, std::string{xesam::Artist::name}},
            {GST_TAG_LYRICS, std::string{xesam::AsText::name}},
            {GST_TAG_COMMENT, std::string{xesam::Comment::name}},
            {GST_TAG_COMPOSER, std::string{xesam::Composer::name}},
            {GST_TAG_DATE, std::string{xesam::ContentCreated::name}},
            {GST_TAG_ALBUM_VOLUME_NUMBER, std::string{xesam::DiscNumber::name}},
            {GST_TAG_GENRE, std::string{xesam::Genre::name}},
            {GST_TAG_TITLE, std::string{xesam::Title::name}},
            {GST_TAG_TRACK_NUMBER, std::string{xesam::TrackNumber::name}},
            {GST_TAG_USER_RATING, std::string{xesam::UserRating::name}},
            // Below this line are custom entries related but not directly from
            // the MPRIS spec:
            {GST_TAG_IMAGE, std::string{tags::Image::name}},
            {GST_TAG_PREVIEW_IMAGE, std::string{tags::PreviewImage::name}}
        };

        return lut;
    }

    static void on_tag_available(
            const gstreamer::Bus::Message::Detail::Tag& tag,
            core::ubuntu::media::Track::MetaData& md)
    {
        namespace media = core::ubuntu::media;

        gst_tag_list_foreach(
                    tag.tag_list,
                    [](const GstTagList *list,
                    const gchar* tag,
                    gpointer user_data)
        {
            (void) list;

            auto md = static_cast<media::Track::MetaData*>(user_data);
            std::stringstream ss;

            switch (gst_tag_get_type(tag))
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

            const bool has_tag_from_lut = (gstreamer_to_mpris_tag_lut().count(tag) > 0);
            const std::string tag_name{(has_tag_from_lut) ?
                    gstreamer_to_mpris_tag_lut().at(tag) : tag};


            // Specific handling for the following tag types:
            if (tag_name == tags::PreviewImage::name)
                ss << "true";
            if (tag_name == tags::Image::name)
                ss << "true";

            (*md).set((has_tag_from_lut ?
                            gstreamer_to_mpris_tag_lut().at(tag) : tag), ss.str());
        },
        &md);

        // embedded album art
        if(MetaDataSupport::checkForImageData(GST_TAG_IMAGE, 0, tag.tag_list, md)) { 
            md.mark_has_album_art_image();
        } else if(!md.has_embedded_album_art_image() 
                  && MetaDataSupport::checkForImageData(GST_TAG_PREVIEW_IMAGE, 0, tag.tag_list, md)) { 
            md.mark_has_album_art_preview_image();
        }
    }

    MetaDataExtractor()
        : pipe(gst_pipeline_new("meta_data_extractor_pipeline")),
          decoder(gst_element_factory_make ("uridecodebin", NULL)),
          bus(gst_element_get_bus(pipe))
    {
        gst_bin_add(GST_BIN(pipe), decoder);

        auto sink = gst_element_factory_make ("fakesink", NULL);
        gst_bin_add (GST_BIN (pipe), sink);

        g_signal_connect (decoder, "pad-added", G_CALLBACK (on_new_pad), sink);
    }

    ~MetaDataExtractor()
    {
        set_state_and_wait(GST_STATE_NULL);
        gst_object_unref(pipe);
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

        auto ret = gst_element_set_state(pipe, new_state);

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
                    pipe,
                    &current,
                    &pending,
                    state_change_timeout.count());
                break;
        }

        return result;
    }

    core::ubuntu::media::Track::MetaData meta_data_for_track_with_uri(const core::ubuntu::media::Track::UriType& uri)
    {
        if (!gst_uri_is_valid(uri.c_str()))
            throw std::runtime_error("Invalid uri");

        core::ubuntu::media::Track::MetaData meta_data;
        std::promise<core::ubuntu::media::Track::MetaData> promise;
        std::future<core::ubuntu::media::Track::MetaData> future{promise.get_future()};

        core::ScopedConnection on_new_message_connection
        {
            bus.on_new_message.connect(
                    [&](const gstreamer::Bus::Message& msg)
                    {
                        //std::cout << __PRETTY_FUNCTION__ << gst_message_type_get_name(msg.type) << std::endl;
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

        if (std::future_status::ready != future.wait_for(std::chrono::seconds(4)))
        {
            set_state_and_wait(GST_STATE_NULL);
            throw std::runtime_error("Problem extracting meta data for track");
        } else
        {
            set_state_and_wait(GST_STATE_NULL);
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

    GstElement* pipe;
    GstElement* decoder;
    Bus bus;
};
}

#endif // GSTREAMER_META_DATA_EXTRACTOR_H_
