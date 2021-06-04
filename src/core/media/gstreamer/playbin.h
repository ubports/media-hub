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

#include "core/media/player.h"

#include <QObject>
#include <QString>
#include <QUrl>

#include <gio/gio.h>
#include <gst/gst.h>

#include <chrono>
#include <string>

// Uncomment to generate a dot file at the time that the pipeline
// goes to the PLAYING state. Make sure to export GST_DEBUG_DUMP_DOT_DIR
// before starting media-hub-server. To convert the dot file to some
// other image format, use: dot pipeline.dot -Tpng -o pipeline.png
//#define DEBUG_GST_PIPELINE

namespace gstreamer
{
class Playbin: public QObject
{
    Q_OBJECT

public:
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

    static std::string get_audio_role_str(core::ubuntu::media::Player::AudioStreamRole audio_role);

    static const std::string& pipeline_name();

    static void about_to_finish(GstElement*, gpointer user_data);

    static void source_setup(GstElement*,
                             GstElement *source,
                             gpointer user_data);
    static void streams_changed(GstElement*, gpointer user_data);

    Playbin(const core::ubuntu::media::Player::PlayerKey key);
    ~Playbin();

    void reset();
    void reset_pipeline();

    void on_new_message(const Bus::Message& message);
    void processVideoSinkStateChanged(const Bus::Message::Detail::StateChanged &state);
    void process_message_element(GstMessage *message);

    gstreamer::Bus& message_bus();

    void setup_pipeline_for_audio_video();

    void create_video_sink(uint32_t texture_id);

    void set_volume(double new_volume);

    void set_lifetime(core::ubuntu::media::Player::Lifetime);
    core::ubuntu::media::Player::Orientation orientation_lut(const gchar *orientation);

    /** Sets the new audio stream role on the pulsesink in playbin */
    void set_audio_stream_role(core::ubuntu::media::Player::AudioStreamRole new_audio_role);

    /** Returns the current stream position in nanoseconds */
    uint64_t position() const;
    /** Returns the current stream duration in nanoseconds */
    uint64_t duration() const;

    void set_uri(const QUrl &uri, const core::ubuntu::media::Player::HeadersType& headers, bool do_pipeline_reset = true);
    QUrl uri() const;

    void setup_source(GstElement *source);
    void updateMediaFileType();

    // Sets the pipeline's state (stopped, playing, paused, etc).
    bool set_state(GstState new_state);
    bool seek(const std::chrono::microseconds& ms);

    QSize get_video_dimensions() const;

    QString file_info_from_uri(const QUrl &uri) const;
    QString get_file_content_type(const QUrl &uri) const;

    bool is_audio_file(const QUrl &uri) const;
    bool is_video_file(const QUrl &uri) const;

    MediaFileType mediaFileType() const;

    bool can_play_streams() const;

    GstElement* pipeline;
    gstreamer::Bus bus;
    MediaFileType m_fileType;
    GstElement* video_sink;
    GstElement* audio_sink;
    bool is_seeking;
    mutable uint64_t previous_position;
    core::ubuntu::media::Player::HeadersType request_headers;
    core::ubuntu::media::Player::Lifetime player_lifetime;
    gulong about_to_finish_handler_id;
    gulong source_setup_handler_id;
    gulong m_audioChangedHandlerId;
    gulong m_videoChangedHandlerId;
    bool is_missing_audio_codec;
    bool is_missing_video_codec;
    gint audio_stream_id;
    gint video_stream_id;
    GstState current_new_state;

Q_SIGNALS:
    void errorOccurred(const Bus::Message::Detail::ErrorWarningInfo &);
    void warningOccurred(const Bus::Message::Detail::ErrorWarningInfo &);
    void infoOccurred(const Bus::Message::Detail::ErrorWarningInfo &);

    void aboutToFinish();
    void seekedTo(uint64_t offset);
    void stateChanged(const Bus::Message::Detail::StateChanged &state,
                      const QByteArray &source);
    void mediaFileTypeChanged();
    void tagAvailable(Bus::Message::Detail::Tag tag);
    void orientationChanged(core::ubuntu::media::Player::Orientation o);
    void videoDimensionChanged(const QSize &size);
    void bufferingChanged(int progress);
    void clientDisconnected();
    void endOfStream();

protected:
    void setMediaFileType(MediaFileType fileType);

private:
    void setup_video_sink_for_buffer_streaming(void);
    bool is_supported_video_sink(void) const;
    bool connect_to_consumer(void);
    void send_buffer_data(int fd, void *data, size_t len);
    void send_frame_ready(void);
    void process_missing_plugin_message(GstMessage *message);

    const core::ubuntu::media::Player::PlayerKey key;
    const core::ubuntu::media::AVBackend::Backend backend;
    std::string video_sink_name;
    int sock_consumer;
};
}

#endif // GSTREAMER_PLAYBIN_H_
