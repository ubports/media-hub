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

#include "bus.h"
#include "engine.h"
#include "meta_data_extractor.h"
#include "playbin.h"

#include <cassert>

namespace music = com::ubuntu::music;

namespace gstreamer
{
struct Init
{
    Init()
    {
        gst_init(nullptr, nullptr);
    }

    ~Init()
    {
        gst_deinit();
    }
} init;
}

struct gstreamer::Engine::Private
{
    void about_to_finish()
    {
        state = Engine::State::ready;
    }

    void on_playbin_state_changed(
            const gstreamer::Bus::Message::Detail::StateChanged&)
    {
    }

    void on_tag_available(const gstreamer::Bus::Message::Detail::Tag& tag)
    {
        music::Track::MetaData md;

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

        track_meta_data.set(std::make_tuple(playbin.uri(), md));
    }

    void on_volume_changed(const music::Engine::Volume& new_volume)
    {
        playbin.set_volume(new_volume.value);
    }

    Private()
        : meta_data_extractor(new gstreamer::MetaDataExtractor()),
          volume(music::Engine::Volume(1.)),
          about_to_finish_connection(
              playbin.signals.about_to_finish.connect(
                  std::bind(
                      &Private::about_to_finish,
                      this))),
          on_state_changed_connection(
              playbin.signals.on_state_changed.connect(
                  std::bind(
                      &Private::on_playbin_state_changed,
                      this,
                      std::placeholders::_1))),
          on_tag_available_connection(
              playbin.signals.on_tag_available.connect(
                  std::bind(
                      &Private::on_tag_available,
                      this,
                      std::placeholders::_1))),
          on_volume_changed_connection(
              volume.changed().connect(
                  std::bind(
                      &Private::on_volume_changed,
                      this,
                      std::placeholders::_1)))
    {
    }

    std::shared_ptr<Engine::MetaDataExtractor> meta_data_extractor;
    music::Property<Engine::State> state;
    music::Property<std::tuple<music::Track::UriType, music::Track::MetaData>> track_meta_data;
    music::Property<music::Engine::Volume> volume;
    gstreamer::Playbin playbin;
    music::ScopedConnection about_to_finish_connection;
    music::ScopedConnection on_state_changed_connection;
    music::ScopedConnection on_tag_available_connection;
    music::ScopedConnection on_volume_changed_connection;
};

gstreamer::Engine::Engine() : d(new Private{})
{
    d->state = music::Engine::State::ready;
}

gstreamer::Engine::~Engine()
{
    stop();
}

const std::shared_ptr<music::Engine::MetaDataExtractor>& gstreamer::Engine::meta_data_extractor() const
{
    return d->meta_data_extractor;
}

const music::Property<music::Engine::State>& gstreamer::Engine::state() const
{
    return d->state;
}

bool gstreamer::Engine::open_resource_for_uri(const music::Track::UriType& uri)
{
    d->playbin.set_uri(uri);
    return true;
}

bool gstreamer::Engine::play()
{
    auto result = d->playbin.set_state_and_wait(GST_STATE_PLAYING);

    if (result)
    {
        d->state = music::Engine::State::playing;
    }

    return result;
}

bool gstreamer::Engine::stop()
{
    auto result = d->playbin.set_state_and_wait(GST_STATE_NULL);

    if (result)
        d->state = music::Engine::State::stopped;

    return result;
}

bool gstreamer::Engine::pause()
{
    auto result = d->playbin.set_state_and_wait(GST_STATE_PAUSED);

    if (result)
        d->state = music::Engine::State::paused;

    return result;
}

bool gstreamer::Engine::seek_to(const std::chrono::microseconds& ts)
{
    return d->playbin.seek(ts);
}

const com::ubuntu::music::Property<com::ubuntu::music::Engine::Volume>& gstreamer::Engine::volume() const
{
    return d->volume;
}

com::ubuntu::music::Property<com::ubuntu::music::Engine::Volume>& gstreamer::Engine::volume()
{
    return d->volume;
}

const music::Property<std::tuple<music::Track::UriType, music::Track::MetaData>>&
gstreamer::Engine::track_meta_data() const
{
    return d->track_meta_data;
}
