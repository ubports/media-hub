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

#ifndef GSTREAMER_BUS_H_
#define GSTREAMER_BUS_H_

#include <com/ubuntu/music/signal.h>

#include <gst/gst.h>

#include <boost/flyweight.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <tuple>

namespace gstreamer
{
class Bus
{
public:
    struct Message
    {
        ~Message()
        {
            if (cleanup)
                cleanup();

            gst_message_unref(message);
        }

        Message(GstMessage* msg)
            : message(msg),
              type(GST_MESSAGE_TYPE(msg)),
              source(GST_MESSAGE_SRC_NAME(msg)),
              sequence_number(gst_message_get_seqnum(msg))
        {
            switch(type)
            {
            case GST_MESSAGE_UNKNOWN:
                throw std::runtime_error("Cannot construct message for type unknown");
                break;
            case GST_MESSAGE_EOS:
                break;
            case GST_MESSAGE_ERROR:
            {
                gst_message_parse_error(
                            msg,
                            &detail.error_warning_info.error,
                            &detail.error_warning_info.debug);
                cleanup = [this]()
                {
                    g_error_free(detail.error_warning_info.error);
                    g_free(detail.error_warning_info.debug);
                };
                break;
            }
            case GST_MESSAGE_WARNING:
                gst_message_parse_warning(
                            msg,
                            &detail.error_warning_info.error,
                            &detail.error_warning_info.debug);
                cleanup = [this]()
                {
                    g_error_free(detail.error_warning_info.error);
                    g_free(detail.error_warning_info.debug);
                };
                break;
                break;
            case GST_MESSAGE_INFO:
                gst_message_parse_info(
                            msg,
                            &detail.error_warning_info.error,
                            &detail.error_warning_info.debug);
                cleanup = [this]()
                {
                    g_error_free(detail.error_warning_info.error);
                    g_free(detail.error_warning_info.debug);
                };
                break;
                break;
            case GST_MESSAGE_TAG:
                gst_message_parse_tag(
                            msg,
                            &detail.tag.tag_list);
                cleanup = [this]()
                {
                    gst_tag_list_free(detail.tag.tag_list);
                };
                break;
            case GST_MESSAGE_BUFFERING:
                gst_message_parse_buffering(
                            msg,
                            &detail.buffering.percent);
                break;
            case GST_MESSAGE_STATE_CHANGED:
                gst_message_parse_state_changed(
                            msg,
                            &detail.state_changed.old_state,
                            &detail.state_changed.new_state,
                            &detail.state_changed.pending_state);
                break;
            case GST_MESSAGE_STATE_DIRTY:
                break;
            case GST_MESSAGE_STEP_DONE:
                gst_message_parse_step_done(
                            msg,
                            &detail.step_done.format,
                            &detail.step_done.amount,
                            &detail.step_done.rate,
                            &detail.step_done.flush,
                            &detail.step_done.intermediate,
                            &detail.step_done.duration,
                            &detail.step_done.eos
                            );
                break;
            case GST_MESSAGE_CLOCK_PROVIDE:
                gst_message_parse_clock_provide(
                            msg,
                            &detail.clock_provide.clock,
                            &detail.clock_provide.ready);
                break;
            case GST_MESSAGE_CLOCK_LOST:
                gst_message_parse_clock_lost(
                            msg,
                            &detail.clock_lost.clock);
                break;
            case GST_MESSAGE_NEW_CLOCK:
                gst_message_parse_new_clock(
                            msg,
                            &detail.clock_new.clock);
                break;
            case GST_MESSAGE_STRUCTURE_CHANGE:
                break;
            case GST_MESSAGE_STREAM_STATUS:
                break;
            case GST_MESSAGE_APPLICATION:
                break;
            case GST_MESSAGE_ELEMENT:
                break;
            case GST_MESSAGE_SEGMENT_START:
                gst_message_parse_segment_start(
                            msg,
                            &detail.segment_start.format,
                            &detail.segment_start.position);
                break;
            case GST_MESSAGE_SEGMENT_DONE:
                gst_message_parse_segment_done(
                            msg,
                            &detail.segment_done.format,
                            &detail.segment_done.position);
                break;
            case GST_MESSAGE_DURATION_CHANGED:
                break;
            case GST_MESSAGE_LATENCY:
                break;
            case GST_MESSAGE_ASYNC_START:
                break;
            case GST_MESSAGE_ASYNC_DONE:
                gst_message_parse_async_done(
                            msg,
                            &detail.async_done.running_time);
                break;
            case GST_MESSAGE_REQUEST_STATE:
                break;
            case GST_MESSAGE_STEP_START:
                gst_message_parse_step_start(
                            msg,
                            &detail.step_start.active,
                            &detail.step_start.format,
                            &detail.step_start.amount,
                            &detail.step_start.rate,
                            &detail.step_start.flush,
                            &detail.step_start.intermediate);
                break;
            case GST_MESSAGE_QOS:
                gst_message_parse_qos(
                            msg,
                            &detail.qos.live,
                            &detail.qos.running_time,
                            &detail.qos.stream_time,
                            &detail.qos.timestamp,
                            &detail.qos.duration);
                break;
            case GST_MESSAGE_PROGRESS:
                break;
            case GST_MESSAGE_TOC:
                break;
            case GST_MESSAGE_RESET_TIME:
                break;
            case GST_MESSAGE_STREAM_START:
                break;
            case GST_MESSAGE_NEED_CONTEXT:
                break;
            case GST_MESSAGE_HAVE_CONTEXT:
                break;
            case GST_MESSAGE_ANY:
                break;
            }
        }

        GstMessage* message;
        GstMessageType type;
        boost::flyweight<std::string> source;
        uint32_t sequence_number;

        union Detail
        {
            struct ErrorWarningInfo
            {
                GError* error;
                gchar* debug;
            } error_warning_info;
            struct Tag
            {
                GstTagList* tag_list;
            } tag;
            struct
            {
                gint percent;
            } buffering;
            struct
            {
                GstBufferingMode buffering_mode;
                gint avg_in;
                gint avg_out;
                gint64 buffering_left;
            } buffering_stats;
            struct StateChanged
            {
                GstState old_state;
                GstState new_state;
                GstState pending_state;
            } state_changed;
            struct
            {
                gboolean active;
                GstFormat format;
                guint64 amount;
                gdouble rate;
                gboolean flush;
                gboolean intermediate;
            } step_start;
            struct
            {
                GstFormat format;
                guint64 amount;
                gdouble rate;
                gboolean flush;
                gboolean intermediate;
                guint64 duration;
                gboolean eos;
            } step_done;
            struct
            {
                GstClock* clock;
                gboolean ready;
            } clock_provide;
            struct
            {
                GstClock* clock;
            } clock_lost;
            struct
            {
                GstClock* clock;
            } clock_new;
            struct
            {
                GstFormat format;
                gint64 position;
            } segment_start;
            struct
            {
                GstFormat format;
                gint64 position;
            } segment_done;
            struct
            {
                GstClockTime running_time;
            } async_done;
            struct
            {
                gboolean live;
                guint64 running_time;
                guint64 stream_time;
                guint64 timestamp;
                guint64 duration;
            } qos;
        } detail;
        std::function<void()> cleanup;
    };

    static GstBusSyncReply sync_handler(
            GstBus* bus,
            GstMessage* msg,
            gpointer data)
    {
        (void) bus;

        auto thiz = static_cast<Bus*>(data);
        Message message(msg);
        thiz->on_new_message(message);

        return GST_BUS_DROP;
    }

    Bus(GstBus* bus) : bus(bus)
    {
        if (!bus)
            throw std::runtime_error("Cannot create Bus instance if underlying instance is NULL.");

        gst_bus_set_sync_handler(
                    bus,
                    Bus::sync_handler,
                    this,
                    nullptr);
    }

    ~Bus()
    {
        gst_object_unref(bus);
    }

    GstBus* bus;
    com::ubuntu::music::Signal<Message> on_new_message;
};
}

#endif // GSTREAMER_BUS_H_
