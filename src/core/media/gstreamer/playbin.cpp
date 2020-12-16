/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 *              Alfonso Sanchez-Beato <alfonso.sanchez-beato@canonical.com>
 */

#include <core/media/gstreamer/playbin.h>
#include <core/media/gstreamer/engine.h>
#include <core/media/video/socket_types.h>

#include <gst/pbutils/missing-plugins.h>

#include <hybris/media/surface_texture_client_hybris.h>
#include <hybris/media/media_codec_layer.h>

#include "core/media/logger/logger.h"
#include "core/media/util/uri_check.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <utility>
#include <cstring>

static const char *PULSE_SINK = "pulsesink";
static const char *HYBRIS_SINK = "hybrissink";
static const char *MIR_SINK = "mirsink";

using namespace std;

void gstreamer::Playbin::setup_video_sink_for_buffer_streaming()
{
    IGBPWrapperHybris igbp;
    SurfaceTextureClientHybris stc;
    GstContext *context;
    GstStructure *structure;

    switch (backend) {
    case core::ubuntu::media::AVBackend::Backend::hybris:
        // Get the service-side BufferQueue (IGraphicBufferProducer) and
        // associate with it the SurfaceTextureClientHybris instance.
        igbp = decoding_service_get_igraphicbufferproducer();
        stc = surface_texture_client_create_by_igbp(igbp);

        // Because mirsink is being loaded, we are definitely doing * hardware rendering.
        surface_texture_client_set_hardware_rendering(stc, TRUE);

        context = gst_context_new("gst.mir.MirContext", TRUE);
        structure = gst_context_writable_structure(context);
        gst_structure_set(structure, "gst_mir_context", G_TYPE_POINTER, stc, NULL);

        /* Propagate context in pipeline (needed by amchybris and mirsink) */
        gst_element_set_context(pipeline, context);
        break;
    case core::ubuntu::media::AVBackend::Backend::mir:
        // Connect to buffer consumer socket
        connect_to_consumer();
        // Configure mirsink so it exports buffers (otherwise it would create
        // its own window).
        g_object_set (G_OBJECT (video_sink), "export-buffers", TRUE, nullptr);
        break;
    case core::ubuntu::media::AVBackend::Backend::none:
    default:
        throw core::ubuntu::media::Player::Errors::
            OutOfProcessBufferStreamingNotSupported{};
    }
}

bool gstreamer::Playbin::is_supported_video_sink(void) const
{
    if (video_sink_name == HYBRIS_SINK || video_sink_name ==  MIR_SINK)
        return TRUE;

    return FALSE;
}

// Uncomment to generate a dot file at the time that the pipeline
// goes to the PLAYING state. Make sure to export GST_DEBUG_DUMP_DOT_DIR
// before starting media-hub-server. To convert the dot file to something
// other image format, use: dot pipeline.dot -Tpng -o pipeline.png
//#define DEBUG_GST_PIPELINE

namespace media = core::ubuntu::media;
namespace video = core::ubuntu::media::video;

const std::string& gstreamer::Playbin::pipeline_name()
{
    static const std::string s{"playbin"};
    return s;
}

void gstreamer::Playbin::about_to_finish(GstElement*, gpointer user_data)
{
    auto thiz = static_cast<Playbin*>(user_data);
    thiz->signals.about_to_finish();
}

void gstreamer::Playbin::source_setup(GstElement*,
                                      GstElement *source,
                                      gpointer user_data)
{
    if (user_data == nullptr)
        return;

    static_cast<Playbin*>(user_data)->setup_source(source);
}

gstreamer::Playbin::Playbin(const core::ubuntu::media::Player::PlayerKey key_in)
    : pipeline(gst_element_factory_make("playbin", pipeline_name().c_str())),
      bus{gst_element_get_bus(pipeline)},
      file_type(MEDIA_FILE_TYPE_NONE),
      video_sink(nullptr),
      audio_sink(nullptr),
      on_new_message_connection_async(
          bus.on_new_message_async.connect(
              std::bind(
                  &Playbin::on_new_message_async,
                  this,
                  std::placeholders::_1))),
      is_seeking(false),
      previous_position(0),
      cached_video_dimensions{
        core::ubuntu::media::video::Height{0},
        core::ubuntu::media::video::Width{0}},
      player_lifetime(media::Player::Lifetime::normal),
      about_to_finish_handler_id(0),
      source_setup_handler_id(0),
      is_missing_audio_codec(false),
      is_missing_video_codec(false),
      audio_stream_id(-1),
      video_stream_id(-1),
      current_new_state(GST_STATE_NULL),
      key(key_in),
      backend(core::ubuntu::media::AVBackend::get_backend_type()),
      sock_consumer(-1)
{
    if (!pipeline)
        throw std::runtime_error("Could not create pipeline for playbin.");

    // Add audio and/or video sink elements depending on environment variables
    // being set or not set
    setup_pipeline_for_audio_video();

    about_to_finish_handler_id = g_signal_connect(
                pipeline,
                "about-to-finish",
                G_CALLBACK(about_to_finish),
                this
                );

    source_setup_handler_id = g_signal_connect(
        pipeline,
        "source-setup",
        G_CALLBACK(source_setup),
        this
        );
}

// Note that we might be accessing freed memory here, so activate DEBUG_REFS
// only for debugging
//#define DEBUG_REFS
#ifdef DEBUG_REFS
static void print_refs(const gstreamer::Playbin &pb, const char *func)
{
    using namespace std;

    MH_DEBUG("%s", func);
    if (pb.pipeline)
        MH_DEBUG("pipeline:   %d", (const void *) GST_OBJECT_REFCOUNT(pb.pipeline));
    if (pb.video_sink)
        MH_DEBUG("video_sink: %d", (const void *) GST_OBJECT_REFCOUNT(pb.video_sink));
    if (pb.audio_sink)
        MH_DEBUG("audio_sink: %d", (const void *) GST_OBJECT_REFCOUNT(pb.audio_sink));
}
#endif

gstreamer::Playbin::~Playbin()
{
#ifdef DEBUG_REFS
    print_refs(*this, "gstreamer::Playbin::~Playbin pipeline");
#endif

    g_signal_handler_disconnect(pipeline, about_to_finish_handler_id);
    g_signal_handler_disconnect(pipeline, source_setup_handler_id);

    if (pipeline)
        gst_object_unref(pipeline);

    if (sock_consumer != -1) {
        close(sock_consumer);
        sock_consumer = -1;
    }

#ifdef DEBUG_REFS
    print_refs(*this, "gstreamer::Playbin::~Playbin pipeline");
#endif
}

void gstreamer::Playbin::reset()
{
    MH_INFO("Client died, resetting pipeline");
    // When the client dies, tear down the current pipeline and get it
    // in a state that is ready for the next client that connects to the
    // service

    // Don't reset the pipeline if we want to resume
    if (player_lifetime != media::Player::Lifetime::resumable) {
        reset_pipeline();
    }
    // Signal to the Player class that the client side has disconnected
    signals.client_disconnected();
}

void gstreamer::Playbin::reset_pipeline()
{
    MH_TRACE("");
    const auto ret = gst_element_set_state(pipeline, GST_STATE_NULL);
    switch (ret)
    {
    case GST_STATE_CHANGE_FAILURE:
        MH_WARNING("Failed to reset the pipeline state. Client reconnect may not function properly.");
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
    case GST_STATE_CHANGE_SUCCESS:
    case GST_STATE_CHANGE_ASYNC:
        break;
    default:
        MH_WARNING("Failed to reset the pipeline state. Client reconnect may not function properly.");
    }
    file_type = MEDIA_FILE_TYPE_NONE;
    is_missing_audio_codec = false;
    is_missing_video_codec = false;
    audio_stream_id = -1;
    video_stream_id = -1;
    if (sock_consumer != -1) {
        close(sock_consumer);
        sock_consumer = -1;
    }
}

void gstreamer::Playbin::process_missing_plugin_message(GstMessage *message)
{
    gchar *desc = gst_missing_plugin_message_get_description(message);
    MH_WARNING("Missing plugin: %s", desc);
    g_free(desc);

    const GstStructure *msg_data = gst_message_get_structure(message);
    if (g_strcmp0("decoder", gst_structure_get_string(msg_data, "type")) != 0)
        return;

    GstCaps *caps;
    if (!gst_structure_get(msg_data, "detail", GST_TYPE_CAPS, &caps, NULL)) {
        MH_ERROR("No detail");
        return;
    }

    GstStructure *caps_data = gst_caps_get_structure(caps, 0);
    if (!caps_data) {
        MH_ERROR("No caps data");
        return;
    }

    const gchar *mime = gst_structure_get_name(caps_data);
    if (strstr(mime, "audio"))
        is_missing_audio_codec = true;
    else if (strstr(mime, "video"))
        is_missing_video_codec = true;

    MH_ERROR("Missing decoder for %s", mime);
}

void gstreamer::Playbin::process_message_element(GstMessage *message)
{
    const GstStructure *msg_data = gst_message_get_structure(message);
    const gchar *struct_name = gst_structure_get_name(msg_data);

    if (g_strcmp0("buffer-export-data", struct_name) == 0)
    {
        int fd;
        core::ubuntu::media::video::BufferMetadata meta;
        if (!gst_structure_get(msg_data,
                               "fd", G_TYPE_INT, &fd,
                               "width", G_TYPE_INT, &meta.width,
                               "height", G_TYPE_INT, &meta.height,
                               "fourcc", G_TYPE_INT, &meta.fourcc,
                               "stride", G_TYPE_INT, &meta.stride,
                               "offset", G_TYPE_INT, &meta.offset,
                               NULL))
        {
            MH_ERROR("Bad buffer-export-data message: mirsink version mismatch?");
            return;
        }
        MH_DEBUG("Exporting %dx%d buffer (fd %d)", meta.width, meta.height, fd);
        send_buffer_data(fd, &meta, sizeof meta);
    }
    else if (g_strcmp0("frame-ready", struct_name) == 0)
    {
        send_frame_ready();
    }
    else
    {
        MH_ERROR("Unknown GST_MESSAGE_ELEMENT with struct %s", struct_name);
    }
}

void gstreamer::Playbin::on_new_message_async(const Bus::Message& message)
{
    switch (message.type)
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
    case GST_MESSAGE_STATE_CHANGED:
        if (message.source == "playbin") {
            g_object_get(G_OBJECT(pipeline), "current-audio", &audio_stream_id, NULL);
            g_object_get(G_OBJECT(pipeline), "current-video", &video_stream_id, NULL);
        }
        signals.on_state_changed(std::make_pair(message.detail.state_changed, message.source));
        break;
    case GST_MESSAGE_ELEMENT:
        if (gst_is_missing_plugin_message(message.message))
            process_missing_plugin_message(message.message);
        else
            process_message_element(message.message);
        break;
    case GST_MESSAGE_TAG:
        {
            gchar *orientation;
            if (gst_tag_list_get_string(message.detail.tag.tag_list, "image-orientation", &orientation))
            {
                // If the image-orientation tag is in the GstTagList, signal the Engine
                signals.on_orientation_changed(orientation_lut(orientation));
                g_free (orientation);
            }

            signals.on_tag_available(message.detail.tag);
        }
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
        break;
    case GST_MESSAGE_BUFFERING:
        signals.on_buffering_changed(message.detail.buffering.percent);
        break;
    default:
        break;
    }
}

gstreamer::Bus& gstreamer::Playbin::message_bus()
{
    return bus;
}

void gstreamer::Playbin::setup_pipeline_for_audio_video()
{
    gint flags;
    g_object_get (pipeline, "flags", &flags, nullptr);
    flags |= GST_PLAY_FLAG_AUDIO;
    flags |= GST_PLAY_FLAG_VIDEO;
    flags &= ~GST_PLAY_FLAG_TEXT;
    g_object_set (pipeline, "flags", flags, nullptr);

    const char *asink_name = ::getenv("CORE_UBUNTU_MEDIA_SERVICE_AUDIO_SINK_NAME");

    if (asink_name == nullptr)
        asink_name = PULSE_SINK;

    audio_sink = gst_element_factory_make (asink_name, "audio-sink");
    if (audio_sink)
        g_object_set (pipeline, "audio-sink", audio_sink, NULL);
    else
        MH_ERROR("Error trying to create audio sink %s", asink_name);

    const char *vsink_name = ::getenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME");

    if (vsink_name == nullptr) {
        if (backend == core::ubuntu::media::AVBackend::Backend::hybris)
            vsink_name = HYBRIS_SINK;
        else if (backend == core::ubuntu::media::AVBackend::Backend::mir)
            vsink_name = MIR_SINK;
    }

    if (vsink_name) {
        video_sink_name = vsink_name;
        video_sink = gst_element_factory_make (vsink_name, "video-sink");
        if (video_sink)
            g_object_set (pipeline, "video-sink", video_sink, NULL);
        else
            MH_ERROR("Error trying to create video sink %s", vsink_name);
    }
}

void gstreamer::Playbin::create_video_sink(uint32_t)
{
    if (not video_sink) throw std::logic_error
    {
        "No video sink configured for the current pipeline"
    };

    setup_video_sink_for_buffer_streaming();
}

void gstreamer::Playbin::set_volume(double new_volume)
{
    g_object_set (pipeline, "volume", new_volume, NULL);
}

/** Translate the AudioStreamRole enum into a string */
std::string gstreamer::Playbin::get_audio_role_str(media::Player::AudioStreamRole audio_role)
{
    switch (audio_role)
    {
        case media::Player::AudioStreamRole::alarm:
            return "alarm";
            break;
        case media::Player::AudioStreamRole::alert:
            return "alert";
            break;
        case media::Player::AudioStreamRole::multimedia:
            return "multimedia";
            break;
        case media::Player::AudioStreamRole::phone:
            return "phone";
            break;
        default:
            return "multimedia";
            break;
    }
}

media::Player::Orientation gstreamer::Playbin::orientation_lut(const gchar *orientation)
{
    if (g_strcmp0(orientation, "rotate-0") == 0)
        return media::Player::Orientation::rotate0;
    else if (g_strcmp0(orientation, "rotate-90") == 0)
        return media::Player::Orientation::rotate90;
    else if (g_strcmp0(orientation, "rotate-180") == 0)
        return media::Player::Orientation::rotate180;
    else if (g_strcmp0(orientation, "rotate-270") == 0)
        return media::Player::Orientation::rotate270;
    else
        return media::Player::Orientation::rotate0;
}

/** Sets the new audio stream role on the pulsesink in playbin */
void gstreamer::Playbin::set_audio_stream_role(media::Player::AudioStreamRole new_audio_role)
{
    const std::string role_str("props,media.role=" + get_audio_role_str(new_audio_role));
    MH_INFO("Audio stream role: %s", role_str);

    GstStructure *props = gst_structure_from_string (role_str.c_str(), NULL);
    if (audio_sink != nullptr && props != nullptr)
    {
        g_object_set (audio_sink, "stream-properties", props, NULL);
    }
    else
    {
        MH_WARNING("Couldn't set audio stream role - couldn't get audio_sink from pipeline");
    }

    gst_structure_free (props);
}

void gstreamer::Playbin::set_lifetime(media::Player::Lifetime lifetime)
{
    player_lifetime = lifetime;
}

uint64_t gstreamer::Playbin::position() const
{
    int64_t pos = 0;
    gst_element_query_position (pipeline, GST_FORMAT_TIME, &pos);

    // This prevents a 0 position from being reported to the app which happens while seeking.
    // This is covering over a GStreamer issue
    if ((static_cast<uint64_t>(pos) < duration()) && is_seeking && pos == 0)
    {
        return previous_position;
    }

    // Save the current position to use just in case it's needed the next time position is
    // requested
    previous_position = static_cast<uint64_t>(pos);

    // FIXME: this should be int64_t, but dbus-cpp doesn't seem to handle it correctly
    return static_cast<uint64_t>(pos);
}

uint64_t gstreamer::Playbin::duration() const
{
    int64_t dur = 0;
    gst_element_query_duration (pipeline, GST_FORMAT_TIME, &dur);

    // FIXME: this should be int64_t, but dbus-cpp doesn't seem to handle it correctly
    return static_cast<uint64_t>(dur);
}

void gstreamer::Playbin::set_uri(
    const std::string& uri,
    const core::ubuntu::media::Player::HeadersType& headers = core::ubuntu::media::Player::HeadersType(),
    bool do_pipeline_reset)
{
    gchar *current_uri = nullptr;
    g_object_get(pipeline, "current-uri", &current_uri, NULL);

    // Checking for a current_uri being set and not resetting the pipeline
    // if there isn't a current_uri causes the first play to start playback
    // sooner since reset_pipeline won't be called
    if (current_uri and do_pipeline_reset)
        reset_pipeline();

    std::string tmp_uri{uri};
    media::UriCheck::Ptr uri_check{std::make_shared<media::UriCheck>(uri)};
    if (uri_check->is_local_file())
    {
        if (uri_check->is_encoded())
        {
            // First decode the URI just in case it's partially encoded already
            tmp_uri = decode_uri(uri);
            MH_DEBUG("File URI was encoded, now decoded: %s", tmp_uri);
        }
        tmp_uri = encode_uri(tmp_uri);
    }

    g_object_set(pipeline, "uri", tmp_uri.c_str(), NULL);
    if (is_video_file(tmp_uri))
        file_type = MEDIA_FILE_TYPE_VIDEO;
    else if (is_audio_file(tmp_uri))
        file_type = MEDIA_FILE_TYPE_AUDIO;

    request_headers = headers;

    g_free(current_uri);
}

void gstreamer::Playbin::setup_source(GstElement *source)
{
    if (source == NULL || request_headers.empty())
        return;

    if (request_headers.find("Cookie") != request_headers.end()) {
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(source),
                                         "cookies") != NULL) {
            gchar ** cookies = g_strsplit(request_headers["Cookie"].c_str(), ";", 0);
            g_object_set(source, "cookies", cookies, NULL);
            g_strfreev(cookies);
        }
    }

    if (request_headers.find("User-Agent") != request_headers.end()) {
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(source),
                                         "user-agent") != NULL) {
            g_object_set(source, "user-agent", request_headers["User-Agent"].c_str(), NULL);
        }
    }
}

std::string gstreamer::Playbin::uri() const
{
    gchar* data = nullptr;
    g_object_get(pipeline, "current-uri", &data, nullptr);

    std::string result((data == nullptr ? "" : data));
    g_free(data);

    return result;
}

gboolean gstreamer::Playbin::set_state_in_main_thread(gpointer user_data)
{
    MH_TRACE("");
    auto thiz = static_cast<Playbin*>(user_data);
    if (thiz and thiz->pipeline)
        gst_element_set_state(thiz->pipeline, thiz->current_new_state);

    // Always return false so this is a single shot function call
    return false;
}
bool gstreamer::Playbin::set_state_and_wait(GstState new_state, bool use_main_thread)
{
    static const std::chrono::nanoseconds state_change_timeout
    {
        // We choose a quite high value here as tests are run under valgrind
        // and gstreamer pipeline setup/state changes take longer in that scenario.
        // The value does not negatively impact runtime performance.
        std::chrono::milliseconds{5000}
    };

    bool result = false;
    GstStateChangeReturn state_change_return;
    GstState current, pending;
    if (use_main_thread)
    {
        // Cache this value for the static g_idle_add handler function
        current_new_state = new_state;
        g_idle_add((GSourceFunc) gstreamer::Playbin::set_state_in_main_thread, (gpointer) this);

        MH_DEBUG("Requested state change in main thread context.");

        GstState current, pending;
        state_change_return = gst_element_get_state(
                pipeline,
                &current,
                &pending,
                state_change_timeout.count());
        result = state_change_return != GST_STATE_CHANGE_FAILURE;
    }
    else
    {
        const auto ret = gst_element_set_state(pipeline, new_state);

        MH_DEBUG("Requested state change not using main thread context.");

        switch (ret)
        {
            case GST_STATE_CHANGE_FAILURE:
                result = false; break;
            case GST_STATE_CHANGE_NO_PREROLL:
            case GST_STATE_CHANGE_SUCCESS:
                state_change_return = ret;
                result = true; break;
            case GST_STATE_CHANGE_ASYNC:
                state_change_return = gst_element_get_state(
                        pipeline,
                        &current,
                        &pending,
                        state_change_timeout.count());
                result = state_change_return != GST_STATE_CHANGE_FAILURE;
                break;
        }
    }

    // We can query the pipeline if it has data
    if (result && state_change_return == GST_STATE_CHANGE_SUCCESS)
    {
        // Get the video height/width from the video sink
        try
        {
            const core::ubuntu::media::video::Dimensions new_dimensions = get_video_dimensions();
            emit_video_dimensions_changed_if_changed(new_dimensions);
            cached_video_dimensions = new_dimensions;
        }
        catch (const std::exception& e)
        {
            MH_WARNING("Problem querying video dimensions: %s", e.what());
        }
        catch (...)
        {
            MH_WARNING("Problem querying video dimensions.");
        }

#ifdef DEBUG_GST_PIPELINE
        MH_DEBUG("Dumping pipeline dot file");
        GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
#endif
    }

    return result;
}

bool gstreamer::Playbin::seek(const std::chrono::microseconds& ms)
{
    is_seeking = true;
    return gst_element_seek_simple(
                pipeline,
                GST_FORMAT_TIME,
                (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                ms.count() * 1000);
}

core::ubuntu::media::video::Dimensions gstreamer::Playbin::get_video_dimensions() const
{
    if (not video_sink || not is_supported_video_sink())
        throw std::runtime_error
        {
            "Missing video sink or video sink does not support query of width and height."
        };

    // Initialize to default value prior to querying actual values from the sink.
    int video_width = 0, video_height = 0;

    // There should be only one pad actually
    GstIterator *iter = gst_element_iterate_pads(video_sink);
    for (GValue item{};
         gst_iterator_next(iter, &item) == GST_ITERATOR_OK;
         g_value_unset(&item))
    {
        GstPad *pad = GST_PAD(g_value_get_object(&item));
        GstCaps *caps = gst_pad_get_current_caps(pad);

        if (caps) {
            const GstStructure *s = gst_caps_get_structure(caps, 0);
            gst_structure_get_int(s, "width", &video_width);
            gst_structure_get_int(s, "height", &video_height);
            MH_DEBUG("Video dimensions are %d x %d", video_width, video_height);

            gst_caps_unref(caps);
        }
    }
    gst_iterator_free(iter);

    // TODO(tvoss): We should probably check here if width and height are valid.
    return core::ubuntu::media::video::Dimensions
    {
        core::ubuntu::media::video::Height{video_height},
        core::ubuntu::media::video::Width{video_width}
    };
}

void gstreamer::Playbin::emit_video_dimensions_changed_if_changed(const core::ubuntu::media::video::Dimensions &new_dimensions)
{
    // Only signal the application layer if the dimensions have in fact changed. This might happen
    // if reusing the same media-hub session to play two different video sources.
    if (new_dimensions != cached_video_dimensions)
        signals.on_video_dimensions_changed(new_dimensions);
}

std::string gstreamer::Playbin::file_info_from_uri(const std::string& uri) const
{
    GError *error = nullptr;
    // Open the URI and get the mime type from it. This will currently only work for
    // a local file
    std::unique_ptr<GFile, void(*)(void *)> file(
            g_file_new_for_uri(uri.c_str()), g_object_unref);
    std::unique_ptr<GFileInfo, void(*)(void *)> info(
            g_file_query_info(
                file.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE ","
                G_FILE_ATTRIBUTE_ETAG_VALUE, G_FILE_QUERY_INFO_NONE,
                /* cancellable */ NULL, &error),
            g_object_unref);
    if (!info)
        return std::string();

    std::string content_type = g_file_info_get_attribute_string(
                info.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
    if (content_type.empty())
        return std::string();

    if (content_type == "application/octet-stream")
    {
        std::unique_ptr<GFileInfo, void(*)(void *)> full_info(
                    g_file_query_info(file.get(), G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                      G_FILE_QUERY_INFO_NONE,
                                      /* cancellable */ NULL, &error),g_object_unref);

        if (!full_info)
            return std::string();

        content_type = g_file_info_get_attribute_string(
                    full_info.get(), G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
        if (content_type.empty())
            return std::string();
    }
    return content_type;
}

std::string gstreamer::Playbin::encode_uri(const std::string& uri) const
{
    if (uri.empty())
        return std::string();

    std::string encoded_uri;
    media::UriCheck::Ptr uri_check{std::make_shared<media::UriCheck>(uri)};
    gchar *uri_scheme = g_uri_parse_scheme(uri.c_str());
    // We have a URI and it is already percent encoded
    if (uri_scheme and strlen(uri_scheme) > 0 and uri_check->is_encoded())
    {
        MH_DEBUG("Is a URI and is already percent encoded");
        encoded_uri = uri;
    }
    // We have a URI but it's not already percent encoded
    else if (uri_scheme and strlen(uri_scheme) > 0 and !uri_check->is_encoded())
    {
        MH_DEBUG("Is a URI and is not already percent encoded");
        gchar *encoded = g_uri_escape_string(uri.c_str(),
                                                   "!$&'()*+,;=:/?[]@", // reserved chars
                                                   TRUE); // Allow UTF-8 chars
        if (!encoded)
        {
            g_free(uri_scheme);
            return std::string();
        }
        encoded_uri.assign(encoded);
        g_free(encoded);
    }
    else // We have a path and not a URI. Turn it into a full URI and encode it
    {
        GError *error = nullptr;
        MH_DEBUG("Is a path and is not already percent encoded");
        gchar *str = g_filename_to_uri(uri.c_str(), nullptr, &error);
        if (!str)
        {
            g_free(uri_scheme);
            return std::string();
        }
        encoded_uri.assign(str);
        g_free(str);
        if (error != nullptr)
        {
            MH_WARNING("Failed to get actual track content type: %s", error->message);
            g_error_free(error);
            g_free(str);
            g_free(uri_scheme);
            return std::string("audio/video/");
        }
        gchar *escaped = g_uri_escape_string(encoded_uri.c_str(),
                                         "!$&'()*+,;=:/?[]@", // reserved chars
                                         TRUE); // Allow UTF-8 chars
        if (!escaped)
        {
            g_free(uri_scheme);
            return std::string();
        }
        encoded_uri.assign(escaped);
        g_free(escaped);
    }

    g_free(uri_scheme);

    return encoded_uri;
}

std::string gstreamer::Playbin::decode_uri(const std::string& uri) const
{
    if (uri.empty())
        return std::string();

    gchar *decoded_gchar = g_uri_unescape_string(uri.c_str(), nullptr);
    if (!decoded_gchar)
        return std::string();

    const std::string decoded{decoded_gchar};
    g_free(decoded_gchar);
    return decoded;
}

std::string gstreamer::Playbin::get_file_content_type(const std::string& uri) const
{
    if (uri.empty())
        return std::string();

    const std::string encoded_uri{encode_uri(uri)};

    const std::string content_type {file_info_from_uri(encoded_uri)};
    if (content_type.empty())
    {
        MH_WARNING("Failed to get actual track content type");
        return std::string("audio/video/");
    }

    MH_INFO("Found content type: %s", content_type);

    return content_type;
}

bool gstreamer::Playbin::is_audio_file(const std::string& uri) const
{
    if (uri.empty())
        return false;

    if (get_file_content_type(uri).find("audio/") == 0)
    {
        MH_INFO("Found audio content");
        return true;
    }

    return false;
}

bool gstreamer::Playbin::is_video_file(const std::string& uri) const
{
    if (uri.empty())
        return false;

    if (get_file_content_type(uri).find("video/") == 0)
    {
        MH_INFO("Found video content");
        return true;
    }

    return false;
}

gstreamer::Playbin::MediaFileType gstreamer::Playbin::media_file_type() const
{
    return file_type;
}

bool gstreamer::Playbin::can_play_streams() const
{
    /*
     * We do not consider that we can play the video when
     * 1. No audio stream selected due to missing decoder
     * 2. No video stream selected due to missing decoder
     * 3. No stream selected at all
     * Note that if there are several, say, audio streams, we will play the file
     * provided that we can decode just one of them, even if there are missing
     * audio codecs. We will also play files with only one type of stream.
     */
    if ((is_missing_audio_codec && audio_stream_id == -1) ||
            (is_missing_video_codec && video_stream_id == -1) ||
            (audio_stream_id == -1 && video_stream_id == -1))
        return false;
    else
        return true;
}

bool gstreamer::Playbin::connect_to_consumer(void)
{
    static const char *local_socket = "media-hub-server";
    static const char *consumer_socket = "media-consumer";

    int len;
    struct sockaddr_un local, remote;

    if (sock_consumer != -1) {
        MH_DEBUG("Resetting socket");
        close(sock_consumer);
    }

    if ((sock_consumer = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        MH_ERROR("Cannot create socket: %s (%d)", strerror(errno), errno);
        return false;
    }

    // Bind client to local -abstract- socket (media-hub-server<session>)
    ostringstream local_ss;
    local_ss << local_socket << key;
    local.sun_family = AF_UNIX;
    local.sun_path[0] = '\0';
    strcpy(local.sun_path + 1, local_ss.str().c_str());
    len = sizeof(local.sun_family) + local_ss.str().length() + 1;
    if (bind(sock_consumer, (struct sockaddr *) &local, len) == -1)
    {
        MH_ERROR("Cannot bind socket: %s (%d)", strerror(errno), errno);
        close(sock_consumer);
        sock_consumer = -1;
        return false;
    }

    // Connect to buffer consumer (media-consumer<session>)
    ostringstream remote_ss;
    remote_ss << consumer_socket << key;
    remote.sun_family = AF_UNIX;
    remote.sun_path[0] = '\0';
    strcpy(remote.sun_path + 1, remote_ss.str().c_str());
    len = sizeof(remote.sun_family) + remote_ss.str().length() + 1;
    if (connect(sock_consumer, (struct sockaddr *) &remote, len) == -1)
    {
        MH_ERROR("Cannot connect to consumer: %s (%d)", strerror(errno), errno);
        close(sock_consumer);
        sock_consumer = -1;
        return false;
    }

    MH_DEBUG("Connected to buffer consumer socket");

    return true;
}

void gstreamer::Playbin::send_buffer_data(int fd, void *data, size_t len)
{
    struct msghdr msg{};
    char buf[CMSG_SPACE(sizeof fd)]{};
    struct cmsghdr *cmsg;
    struct iovec io = { .iov_base = data, .iov_len = len };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof buf;

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof fd);

    memmove(CMSG_DATA(cmsg), &fd, sizeof fd);

    msg.msg_controllen = cmsg->cmsg_len;

    if (sendmsg(sock_consumer, &msg, 0) < 0)
        MH_ERROR("Failed to send dma_buf fd to consumer: %s (%d)",
                 strerror(errno), errno);
}

void gstreamer::Playbin::send_frame_ready(void)
{
    const char ready = 'r';

    if (send (sock_consumer, &ready, sizeof ready, 0) == -1)
        MH_ERROR("Error when sending frame ready flag to client: %s (%d)",
                 strerror(errno), errno);
}
