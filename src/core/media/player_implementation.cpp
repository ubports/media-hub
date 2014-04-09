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
 */

#include "player_implementation.h"

#include <unistd.h>

#include "engine.h"
#include "track_list_implementation.h"

#include "powerd_service.h"

#define UNUSED __attribute__((unused))

namespace media = core::ubuntu::media;
namespace dbus = core::dbus;

struct media::PlayerImplementation::Private
{
    Private(PlayerImplementation* parent,
            const dbus::types::ObjectPath& session_path,
            const std::shared_ptr<media::Service>& service,
            const std::shared_ptr<media::Engine>& engine)
        : parent(parent),
          service(service),
          engine(engine),
          session_path(session_path),
          track_list(
              new media::TrackListImplementation(
                  session_path.as_string() + "/TrackList",
                  engine->meta_data_extractor()))

    {
        auto bus = std::shared_ptr<dbus::Bus>(new dbus::Bus(core::dbus::WellKnownBus::system));
        bus->install_executor(dbus::asio::make_executor(bus));

        auto stub_service = dbus::Service::use_service(bus, dbus::traits::Service<core::Powerd>::interface_name());
        powerd_session = stub_service->object_for_path(dbus::types::ObjectPath("/com/canonical/powerd"));

        engine->state().changed().connect(
                    [parent, this](const Engine::State& state)
        {
            switch(state)
            {
            case Engine::State::ready:
            {
                printf("%s():%d new state: READY\n", __func__, __LINE__);
                parent->playback_status().set(media::Player::ready);
                if (!lock_cookie.empty())
                    powerd_session->invoke_method_synchronously<core::Powerd::clearSysState, void>(lock_cookie);
                break;
            }
            case Engine::State::playing:
            {
                printf("%s():%d new state: PLAYING\n", __func__, __LINE__);
                parent->playback_status().set(media::Player::playing);
                if (lock_cookie.empty())
                {
                    auto result = powerd_session->invoke_method_synchronously<core::Powerd::requestSysState, std::string>(lock_name, 1);
                    if (result.is_error())
                        throw std::runtime_error(result.error().print());

                    lock_cookie = result.value();
                }
                break;
            }
            case Engine::State::stopped:
            {
                printf("%s():%d new state: STOPPED\n", __func__, __LINE__);
                parent->playback_status().set(media::Player::stopped);
                if (!lock_cookie.empty())
                    powerd_session->invoke_method_synchronously<core::Powerd::clearSysState, void>(lock_cookie);
                break;
            }
            case Engine::State::paused:
            {
                printf("%s():%d new state: PAUSED\n", __func__, __LINE__);
                parent->playback_status().set(media::Player::paused);
                if (!lock_cookie.empty())
                    powerd_session->invoke_method_synchronously<core::Powerd::clearSysState, void>(lock_cookie);
                break;
            }
            default:
                break;
            };
        });

    }

    PlayerImplementation* parent;
    std::shared_ptr<Service> service;
    std::shared_ptr<Engine> engine;
    dbus::types::ObjectPath session_path;
    std::shared_ptr<TrackListImplementation> track_list;
    std::shared_ptr<dbus::Object> powerd_session;
    std::string lock_name;
    std::string lock_cookie;
};

media::PlayerImplementation::PlayerImplementation(
        const dbus::types::ObjectPath& session_path,
        const std::shared_ptr<Service>& service,
        const std::shared_ptr<Engine>& engine)
    : media::PlayerSkeleton(session_path),
      d(new Private(
            this,
            session_path,
            service,
            engine))
{
    // Initializing default values for properties
    can_play().set(true);
    can_pause().set(true);
    can_seek().set(true);
    can_go_previous().set(true);
    can_go_next().set(true);
    is_video_source().set(false);
    is_audio_source().set(false);
    is_shuffle().set(true);
    playback_rate().set(1.f);
    playback_status().set(Player::PlaybackStatus::null);
    loop_status().set(Player::LoopStatus::none);
    position().set(0);
    duration().set(0);

    // Make sure that the Position property gets updated from the Engine
    // every time the client requests position
    std::function<uint64_t()> position_getter = [this]()
    {
        return d->engine->position().get();
    };
    position().install(position_getter);

    // Make sure that the Duration property gets updated from the Engine
    // every time the client requests duration
    std::function<uint64_t()> duration_getter = [this]()
    {
        return d->engine->duration().get();
    };
    duration().install(duration_getter);

    std::function<bool()> video_type_getter = [this]()
    {
        return d->engine->is_video_source().get();
    };
    is_video_source().install(video_type_getter);

    std::function<bool()> audio_type_getter = [this]()
    {
        return d->engine->is_audio_source().get();
    };
    is_audio_source().install(audio_type_getter);

    engine->about_to_finish_signal().connect([this]()
    {
        if (d->track_list->has_next())
        {
            Track::UriType uri = d->track_list->query_uri_for_track(d->track_list->next());
            if (!uri.empty())
                d->parent->open_uri(uri);
        }
    });

    engine->end_of_stream_signal().connect([this]()
    {
        end_of_stream()();
    });
}

media::PlayerImplementation::~PlayerImplementation()
{
}

std::shared_ptr<media::TrackList> media::PlayerImplementation::track_list()
{
    return d->track_list;
}

bool media::PlayerImplementation::open_uri(const Track::UriType& uri)
{
    return d->engine->open_resource_for_uri(uri);
}

void media::PlayerImplementation::create_video_sink(uint32_t texture_id)
{
    d->engine->create_video_sink(texture_id);
}

media::Player::GLConsumerWrapperHybris media::PlayerImplementation::gl_consumer() const
{
    // This method is client-side only and is simply a no-op for the service side
    return NULL;
}

void media::PlayerImplementation::next()
{
}

void media::PlayerImplementation::previous()
{
}

void media::PlayerImplementation::play()
{
    d->engine->play();
}

void media::PlayerImplementation::pause()
{
    d->engine->pause();
}

void media::PlayerImplementation::stop()
{
    d->engine->stop();
}

void media::PlayerImplementation::set_frame_available_callback(
    UNUSED FrameAvailableCb cb, UNUSED void *context)
{
    // This method is client-side only and is simply a no-op for the service side
}

void media::PlayerImplementation::set_playback_complete_callback(
    UNUSED PlaybackCompleteCb cb, UNUSED void *context)
{
    // This method is client-side only and is simply a no-op for the service side
}

void media::PlayerImplementation::seek_to(const std::chrono::microseconds& ms)
{
    d->engine->seek_to(ms);
}
