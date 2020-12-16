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
 *              Jim Hodapp <jim.hodapp@canonical.com>
 */

#include <core/media/service.h>

#include "player_implementation.h"
#include "service_skeleton.h"
#include "util/timeout.h"

#include <unistd.h>
#include <ctime>

#include "client_death_observer.h"
#include "engine.h"
#include "track_list_implementation.h"
#include "xesam.h"

#include "gstreamer/engine.h"

#include "core/media/logger/logger.h"

#include <memory>
#include <exception>
#include <mutex>

#define UNUSED __attribute__((unused))

namespace media = core::ubuntu::media;
namespace dbus = core::dbus;

using namespace std;

template<typename Parent>
struct media::PlayerImplementation<Parent>::Private :
        public std::enable_shared_from_this<Private>
{
    enum class wakelock_clear_t
    {
        WAKELOCK_CLEAR_INACTIVE,
        WAKELOCK_CLEAR_DISPLAY,
        WAKELOCK_CLEAR_SYSTEM,
        WAKELOCK_CLEAR_INVALID
    };

    Private(PlayerImplementation* parent, const media::PlayerImplementation<Parent>::Configuration& config)
        : parent(parent),
          config(config),
          display_state_lock(config.power_state_controller->display_state_lock()),
          system_state_lock(config.power_state_controller->system_state_lock()),
          engine(std::make_shared<gstreamer::Engine>(config.key)),
          track_list(std::make_shared<TrackListImplementation>(
              config.parent.bus,
              config.parent.service->add_object_for_path(
                  dbus::types::ObjectPath(config.parent.session->path().as_string() + "/TrackList")),
              engine->meta_data_extractor(),
              config.parent.request_context_resolver,
              config.parent.request_authenticator)),
          system_wakelock_count(0),
          display_wakelock_count(0),
          previous_state(Engine::State::stopped),
          engine_state_change_connection(engine->state().changed().connect(make_state_change_handler())),
          engine_playback_status_change_connection(engine->playback_status_changed_signal().connect(make_playback_status_change_handler())),
          doing_abandon(false)
    {
        // Poor man's logging of release/acquire events.
        display_state_lock->acquired().connect([](media::power::DisplayState state)
        {
            MH_INFO("Acquired new display state: %s", state);
        });

        display_state_lock->released().connect([](media::power::DisplayState state)
        {
            MH_INFO("Released display state: %s", state);
        });

        system_state_lock->acquired().connect([](media::power::SystemState state)
        {
            MH_INFO("Acquired new system state: %s", state);
        });

        system_state_lock->released().connect([](media::power::SystemState state)
        {
            MH_INFO("Released system state: %s", state);
        });
    }

    ~Private()
    {
        // Make sure that we don't hold on to the wakelocks if media-hub-server
        // ever gets restarted manually or automatically
        clear_wakelocks();

        // The engine destructor can lead to a stop change state which will
        // trigger the state change handler. Ensure the handler is not called
        // by disconnecting the state change signal
        engine_state_change_connection.disconnect();

        // The engine destructor can lead to a playback status change which will
        // trigger the playback status change handler. Ensure the handler is not called
        // by disconnecting the playback status change signal
        engine_playback_status_change_connection.disconnect();
    }

    std::function<void(const Engine::State& state)> make_state_change_handler()
    {
        /*
         * Wakelock state logic:
         * PLAYING->READY or PLAYING->PAUSED or PLAYING->STOPPED: delay 4 seconds and try to clear current wakelock type
         * ANY STATE->PLAYING: request a new wakelock (system or display)
         */
        return [this](const Engine::State& state)
        {
            MH_DEBUG("Setting state for parent: %s", parent);
            switch(state)
            {
            case Engine::State::ready:
            {
                parent->playback_status().set(media::Player::ready);
                if (previous_state == Engine::State::playing)
                {
                    timeout(4000, true, make_clear_wakelock_functor());
                }
                break;
            }
            case Engine::State::playing:
            {
                // We update the track metadata prior to updating the playback status.
                // Some MPRIS clients expect this order of events.
                time_t now;
                time(&now);
                char buf[sizeof("2011-10-08T07:07:09Z")];
                strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
                media::Track::MetaData metadata{std::get<1>(engine->track_meta_data().get())};
                // Setting this with second resolution makes sure that the track_meta_data property changes
                // and thus the track_meta_data().changed() signal gets sent out over dbus. Otherwise the
                // Property caching mechanism would prevent this.
                metadata.set_last_used(std::string{buf});
                update_mpris_metadata(std::get<0>(engine->track_meta_data().get()), metadata);

                // And update our playback status.
                parent->playback_status().set(media::Player::playing);
                MH_INFO("Requesting power state");
                request_power_state();
                break;
            }
            case Engine::State::stopped:
            {
                parent->playback_status().set(media::Player::stopped);
                if (previous_state ==  Engine::State::playing)
                {
                    timeout(4000, true, make_clear_wakelock_functor());
                }
                break;
            }
            case Engine::State::paused:
            {
                parent->playback_status().set(media::Player::paused);
                if (previous_state == Engine::State::playing)
                {
                    timeout(4000, true, make_clear_wakelock_functor());
                }
                break;
            }
            default:
                break;
            };

            // Keep track of the previous Engine playback state:
            previous_state = state;
        };
    }

    std::function<void(const media::Player::PlaybackStatus& status)> make_playback_status_change_handler()
    {
        return [this](const media::Player::PlaybackStatus& status)
        {
            MH_INFO("Emiting playback_status_changed signal: %s", status);
            parent->emit_playback_status_changed(status);
        };
    }

    void request_power_state()
    {
        MH_TRACE("");
        try
        {
            if (parent->is_video_source())
            {
                if (++display_wakelock_count == 1)
                {
                    MH_INFO("Requesting new display wakelock.");
                    display_state_lock->request_acquire(media::power::DisplayState::on);
                    MH_INFO("Requested new display wakelock.");
                }
            }
            else
            {
                if (++system_wakelock_count == 1)
                {
                    MH_INFO("Requesting new system wakelock.");
                    system_state_lock->request_acquire(media::power::SystemState::active);
                    MH_INFO("Requested new system wakelock.");
                }
            }
        }
        catch(const std::exception& e)
        {
            MH_WARNING("Failed to request power state: %s", e.what());
        }
    }

    void clear_wakelock(const wakelock_clear_t &wakelock)
    {
        MH_TRACE("");
        try
        {
            switch (wakelock)
            {
                case wakelock_clear_t::WAKELOCK_CLEAR_INACTIVE:
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM:
                    // Only actually clear the system wakelock once the count reaches zero
                    if (--system_wakelock_count == 0)
                    {
                        MH_INFO("Clearing system wakelock.");
                        system_state_lock->request_release(media::power::SystemState::active);
                    }
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY:
                    // Only actually clear the display wakelock once the count reaches zero
                    if (--display_wakelock_count == 0)
                    {
                        MH_INFO("Clearing display wakelock.");
                        display_state_lock->request_release(media::power::DisplayState::on);
                    }
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_INVALID:
                default:
                    MH_WARNING("Can't clear invalid wakelock type");
            }
        }
        catch(const std::exception& e)
        {
            MH_WARNING("Failed to request clear power state: %s", e.what());
        }
    }

    wakelock_clear_t current_wakelock_type() const
    {
        return (parent->is_video_source()) ?
            wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY : wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM;
    }

    void clear_wakelocks()
    {
        // Clear both types of wakelocks (display and system)
        if (system_wakelock_count.load() > 0)
        {
            system_wakelock_count = 1;
            clear_wakelock(wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM);
        }
        if (display_wakelock_count.load() > 0)
        {
            display_wakelock_count = 1;
            clear_wakelock(wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY);
        }
    }

    std::function<void()> make_clear_wakelock_functor()
    {
        // Since this functor will be executed on a separate detached thread
        // the execution of the functor may surpass the lifetime of this Private
        // object instance. By keeping a weak_ptr to the private object instance
        // we can check if the object is dead before calling methods on it
        std::weak_ptr<Private> weak_self{this->shared_from_this()};
        auto wakelock_type = current_wakelock_type();
        return [weak_self, wakelock_type] {
            if (auto self = weak_self.lock())
                self->clear_wakelock(wakelock_type);
        };
    }

    void on_client_died()
    {
        engine->reset();
    }

    void open_first_track_from_tracklist(const media::Track::Id& id)
    {
        const Track::UriType uri = track_list->query_uri_for_track(id);
        if (!uri.empty())
        {
            // Using a TrackList for playback, added tracks via add_track(), but open_uri hasn't
            // been called yet to load a media resource
            MH_INFO("Calling d->engine->open_resource_for_uri() for first track added only: %s",
                      uri);
            MH_INFO("\twith a Track::Id: %s", id);
            static const bool do_pipeline_reset = false;
            engine->open_resource_for_uri(uri, do_pipeline_reset);
        }
    }

    void update_mpris_properties()
    {
        const bool has_previous = track_list->has_previous()
                            or parent->Parent::loop_status() != Player::LoopStatus::none;
        const bool has_next = track_list->has_next()
                        or parent->Parent::loop_status() != Player::LoopStatus::none;
        const auto n_tracks = track_list->tracks()->size();
        const bool has_tracks = (n_tracks > 0) ? true : false;

        MH_INFO("Updating MPRIS TrackList properties:");
        MH_INFO("\tTracks: %d", n_tracks);
        MH_INFO("\thas_previous: %d", has_previous);
        MH_INFO("\thas_next: %d", has_next);

        // Update properties
        parent->can_play().set(has_tracks);
        parent->can_pause().set(has_tracks);
        parent->can_go_previous().set(has_previous);
        parent->can_go_next().set(has_next);
    }

    std::string get_uri_for_album_artwork(const media::Track::UriType& uri,
            const media::Track::MetaData& metadata)
    {
        std::string art_uri;
        static const std::string file_uri_prefix{"file://"};
        bool is_local_file = false;
        if (not uri.empty())
            is_local_file = (uri.substr(0, 7) == file_uri_prefix or uri.at(0) == '/');

        // If the track has a full image or preview image or is a video and it is a local file,
        // then use the thumbnailer cache
        if (  (( metadata.count(tags::PreviewImage::name) > 0
                 and metadata.get(tags::PreviewImage::name) == "true")
           or ( metadata.count(tags::Image::name) > 0
                 and metadata.get(tags::Image::name) == "true")
           or parent->is_video_source().get())
           and is_local_file)
        {
            art_uri = "image://thumbnailer/" + uri;
        }
        // If all else fails, display a placeholder icon
        else
        {
            art_uri = "file:///usr/share/icons/suru/apps/scalable/music-app-symbolic.svg";
        }

        return art_uri;
    }

    // Makes sure all relevant metadata fields are set to current data and
    // will trigger the track_meta_data().changed() signal to go out over dbus
    void update_mpris_metadata(const media::Track::UriType& uri, const media::Track::MetaData& md)
    {
        media::Track::MetaData metadata{md};
        if (not metadata.is_set(media::Track::MetaData::TrackIdKey))
        {
            const std::string current_track = track_list->current();
            if (not current_track.empty())
            {
                const std::size_t last_slash = current_track.find_last_of("/");
                const std::string track_id = current_track.substr(last_slash + 1);
                if (not track_id.empty())
                    metadata.set_track_id("/org/mpris/MediaPlayer2/Track/" + track_id);
                else
                    MH_WARNING("Failed to set MPRIS track id since the id value is NULL");
            }
            else
                MH_WARNING("Failed to set MPRIS track id since the id value is NULL");
        }

        if (not metadata.is_set(media::Track::MetaData::TrackArtlUrlKey))
            metadata.set_art_url(get_uri_for_album_artwork(uri, metadata));

        if (not metadata.is_set(media::Track::MetaData::TrackLengthKey))
        {
            // Duration is in nanoseconds, MPRIS spec requires microseconds
            metadata.set_track_length(std::to_string(engine->duration().get() / 1000));
        }

        parent->meta_data_for_current_track().set(metadata);
    }

    bool pause_other_players(media::Player::PlayerKey key)
    {
        if (not config.parent.player_service)
            return false;

        media::ServiceSkeleton* skel {
            reinterpret_cast<media::ServiceSkeleton*>(config.parent.player_service)
        };
        skel->pause_other_sessions(key);
        return true;
    }

    bool update_current_player(media::Player::PlayerKey key)
    {
        if (not config.parent.player_service)
            return false;

        media::ServiceSkeleton* skel {
            reinterpret_cast<media::ServiceSkeleton*>(config.parent.player_service)
        };
        skel->set_current_player(key);
        return true;
    }

    bool is_current_player() const
    {
        if (not config.parent.player_service)
            return false;

        media::ServiceSkeleton* skel {
            reinterpret_cast<media::ServiceSkeleton*>(config.parent.player_service)
        };
        return skel->is_current_player(parent->key());
    }

    bool is_multimedia_role() const
    {
        return parent->audio_stream_role() == media::Player::AudioStreamRole::multimedia;
    }

    bool reset_current_player()
    {
        if (not config.parent.player_service)
            return false;

        media::ServiceSkeleton* skel {
            reinterpret_cast<media::ServiceSkeleton*>(config.parent.player_service)
        };
        skel->reset_current_player();
        return true;
    }

    // Our link back to our parent.
    media::PlayerImplementation<Parent>* parent;
    // We just store the parameters passed on construction.
    media::PlayerImplementation<Parent>::Configuration config;
    media::power::StateController::Lock<media::power::DisplayState>::Ptr display_state_lock;
    media::power::StateController::Lock<media::power::SystemState>::Ptr system_state_lock;

    std::shared_ptr<Engine> engine;
    std::shared_ptr<media::TrackListImplementation> track_list;
    std::atomic<int> system_wakelock_count;
    std::atomic<int> display_wakelock_count;
    Engine::State previous_state;
    core::Signal<> on_client_disconnected;
    core::Connection engine_state_change_connection;
    core::Connection engine_playback_status_change_connection;
    // Prevent the TrackList from auto advancing to the next track
    std::mutex doing_go_to_track;
    std::atomic<bool> doing_abandon;
};

template<typename Parent>
media::PlayerImplementation<Parent>::PlayerImplementation(const media::PlayerImplementation<Parent>::Configuration& config)
    : Parent{config.parent},
      d{std::make_shared<Private>(this, config)}
{
    // Initialize default values for Player interface properties
    Parent::can_play().set(false);
    Parent::can_pause().set(false);
    Parent::can_seek().set(true);
    Parent::can_go_previous().set(false);
    Parent::can_go_next().set(false);
    Parent::is_video_source().set(false);
    Parent::is_audio_source().set(false);
    Parent::shuffle().set(false);
    Parent::playback_rate().set(1.f);
    Parent::playback_status().set(Player::PlaybackStatus::null);
    Parent::backend().set(media::AVBackend::get_backend_type());
    Parent::loop_status().set(Player::LoopStatus::none);
    Parent::position().set(0);
    Parent::duration().set(0);
    Parent::audio_stream_role().set(Player::AudioStreamRole::multimedia);
    d->engine->audio_stream_role().set(Player::AudioStreamRole::multimedia);
    Parent::orientation().set(Player::Orientation::rotate0);
    Parent::lifetime().set(Player::Lifetime::normal);
    d->engine->lifetime().set(Player::Lifetime::normal);

    // Make sure that the Position property gets updated from the Engine
    // every time the client requests position
    std::function<uint64_t()> position_getter = [this]()
    {
        return d->engine->position().get();
    };
    Parent::position().install(position_getter);

    d->engine->position().changed().connect([this](uint64_t position)
    {
        d->track_list->on_position_changed(position);
    });

    // Make sure that the Duration property gets updated from the Engine
    // every time the client requests duration
    std::function<uint64_t()> duration_getter = [this]()
    {
        return d->engine->duration().get();
    };
    Parent::duration().install(duration_getter);

    std::function<bool()> video_type_getter = [this]()
    {
        return d->engine->is_video_source().get();
    };
    Parent::is_video_source().install(video_type_getter);

    std::function<bool()> audio_type_getter = [this]()
    {
        return d->engine->is_audio_source().get();
    };
    Parent::is_audio_source().install(audio_type_getter);

    std::function<bool()> can_go_next_getter = [this]()
    {
        // If LoopStatus == playlist, then there is always a next track
        return d->track_list->has_next() or Parent::loop_status() != Player::LoopStatus::none;
    };
    Parent::can_go_next().install(can_go_next_getter);

    std::function<bool()> can_go_previous_getter = [this]()
    {
        // If LoopStatus == playlist, then there is always a next previous
        return d->track_list->has_previous() or Parent::loop_status() != Player::LoopStatus::none;
    };
    Parent::can_go_previous().install(can_go_previous_getter);

    // When the client changes the loop status, make sure to update the TrackList
    Parent::loop_status().changed().connect([this](media::Player::LoopStatus loop_status)
    {
        MH_INFO("LoopStatus: %s", loop_status);
        d->track_list->on_loop_status_changed(loop_status);
    });

    // When the client changes the shuffle setting, make sure to update the TrackList
    Parent::shuffle().changed().connect([this](bool shuffle)
    {
        d->track_list->on_shuffle_changed(shuffle);
    });

    // Make sure that the audio_stream_role property gets updated on the Engine side
    // whenever the client side sets the role
    Parent::audio_stream_role().changed().connect([this](media::Player::AudioStreamRole new_role)
    {
        d->engine->audio_stream_role().set(new_role);
    });

    // When the value of the orientation Property is changed in the Engine by playbin,
    // update the Player's cached value
    d->engine->orientation().changed().connect([this](const Player::Orientation& o)
    {
        Parent::orientation().set(o);
    });

    Parent::lifetime().changed().connect([this](media::Player::Lifetime lifetime)
    {
        d->engine->lifetime().set(lifetime);
    });

    d->engine->track_meta_data().changed().connect([this, config](
           const std::tuple<media::Track::UriType, media::Track::MetaData>& md)
    {
        d->update_mpris_metadata(std::get<0>(md), std::get<1>(md));
    });

    d->engine->about_to_finish_signal().connect([this]()
    {
        if (d->doing_abandon)
            return;

        // Prevent on_go_to_track from executing as it's not needed in this case. on_go_to_track
        // (see the lambda below) is only needed when the client explicitly calls next() not during
        // the about_to_finish condition
        d->doing_go_to_track.lock();

        Parent::about_to_finish()();

        const media::Track::Id prev_track_id = d->track_list->current();
        // Make sure that the TrackList keeps advancing. The logic for what gets played next,
        // if anything at all, occurs in TrackListSkeleton::next()
        const Track::UriType uri = d->track_list->query_uri_for_track(d->track_list->next());
        if (prev_track_id != d->track_list->current() && !uri.empty())
        {
            MH_INFO("Advancing to next track on playbin: %s", uri);
            static const bool do_pipeline_reset = false;
            d->engine->open_resource_for_uri(uri, do_pipeline_reset);

            // Force the 'playing' state.
            MH_INFO("Forcing engine 'playing' state");
            d->engine->play();
        }

        d->doing_go_to_track.unlock();
    });

    d->engine->client_disconnected_signal().connect([this]()
    {
        // If the client disconnects, make sure both wakelock types
        // are cleared
        d->clear_wakelocks();
        d->track_list->reset();

        // This is not a fatal error but merely a warning that should
        // be logged
        if (d->is_multimedia_role() and d->is_current_player())
        {
            MH_DEBUG("==== Resetting current player");
            if (not d->reset_current_player())
                MH_WARNING("Failed to reset current player");
        }

        // And tell the outside world that the client has gone away
        d->on_client_disconnected();
    });

    d->engine->seeked_to_signal().connect([this](uint64_t value)
    {
        Parent::seeked_to()(value);
    });

    d->engine->on_buffering_changed_signal().connect([this](int value)
    {
        Parent::buffering_changed()(value);
    });

    d->engine->end_of_stream_signal().connect([this]()
    {
        Parent::end_of_stream()();
    });

    d->engine->video_dimension_changed_signal().connect([this](const media::video::Dimensions& dimensions)
    {
        Parent::video_dimension_changed()(dimensions);
    });

    d->engine->error_signal().connect([this](const Player::Error& e)
    {
        Parent::error()(e);
    });

    d->track_list->on_end_of_tracklist().connect([this]()
    {
        if (d->engine->state() != gstreamer::Engine::State::ready
                && d->engine->state() != gstreamer::Engine::State::stopped)
        {
            MH_INFO("End of tracklist reached, stopping playback");
            const constexpr bool use_main_thread = true;
            d->engine->stop(use_main_thread);
        }
    });

    d->track_list->on_go_to_track().connect([this](const media::Track::Id& id)
    {
        // This lambda needs to be mutually exclusive with the about_to_finish lambda above
        const bool locked = d->doing_go_to_track.try_lock();
        // If the try_lock fails, it means that about_to_finish lambda above has it locked and it will
        // call d->engine->open_resource_for_uri()
        if (!locked)
            return;

        // Store whether we should restore the current playing state after loading the new uri
        const bool auto_play = Parent::playback_status().get() == media::Player::playing;

        const Track::UriType uri = d->track_list->query_uri_for_track(id);
        if (!uri.empty())
        {
            MH_INFO("Setting next track on playbin (on_go_to_track signal): %s", uri);
            MH_INFO("\twith a Track::Id: %s", id);
            static const bool do_pipeline_reset = true;
            d->engine->open_resource_for_uri(uri, do_pipeline_reset);
        }

        if (auto_play)
        {
            MH_DEBUG("Restoring playing state");
            d->engine->play();
        }

        d->doing_go_to_track.unlock();
    });

    d->track_list->on_track_added().connect([this](const media::Track::Id& id)
    {
        MH_TRACE("** Track was added, handling in PlayerImplementation");
        if (d->track_list->tracks()->size() == 1)
            d->open_first_track_from_tracklist(id);

        d->update_mpris_properties();
    });

    d->track_list->on_tracks_added().connect([this](const media::TrackList::ContainerURI& tracks)
    {
        MH_TRACE("** Track was added, handling in PlayerImplementation");
        // If the two sizes are the same, that means the TrackList was previously empty and we need
        // to open the first track in the TrackList so that is_audio_source() and is_video_source()
        // will function correctly.
        if (tracks.size() >= 1 and d->track_list->tracks()->size() == tracks.size())
            d->open_first_track_from_tracklist(tracks.front());

        d->update_mpris_properties();
    });

    d->track_list->on_track_removed().connect([this](const media::Track::Id&)
    {
        d->update_mpris_properties();
    });

    d->track_list->on_track_list_reset().connect([this](void)
    {
        d->update_mpris_properties();
    });

    d->track_list->on_track_changed().connect([this](const media::Track::Id&)
    {
        d->update_mpris_properties();
    });

    d->track_list->on_track_list_replaced().connect(
        [this](const media::TrackList::ContainerTrackIdTuple&)
        {
            d->update_mpris_properties();
        }
    );

    // Everything is setup, we now subscribe to death notifications.
    std::weak_ptr<Private> wp{d};

    d->config.client_death_observer->register_for_death_notifications_with_key(config.key);
    d->config.client_death_observer->on_client_with_key_died().connect([wp](const media::Player::PlayerKey& died)
    {
        if (auto sp = wp.lock())
        {
            if (sp->doing_abandon)
                return;

            if (died != sp->config.key)
                return;

            static const std::chrono::milliseconds timeout{1000};
            media::timeout(timeout.count(), true, [wp]()
            {
                if (auto sp = wp.lock())
                    sp->on_client_died();
            });
        }
    });
}

template<typename Parent>
media::PlayerImplementation<Parent>::~PlayerImplementation()
{
    // Install null getters as these properties may be destroyed
    // after the engine has been destroyed since they are owned by the
    // base class.
    std::function<uint64_t()> position_getter = [this]()
    {
        return static_cast<uint64_t>(0);
    };
    Parent::position().install(position_getter);

    std::function<uint64_t()> duration_getter = [this]()
    {
        return static_cast<uint64_t>(0);
    };
    Parent::duration().install(duration_getter);

    std::function<bool()> video_type_getter = [this]()
    {
        return false;
    };
    Parent::is_video_source().install(video_type_getter);

    std::function<bool()> audio_type_getter = [this]()
    {
        return false;
    };
    Parent::is_audio_source().install(audio_type_getter);
}

template<typename Parent>
std::string media::PlayerImplementation<Parent>::uuid() const
{
    // No impl for now, as not needed internally.
    return std::string{};
}

template<typename Parent>
void media::PlayerImplementation<Parent>::reconnect()
{
    d->config.client_death_observer->register_for_death_notifications_with_key(d->config.key);
}

template<typename Parent>
void media::PlayerImplementation<Parent>::abandon()
{
    // Signal client disconnection due to abandonment of player
    d->doing_abandon = true;
    d->on_client_died();
}

template<typename Parent>
std::shared_ptr<media::TrackList> media::PlayerImplementation<Parent>::track_list()
{
    return d->track_list;
}

// TODO: Convert this to be a property instead of sync call
template<typename Parent>
media::Player::PlayerKey media::PlayerImplementation<Parent>::key() const
{
    return d->config.key;
}

template<typename Parent>
media::video::Sink::Ptr media::PlayerImplementation<Parent>::create_gl_texture_video_sink(std::uint32_t texture_id)
{
    d->engine->create_video_sink(texture_id);
    return media::video::Sink::Ptr{};
}

template<typename Parent>
bool media::PlayerImplementation<Parent>::open_uri(const Track::UriType& uri)
{
    d->track_list->reset();

    // If empty uri, give the same meaning as QMediaPlayer::setMedia("")
    if (uri.empty())
    {
        MH_DEBUG("Resetting current media");
        return true;
    }

    static const bool do_pipeline_reset = false;
    const bool ret = d->engine->open_resource_for_uri(uri, do_pipeline_reset);
    // Don't set new track as the current track to play since we're calling open_resource_for_uri above
    static const bool make_current = false;
    d->track_list->add_track_with_uri_at(uri, media::TrackList::after_empty_track(), make_current);

    return ret;
}

template<typename Parent>
bool media::PlayerImplementation<Parent>::open_uri(const Track::UriType& uri, const Player::HeadersType& headers)
{
    return d->engine->open_resource_for_uri(uri, headers);
}

template<typename Parent>
void media::PlayerImplementation<Parent>::next()
{
    d->track_list->next();
}

template<typename Parent>
void media::PlayerImplementation<Parent>::previous()
{
    d->track_list->previous();
}

template<typename Parent>
void media::PlayerImplementation<Parent>::play()
{
    MH_TRACE("");
    if (d->is_multimedia_role())
    {
        MH_DEBUG("==== Pausing all other multimedia player sessions");
        if (not d->pause_other_players(d->config.key))
            MH_WARNING("Failed to pause other player sessions");

        MH_DEBUG("==== Updating the current player");
        // This player will begin playing so make sure it's the current player. If
        // this operation fails it is not a fatal condition but should be logged
        if (not d->update_current_player(d->config.key))
            MH_WARNING("Failed to update current player");
    }

    d->engine->play();
}

template<typename Parent>
void media::PlayerImplementation<Parent>::pause()
{
    MH_TRACE("");
    d->engine->pause();
}

template<typename Parent>
void media::PlayerImplementation<Parent>::stop()
{
    MH_TRACE("");
    d->engine->stop();
}

template<typename Parent>
void media::PlayerImplementation<Parent>::seek_to(const std::chrono::microseconds& ms)
{
    d->engine->seek_to(ms);
}

template<typename Parent>
const core::Signal<>& media::PlayerImplementation<Parent>::on_client_disconnected() const
{
    return d->on_client_disconnected;
}

template<typename Parent>
void media::PlayerImplementation<Parent>::emit_playback_status_changed(const media::Player::PlaybackStatus &status)
{
    Parent::playback_status_changed()(status);
}

#include <core/media/player_skeleton.h>

// For linking purposes, we have to make sure that we have all symbols included within the dso.
template class media::PlayerImplementation<media::PlayerSkeleton>;
