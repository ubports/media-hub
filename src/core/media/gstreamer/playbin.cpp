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
 */

#include <core/media/gstreamer/playbin.h>
#include <core/media/gstreamer/engine.h>

#include <gst/pbutils/missing-plugins.h>

#if defined(MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
#include <hybris/media/surface_texture_client_hybris.h>
#include <hybris/media/media_codec_layer.h>

#include "../util/uri_check.h"

#include <utility>

//#define VERBOSE_DEBUG

namespace
{
void setup_video_sink_for_buffer_streaming(GstElement* pipeline)
{
    // Get the service-side BufferQueue (IGraphicBufferProducer) and associate it with
    // the SurfaceTextureClientHybris instance
    IGBPWrapperHybris igbp = decoding_service_get_igraphicbufferproducer();
    SurfaceTextureClientHybris stc = surface_texture_client_create_by_igbp(igbp);

    // Because mirsink is being loaded, we are definitely doing * hardware rendering.
    surface_texture_client_set_hardware_rendering(stc, TRUE);

    GstContext *context = gst_context_new("gst.mir.MirContext", TRUE);
    GstStructure *structure = gst_context_writable_structure(context);
    gst_structure_set(structure, "gst_mir_context", G_TYPE_POINTER, stc, NULL);

    /* Propagate context in pipeline (needed by amchybris and mirsink) */
    gst_element_set_context(pipeline, context);
}
}
#else  // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER
namespace
{
void setup_video_sink_for_buffer_streaming(GstElement*)
{
    throw core::ubuntu::media::Player::Errors::OutOfProcessBufferStreamingNotSupported{};
}
}
#endif // MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER

namespace
{
bool is_mir_video_sink()
{
    return g_strcmp0(::getenv("CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME"), "mirsink") == 0;
}
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

gstreamer::Playbin::Playbin()
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
      video_stream_id(-1)
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

    cout << func << endl;
    if (pb.pipeline)
        cout << "pipeline:   " << GST_OBJECT_REFCOUNT(pb.pipeline) << endl;
    if (pb.video_sink)
        cout << "video_sink: " << GST_OBJECT_REFCOUNT(pb.video_sink) << endl;
    if (pb.audio_sink)
        cout << "audio_sink: " << GST_OBJECT_REFCOUNT(pb.audio_sink) << endl;
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

#ifdef DEBUG_REFS
    print_refs(*this, "gstreamer::Playbin::~Playbin pipeline");
#endif
}

void gstreamer::Playbin::reset()
{
    std::cout << "Client died, resetting pipeline" << std::endl;
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
    is_missing_audio_codec = false;
    is_missing_video_codec = false;
    audio_stream_id = -1;
    video_stream_id = -1;
}

void gstreamer::Playbin::process_message_element(GstMessage *message)
{
    if (!gst_is_missing_plugin_message(message))
      return;

    gchar *desc = gst_missing_plugin_message_get_description(message);
    std::cerr << "Missing plugin: " << desc << std::endl;
    g_free(desc);

    const GstStructure *msg_data = gst_message_get_structure(message);
    if (g_strcmp0("decoder", gst_structure_get_string(msg_data, "type")) != 0)
        return;

    GstCaps *caps;
    if (!gst_structure_get(msg_data, "detail", GST_TYPE_CAPS, &caps, NULL)) {
        std::cerr << __PRETTY_FUNCTION__ << ": No detail" << std::endl;
        return;
    }

    GstStructure *caps_data = gst_caps_get_structure(caps, 0);
    if (!caps_data) {
        std::cerr << __PRETTY_FUNCTION__ << ": No caps data" << std::endl;
        return;
    }

    const gchar *mime = gst_structure_get_name(caps_data);
    if (strstr(mime, "audio"))
        is_missing_audio_codec = true;
    else if (strstr(mime, "video"))
        is_missing_video_codec = true;

    std::cerr << "Missing decoder for " << mime << std::endl;
}

void gstreamer::Playbin::on_new_message_async(const Bus::Message& message)
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
    case GST_MESSAGE_STATE_CHANGED:
        if (message.source == "playbin") {
            g_object_get(G_OBJECT(pipeline), "current-audio", &audio_stream_id, NULL);
            g_object_get(G_OBJECT(pipeline), "current-video", &video_stream_id, NULL);
        }
        signals.on_state_changed(std::make_pair(message.detail.state_changed, message.source));
        break;
    case GST_MESSAGE_ELEMENT:
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

    if (::getenv("CORE_UBUNTU_MEDIA_SERVICE_AUDIO_SINK_NAME") != nullptr)
    {
        audio_sink = gst_element_factory_make (
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
        video_sink = gst_element_factory_make (
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

void gstreamer::Playbin::create_video_sink(uint32_t)
{
    if (not video_sink) throw std::logic_error
    {
        "No video sink configured for the current pipeline"
    };

    setup_video_sink_for_buffer_streaming(pipeline);
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
    std::string role_str("props,media.role=" + get_audio_role_str(new_audio_role));
    std::cout << "Audio stream role: " << role_str << std::endl;

    GstStructure *props = gst_structure_from_string (role_str.c_str(), NULL);
    if (audio_sink != nullptr && props != nullptr) {
        g_object_set (audio_sink, "stream-properties", props, NULL);
    } else {
        std::cerr <<
            "Warning: couldn't set audio stream role - couldn't get audio_sink from pipeline" <<
            std::endl;
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

    const std::string encoded_uri{encode_uri(uri)};
    g_object_set(pipeline, "uri", encoded_uri.c_str(), NULL);
    if (is_video_file(encoded_uri))
        file_type = MEDIA_FILE_TYPE_VIDEO;
    else if (is_audio_file(encoded_uri))
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

bool gstreamer::Playbin::set_state_and_wait(GstState new_state)
{
    static const std::chrono::nanoseconds state_change_timeout
    {
        // We choose a quite high value here as tests are run under valgrind
        // and gstreamer pipeline setup/state changes take longer in that scenario.
        // The value does not negatively impact runtime performance.
        std::chrono::milliseconds{5000}
    };

    auto ret = gst_element_set_state(pipeline, new_state);

    std::cout << __PRETTY_FUNCTION__ << ": requested state change." << std::endl;

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

    // We only should query the pipeline if we actually succeeded in
    // setting the requested state.
    if (result && new_state == GST_STATE_PLAYING)
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
            std::cerr << "Problem querying video dimensions: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Problem querying video dimensions." << std::endl;
        }

#ifdef DEBUG_GST_PIPELINE
        std::cout << "Dumping pipeline dot file" << std::endl;
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
    if (not video_sink || not is_mir_video_sink())
        throw std::runtime_error
        {
            "Missing video sink or video sink does not support query of width and height."
        };

    // Initialize to default value prior to querying actual values from the sink.
    uint32_t video_width = 0, video_height = 0;
    g_object_get (video_sink, "height", &video_height, nullptr);
    g_object_get (video_sink, "width", &video_width, nullptr);

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

    return std::string(g_file_info_get_attribute_string(
                info.get(), G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE));
}

std::string gstreamer::Playbin::encode_uri(const std::string& uri) const
{
    if (uri.empty())
        return std::string();

    std::string encoded_uri;
    media::UriCheck::Ptr uri_check{std::make_shared<media::UriCheck>(uri)};
    const std::string uri_scheme {g_uri_parse_scheme(uri.c_str())};
    // We have a URI and it is already percent encoded
    if (!uri_scheme.empty() and uri_check->is_encoded())
    {
#ifdef VERBOSE_DEBUG
        std::cout << "Is a URI and is already percent encoded" << std::endl;
#endif
        encoded_uri = uri;
    }
    // We have a URI but it's not already percent encoded
    else if (!uri_scheme.empty() and !uri_check->is_encoded())
    {
#ifdef VERBOSE_DEBUG
        std::cout << "Is a URI and is not already percent encoded" << std::endl;
#endif
        encoded_uri.assign(g_uri_escape_string(uri.c_str(),
                                               "!$&'()*+,;=:/?[]@", // reserved chars
                                               TRUE)); // Allow UTF-8 chars
    }
    else // We have a path and not a URI. Turn it into a full URI and encode it
    {
        GError *error = nullptr;
#ifdef VERBOSE_DEBUG
        std::cout << "Is a path and is not already percent encoded" << std::endl;
#endif
        encoded_uri.assign(g_filename_to_uri(uri.c_str(), nullptr, &error));
        if (error != nullptr)
        {
            std::cerr << "Warning: failed to get actual track content type: " << error->message
                      << std::endl;
            g_error_free(error);
            return std::string("audio/video/");
        }
        encoded_uri.assign(g_uri_escape_string(uri.c_str(),
                                               "!$&'()*+,;=:/?[]@", // reserved chars
                                               TRUE)); // Allow UTF-8 chars
    }

    return encoded_uri;
}

std::string gstreamer::Playbin::decode_uri(const std::string& uri) const
{
    if (uri.empty())
        return std::string();

    return std::string(g_uri_unescape_string(uri.c_str(), nullptr));
}

std::string gstreamer::Playbin::get_file_content_type(const std::string& uri) const
{
    if (uri.empty())
        return std::string();

    const std::string encoded_uri{encode_uri(uri)};

    const std::string content_type {file_info_from_uri(encoded_uri)};
    if (content_type.empty())
    {
        std::cerr << "Warning: failed to get actual track content type" << std::endl;
        return std::string("audio/video/");
    }

    std::cout << "Found content type: " << content_type << std::endl;

    return content_type;
}

bool gstreamer::Playbin::is_audio_file(const std::string& uri) const
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

bool gstreamer::Playbin::is_video_file(const std::string& uri) const
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
