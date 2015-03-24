/*
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

#include "player_implementation.h"
#include "util/timeout.h"

#include <unistd.h>

#include "client_death_observer.h"
#include "engine.h"
#include "null_track_list.h"
#include "track_list_implementation.h"

#include "gstreamer/engine.h"

#include <memory>
#include <exception>
#include <iostream>
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
          engine(std::make_shared<gstreamer::Engine>()),
          track_list(std::make_shared<NullTrackList>()),
          system_wakelock_count(0),
          display_wakelock_count(0),
          previous_state(Engine::State::stopped),
          engine_state_change_connection(engine->state().changed().connect(make_state_change_handler()))
    {
        // Poor man's logging of release/acquire events.
        display_state_lock->acquired().connect([](media::power::DisplayState state)
        {
            std::cout << "Acquired new display state: " << state << std::endl;
        });

        display_state_lock->released().connect([](media::power::DisplayState state)
        {
            std::cout << "Released display state: " << state << std::endl;
        });

        system_state_lock->acquired().connect([](media::power::SystemState state)
        {
            std::cout << "Acquired new system state: "  << state << std::endl;
        });

        system_state_lock->released().connect([](media::power::SystemState state)
        {
            std::cout << "Released system state: "  << state << std::endl;
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
                // We update the track meta data prior to updating the playback status.
                // Some MPRIS clients expect this order of events.
                parent->meta_data_for_current_track().set(std::get<1>(engine->track_meta_data().get()));
                // And update our playback status.
                parent->playback_status().set(media::Player::playing);
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

    void request_power_state()
    {
        try
        {
            if (parent->is_video_source())
            {
                if (++display_wakelock_count == 1)
                {
                    display_state_lock->request_acquire(media::power::DisplayState::on);
                    std::cout << "Requested new display wakelock." << std::endl;
                }
            }
            else
            {
                if (++system_wakelock_count == 1)
                {
                    system_state_lock->request_acquire(media::power::SystemState::active);
                    std::cout << "Requested new system wakelock." << std::endl;
                }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << "Warning: failed to request power state: ";
            std::cerr << e.what() << std::endl;
        }
    }

    void clear_wakelock(const wakelock_clear_t &wakelock)
    {
        cout << __PRETTY_FUNCTION__ << endl;
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
                        std::cout << "Clearing system wakelock." << std::endl;
                        system_state_lock->request_release(media::power::SystemState::active);
                    }
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY:
                    // Only actually clear the display wakelock once the count reaches zero
                    if (--display_wakelock_count == 0)
                    {
                        std::cout << "Clearing display wakelock." << std::endl;
                        display_state_lock->request_release(media::power::DisplayState::on);
                    }
                    break;
                case wakelock_clear_t::WAKELOCK_CLEAR_INVALID:
                default:
                    cerr << "Can't clear invalid wakelock type" << endl;
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << "Warning: failed to clear power state: ";
            std::cerr << e.what() << std::endl;
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

    // Our link back to our parent.
    media::PlayerImplementation<Parent>* parent;
    // We just store the parameters passed on construction.
    media::PlayerImplementation<Parent>::Configuration config;
    media::power::StateController::Lock<media::power::DisplayState>::Ptr display_state_lock;
    media::power::StateController::Lock<media::power::SystemState>::Ptr system_state_lock;

    std::shared_ptr<Engine> engine;
    std::shared_ptr<media::NullTrackList> track_list;
    std::atomic<int> system_wakelock_count;
    std::atomic<int> display_wakelock_count;
    Engine::State previous_state;
    core::Signal<> on_client_disconnected;
    core::Connection engine_state_change_connection;
};

template<typename Parent>
media::PlayerImplementation<Parent>::PlayerImplementation(const media::PlayerImplementation<Parent>::Configuration& config)
    : Parent{config.parent},
      d{std::make_shared<Private>(this, config)}
{
    // Initialize default values for Player interface properties
    Parent::can_play().set(true);
    Parent::can_pause().set(true);
    Parent::can_seek().set(true);
    Parent::can_go_previous().set(true);
    Parent::can_go_next().set(true);
    Parent::is_video_source().set(false);
    Parent::is_audio_source().set(false);
    Parent::is_shuffle().set(true);
    Parent::playback_rate().set(1.f);
    Parent::playback_status().set(Player::PlaybackStatus::null);
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

    d->engine->about_to_finish_signal().connect([this]()
    {
        Parent::about_to_finish()();

        if (d->track_list->has_next())
        {
            Track::UriType uri = d->track_list->query_uri_for_track(d->track_list->next());
            if (!uri.empty())
                d->parent->open_uri(uri);
        }
    });

    d->engine->client_disconnected_signal().connect([this]()
    {
        // If the client disconnects, make sure both wakelock types
        // are cleared
        d->clear_wakelocks();
        // And tell the outside world that the client has gone away
        d->on_client_disconnected();
    });

    d->engine->seeked_to_signal().connect([this](uint64_t value)
    {
        Parent::seeked_to()(value);
    });

    d->engine->end_of_stream_signal().connect([this]()
    {
        Parent::end_of_stream()();
    });

    d->engine->playback_status_changed_signal().connect([this](const Player::PlaybackStatus& status)
    {
        Parent::playback_status_changed()(status);
    });

    d->engine->video_dimension_changed_signal().connect([this](const media::video::Dimensions& dimensions)
    {
        Parent::video_dimension_changed()(dimensions);
    });

    d->engine->error_signal().connect([this](const Player::Error& e)
    {
        Parent::error()(e);
    });

    // Everything is setup, we now subscribe to death notifications.
    std::weak_ptr<Private> wp{d};

    d->config.client_death_observer->register_for_death_notifications_with_key(config.key);
    d->config.client_death_observer->on_client_with_key_died().connect([wp](const media::Player::PlayerKey& died)
    {
        if (auto sp = wp.lock())
        {
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
    return d->engine->open_resource_for_uri(uri);
}

template<typename Parent>
bool media::PlayerImplementation<Parent>::open_uri(const Track::UriType& uri, const Player::HeadersType& headers)
{
    return d->engine->open_resource_for_uri(uri, headers);
}

template<typename Parent>
void media::PlayerImplementation<Parent>::next()
{
}

template<typename Parent>
void media::PlayerImplementation<Parent>::previous()
{
}

template<typename Parent>
void media::PlayerImplementation<Parent>::play()
{
    d->engine->play();
}

template<typename Parent>
void media::PlayerImplementation<Parent>::pause()
{
    d->engine->pause();
}

template<typename Parent>
void media::PlayerImplementation<Parent>::stop()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
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

#include <core/media/player_skeleton.h>

// For linking purposes, we have to make sure that we have all symbols included within the dso.
template class media::PlayerImplementation<media::PlayerSkeleton>;
