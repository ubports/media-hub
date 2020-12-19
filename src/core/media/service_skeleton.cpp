/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "service_skeleton.h"

#include "mpris/media_player2.h"
#include "mpris/metadata.h"
#include "mpris/player.h"
#include "mpris/playlists.h"
#include "mpris/service.h"

#include "player_configuration.h"
#include "the_session_bus.h"
#include "xesam.h"

#include "core/media/logger/logger.h"

#include <core/dbus/message.h>
#include <core/dbus/object.h>
#include <core/dbus/types/object_path.h>

#include <core/posix/this_process.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <map>
#include <regex>
#include <sstream>

namespace dbus = core::dbus;
namespace media = core::ubuntu::media;

using namespace std;

namespace
{
core::Signal<void> the_empty_signal;
}

struct media::ServiceSkeleton::Private
{
    Private(media::ServiceSkeleton* impl, const ServiceSkeleton::Configuration& config)
        : request_context_resolver(media::apparmor::ubuntu::make_platform_default_request_context_resolver(config.external_services)),
          impl(impl),
          object(impl->access_service()->add_object_for_path(
                     dbus::traits::Service<media::Service>::object_path())),
          configuration(config),
          exported(impl->access_bus(), config.cover_art_resolver, impl, configuration)
    {
        object->install_method_handler<mpris::Service::CreateSession>(
                    std::bind(
                        &Private::handle_create_session,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::DetachSession>(
                    std::bind(
                        &Private::handle_detach_session,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::ReattachSession>(
                    std::bind(
                        &Private::handle_reattach_session,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::DestroySession>(
                    std::bind(
                        &Private::handle_destroy_session,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::CreateFixedSession>(
                    std::bind(
                        &Private::handle_create_fixed_session,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::ResumeSession>(
                    std::bind(
                        &Private::handle_resume_session,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::PauseOtherSessions>(
                    std::bind(
                        &Private::handle_pause_other_sessions,
                        this,
                        std::placeholders::_1));
        object->install_method_handler<mpris::Service::EqualizerSetBand>(
                    std::bind(
                        &Private::handle_equalizer_set_band,
                        this,
                        std::placeholders::_1));
    }

    std::tuple<std::string, media::Player::PlayerKey, std::string> create_session_info()
    {
        static unsigned int session_counter = 0;

        const unsigned int current_session = session_counter++;
        boost::uuids::uuid uuid = gen();

        std::stringstream ss;
        ss << "/core/ubuntu/media/Service/sessions/" << current_session;

        return std::make_tuple(ss.str(), media::Player::PlayerKey(current_session), to_string(uuid));
    }

    void handle_create_session(const core::dbus::Message::Ptr& msg)
    {
        auto session_info = create_session_info();

        dbus::types::ObjectPath op{std::get<0>(session_info)};
        media::Player::PlayerKey key{std::get<1>(session_info)};
        std::string uuid{std::get<2>(session_info)};

        media::Player::Configuration config
        {
            key,
            impl->access_bus(),
            impl->access_service(),
            impl->access_service()->add_object_for_path(op),
            impl
        };

        MH_DEBUG("Session created by request of: %s, key: %d, uuid: %d, path: %s",
                msg->sender(), key, uuid, op);

        try
        {
            const std::shared_ptr<media::Player> player {impl->create_session(config)};
            configuration.player_store->add_player_for_key(key, player);
            uuid_player_map.emplace(std::make_pair(uuid, key));

            request_context_resolver->resolve_context_for_dbus_name_async(msg->sender(),
                    [this, key, msg](const media::apparmor::ubuntu::Context& context)
            {
                MH_DEBUG(" -- app_name='%s', attached", context.str());
                player_owner_map.emplace(std::make_pair(key, std::make_tuple(context.str(), true, msg->sender())));
            });

            auto reply = dbus::Message::make_method_return(msg);
            reply->writer() << std::make_tuple(op, uuid);

            impl->access_bus()->send(reply);
        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::CreatingSession::name(),
                        e.what());
            impl->access_bus()->send(reply);
        }
    }

    void handle_detach_session(const core::dbus::Message::Ptr& msg)
    {
        try
        {
            std::string uuid;
            msg->reader() >> uuid;

            // Make sure we don't try to do a lookup if the map is empty
            if (!uuid_player_map.empty())
            {
                const auto key = uuid_player_map.at(uuid);

                if (player_owner_map.count(key) != 0) {
                    auto info = player_owner_map.at(key);
                    // Check if session is attached(1) and that the detachment
                    // request comes from the same peer(2) that created the session.
                    if (std::get<1>(info) && (std::get<2>(info) == msg->sender())) { // Player is attached
                        std::get<1>(info) = false; // Detached
                        std::get<2>(info).clear(); // Clear registered sender/peer

                        auto player = configuration.player_store->player_for_key(key);
                        player->lifetime().set(media::Player::Lifetime::resumable);
                    }
                }
            }

            auto reply = dbus::Message::make_method_return(msg);
            impl->access_bus()->send(reply);

        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::DetachingSession::name(),
                        e.what());
            impl->access_bus()->send(reply);
        }
    }

    void handle_reattach_session(const core::dbus::Message::Ptr& msg)
    {
        try
        {
            std::string uuid;
            msg->reader() >> uuid;

            if (uuid_player_map.count(uuid) != 0)
            {
                const auto key = uuid_player_map.at(uuid);
                if (not configuration.player_store->has_player_for_key(key))
                {
                    auto reply = dbus::Message::make_error(
                                msg,
                                mpris::Service::Errors::ReattachingSession::name(),
                                "Unable to locate player session");
                    impl->access_bus()->send(reply);
                    return;
                }
                std::stringstream ss;
                ss << "/core/ubuntu/media/Service/sessions/" << key;
                dbus::types::ObjectPath op{ss.str()};

                request_context_resolver->resolve_context_for_dbus_name_async(msg->sender(),
                        [this, msg, key, op](const media::apparmor::ubuntu::Context& context)
                {
                    auto info = player_owner_map.at(key);
                    MH_DEBUG(" -- reattach app_name='%s', info='%s', '%s'",
                            context.str(), std::get<0>(info), std::get<2>(info));
                    if (std::get<0>(info) == context.str()) {
                        std::get<1>(info) = true; // Set to Attached
                        std::get<2>(info) = msg->sender(); // Register new owner

                        // Signal player reconnection
                        auto player = configuration.player_store->player_for_key(key);
                        player->reconnect();

                        auto reply = dbus::Message::make_method_return(msg);
                        reply->writer() << op;

                        impl->access_bus()->send(reply);
                    }
                    else {
                        auto reply = dbus::Message::make_error(
                                    msg,
                                    mpris::Service::Errors::ReattachingSession::name(),
                                    "Invalid permissions for the requested session");
                        impl->access_bus()->send(reply);
                        return;
                    }
                });
            }
            else {
               auto reply = dbus::Message::make_error(
                           msg,
                           mpris::Service::Errors::ReattachingSession::name(),
                           "Invalid session");
               impl->access_bus()->send(reply);
               return;
            }
        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::ReattachingSession::name(),
                        e.what());
            impl->access_bus()->send(reply);
        }
    }

    void handle_destroy_session(const core::dbus::Message::Ptr& msg)
    {
        try
        {
            std::string uuid;
            msg->reader() >> uuid;

            if (uuid_player_map.count(uuid) != 0) {
                const auto key = uuid_player_map.at(uuid);
                if (not configuration.player_store->has_player_for_key(key)) {
                    auto reply = dbus::Message::make_error(
                                msg,
                                mpris::Service::Errors::DestroyingSession::name(),
                                "Unable to locate player session");
                    impl->access_bus()->send(reply);
                    return;
                }

                // Remove control entries from the map, at this point
                // the session is no longer usable.
                uuid_player_map.erase(uuid);

                request_context_resolver->resolve_context_for_dbus_name_async(msg->sender(),
                        [this, msg, key](const media::apparmor::ubuntu::Context& context)
                {
                    auto info = player_owner_map.at(key);
                    MH_DEBUG(" -- Destroying app_name='%s', info='%s', '%s'",
                            context.str(), std::get<0>(info), std::get<2>(info));
                    if (std::get<0>(info) == context.str()) {
                        player_owner_map.erase(key);

                        // Reset lifecycle to non-resumable on the now-abandoned session
                        auto player = configuration.player_store->player_for_key(key);

                        // Delete player instance by abandonment
                        player->lifetime().set(media::Player::Lifetime::normal);
                        player->abandon();

                        auto reply = dbus::Message::make_method_return(msg);
                        impl->access_bus()->send(reply);
                    }
                    else {
                        auto reply = dbus::Message::make_error(
                                    msg,
                                    mpris::Service::Errors::DestroyingSession::name(),
                                    "Invalid permissions for the requested session");
                        impl->access_bus()->send(reply);
                        return;
                    }
                });
            }
            else {
               auto reply = dbus::Message::make_error(
                           msg,
                           mpris::Service::Errors::DestroyingSession::name(),
                           "Invalid session");
               impl->access_bus()->send(reply);
               return;
            }
        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::DestroyingSession::name(),
                        e.what());
            impl->access_bus()->send(reply);
        }
    }

    void handle_create_fixed_session(const core::dbus::Message::Ptr& msg)
    {
        try
        {
            std::string name;
            msg->reader() >> name;

            if (named_player_map.count(name) == 0) {
                // Create new session
                auto session_info = create_session_info();

                dbus::types::ObjectPath op{std::get<0>(session_info)};
                media::Player::PlayerKey key{std::get<1>(session_info)};

                media::Player::Configuration config
                {
                    key,
                    impl->access_bus(),
                    impl->access_service(),
                    impl->access_service()->add_object_for_path(op),
                    impl
                };

                auto session = impl->create_session(config);
                session->lifetime().set(media::Player::Lifetime::resumable);

                configuration.player_store->add_player_for_key(key, session);

                named_player_map.insert(std::make_pair(name, key));

                auto reply = dbus::Message::make_method_return(msg);
                reply->writer() << op;

                impl->access_bus()->send(reply);
            }
            else {
                // Resume previous session
                const auto key = named_player_map.at(name);
                if (not configuration.player_store->has_player_for_key(key)) {
                    auto reply = dbus::Message::make_error(
                                msg,
                                mpris::Service::Errors::CreatingFixedSession::name(),
                                "Unable to locate player session");
                    impl->access_bus()->send(reply);
                    return;
                }

                std::stringstream ss;
                ss << "/core/ubuntu/media/Service/sessions/" << key;
                dbus::types::ObjectPath op{ss.str()};

                auto reply = dbus::Message::make_method_return(msg);
                reply->writer() << op;

                impl->access_bus()->send(reply);
            }
        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::CreatingSession::name(),
                        e.what());
            impl->access_bus()->send(reply);
        }
    }

    void handle_resume_session(const core::dbus::Message::Ptr& msg)
    {
        try
        {
            Player::PlayerKey key;
            msg->reader() >> key;

            if (not configuration.player_store->has_player_for_key(key)) {
                auto reply = dbus::Message::make_error(
                            msg,
                            mpris::Service::Errors::ResumingSession::name(),
                            "Unable to locate player session");
                impl->access_bus()->send(reply);
                return;
            }

            std::stringstream ss;
            ss << "/core/ubuntu/media/Service/sessions/" << key;
            dbus::types::ObjectPath op{ss.str()};

            auto reply = dbus::Message::make_method_return(msg);
            reply->writer() << op;

            impl->access_bus()->send(reply);
        } catch(const std::runtime_error& e)
        {
            auto reply = dbus::Message::make_error(
                        msg,
                        mpris::Service::Errors::CreatingSession::name(),
                        e.what());
            impl->access_bus()->send(reply);
        }
    }

    void handle_set_current_player(const core::dbus::Message::Ptr& msg)
    {
        Player::PlayerKey key;
        msg->reader() >> key;

        core::dbus::Message::Ptr reply;
        if (not configuration.player_store->has_player_for_key(key))
        {
            MH_WARNING("Player key not found: %d", key);
            reply = dbus::Message::make_error(
                            msg,
                            mpris::Service::Errors::PlayerKeyNotFound::name(),
                            "Player key not found");
        }
        else
        {
            try {
                impl->set_current_player(key);
                reply = dbus::Message::make_method_return(msg);
            }
            catch (const std::out_of_range &e) {
                MH_WARNING("Failed to look up Player instance for key %d\
                    , no valid Player instance for that key value and cannot set current player.\
                    This most likely means that media-hub-server has crashed and restarted.", key);
                reply = dbus::Message::make_error(
                            msg,
                            mpris::Service::Errors::PlayerKeyNotFound::name(),
                            "Player key not found");
            }
        }

        impl->access_bus()->send(reply);
    }

    void handle_pause_other_sessions(const core::dbus::Message::Ptr& msg)
    {
        Player::PlayerKey key;
        msg->reader() >> key;
        core::dbus::Message::Ptr reply;
        try {
            impl->pause_other_sessions(key);
            reply = dbus::Message::make_method_return(msg);
        }
        catch (const std::out_of_range &e) {
                MH_WARNING("Failed to look up Player instance for key %d\
                    , no valid Player instance for that key value and cannot set current player.\
                    This most likely means that media-hub-server has crashed and restarted.", key);
            reply = dbus::Message::make_error(
                    msg,
                    mpris::Service::Errors::PlayerKeyNotFound::name(),
                    "Player key not found");
        }

        impl->access_bus()->send(reply);
    }

    void handle_equalizer_set_band(const core::dbus::Message::Ptr& msg) {

        core::dbus::Message::Ptr reply;
        try {
            int band = 0;    //   0 .. 9
            double gain = 0; // -24 .. 24

            msg->reader() >> band >> gain;

            impl->equalizer_set_band(band, gain);
            reply = dbus::Message::make_method_return(msg);

        } catch(const std::runtime_error& e) {
            reply = dbus::Message::make_error(
                    msg,
                    mpris::Service::Errors::EqualizerSetBand::name(),
                    e.what());
        }

        impl->access_bus()->send(reply);
    }

    media::apparmor::ubuntu::RequestContextResolver::Ptr request_context_resolver;
    media::ServiceSkeleton* impl;
    dbus::Object::Ptr object;

    // We remember all our creation time arguments.
    ServiceSkeleton::Configuration configuration;
    // We map named/fixed player instances to their respective keys.
    std::map<std::string, media::Player::PlayerKey> named_player_map;
    // We map UUIDs to their respective keys.
    std::map<std::string, media::Player::PlayerKey> uuid_player_map;
    // We keep a list of keys and their respective owners and states.
    // value: (owner context, attached state, attached dbus name)
    std::map<media::Player::PlayerKey, std::tuple<std::string, bool, std::string>> player_owner_map;

    boost::uuids::random_generator gen;

    // We expose the entire service as an MPRIS player.
    struct Exported
    {
        static mpris::MediaPlayer2::Skeleton::Configuration::Defaults media_player_defaults()
        {
            mpris::MediaPlayer2::Skeleton::Configuration::Defaults defaults;
            // TODO(tvoss): These three elements really should be configurable.
            defaults.identity = "core::media::Hub";
            defaults.desktop_entry = "mediaplayer-app";
            defaults.supported_mime_types = {"audio/mpeg3", "video/mpeg4"};

            return defaults;
        }

        static mpris::Player::Skeleton::Configuration::Defaults player_defaults()
        {
            mpris::Player::Skeleton::Configuration::Defaults defaults;

            return defaults;
        }

        explicit Exported(const dbus::Bus::Ptr& bus, const media::CoverArtResolver& cover_art_resolver,
                media::ServiceSkeleton* impl, const ServiceSkeleton::Configuration& config)
            : bus{bus},
              /* Export MediaHub service interface on dbus */
              service{dbus::Service::add_service(bus, "org.mpris.MediaPlayer2.MediaHub")},
              object{service->add_object_for_path(dbus::types::ObjectPath{"/org/mpris/MediaPlayer2"})},
              media_player{mpris::MediaPlayer2::Skeleton::Configuration{bus, object, media_player_defaults()}},
              player{mpris::Player::Skeleton::Configuration{bus, object, player_defaults()}},
              playlists{mpris::Playlists::Skeleton::Configuration{bus, object, mpris::Playlists::Skeleton::Configuration::Defaults{}}},
              cover_art_resolver{cover_art_resolver},
              impl{impl},
              service_skel_config(config)
        {
            object->install_method_handler<core::dbus::interfaces::Properties::GetAll>([this](const core::dbus::Message::Ptr& msg)
            {
                // Extract the interface
                std::string interface;
                msg->reader() >> interface;
                core::dbus::Message::Ptr reply = core::dbus::Message::make_method_return(msg);

                if (interface == mpris::Player::name())
                    reply->writer() << player.get_all_properties();
                else if (interface == mpris::MediaPlayer2::name())
                    reply->writer() << media_player.get_all_properties();
                else if (interface == mpris::Playlists::name())
                    reply->writer() << playlists.get_all_properties();

                Exported::bus->send(reply);
            });

            // Setup method handlers for mpris::Player methods.
            auto next = [this](const core::dbus::Message::Ptr& msg)
            {
                const auto sp = service_skel_config.player_store->current_player().get();

                if (is_multimedia_role())
                    sp->next();

                Exported::bus->send(core::dbus::Message::make_method_return(msg));
            };
            object->install_method_handler<mpris::Player::Next>(next);

            auto previous = [this](const core::dbus::Message::Ptr& msg)
            {
                const auto sp = service_skel_config.player_store->current_player().get();

                if (is_multimedia_role())
                    sp->previous();

                Exported::bus->send(core::dbus::Message::make_method_return(msg));
            };
            object->install_method_handler<mpris::Player::Previous>(previous);

            auto pause = [this](const core::dbus::Message::Ptr& msg)
            {
                const auto sp = service_skel_config.player_store->current_player().get();

                if (is_multimedia_role() and sp->can_pause())
                    sp->pause();

                Exported::bus->send(core::dbus::Message::make_method_return(msg));
            };
            object->install_method_handler<mpris::Player::Pause>(pause);

            auto stop = [this](const core::dbus::Message::Ptr& msg)
            {
                const auto sp = service_skel_config.player_store->current_player().get();

                if (is_multimedia_role())
                    sp->stop();

                Exported::bus->send(core::dbus::Message::make_method_return(msg));
            };
            object->install_method_handler<mpris::Player::Stop>(stop);

            auto play = [this, impl](const core::dbus::Message::Ptr& msg)
            {
                const auto sp = service_skel_config.player_store->current_player().get();

                if (is_multimedia_role() and sp->can_play())
                {
                    // Make sure other player sessions that are already playing
                    // are paused before triggering new player (sp) to play
                    if (impl)
                        impl->pause_other_sessions(sp->key());

                    sp->play();
                }

                Exported::bus->send(core::dbus::Message::make_method_return(msg));
            };
            object->install_method_handler<mpris::Player::Play>(play);

            auto play_pause = [this, impl](const core::dbus::Message::Ptr& msg)
            {
                const auto sp = service_skel_config.player_store->current_player().get();

                if (is_multimedia_role())
                {
                    if (sp->playback_status() == media::Player::PlaybackStatus::playing
                            and sp->can_pause())
                        sp->pause();
                    else if (sp->playback_status() != media::Player::PlaybackStatus::null
                                and sp->can_play())
                    {
                        // Make sure other player sessions that are already playing
                        // are paused before triggering new player (sp) to play
                        if (impl)
                            impl->pause_other_sessions(sp->key());

                        sp->play();
                    }
                }

                Exported::bus->send(core::dbus::Message::make_method_return(msg));
            };
            object->install_method_handler<mpris::Player::PlayPause>(play_pause);
        }

        inline bool is_multimedia_role()
        {
            MH_TRACE("");

            const auto sp = service_skel_config.player_store->current_player().get();
            return (sp ? sp->audio_stream_role() == media::Player::AudioStreamRole::multimedia : false);
        }

        void set_current_player(media::Player::PlayerKey key)
        {
            MH_TRACE("");

            // Update the current player in the Player store
            service_skel_config.player_store->set_current_player_for_key(key);
            const auto player_sp = service_skel_config.player_store->current_player().get();

            // And announce that we can be controlled again.
            player.properties.can_control->set(true);

            // We wire up player state changes
            connections.seeked_to = player_sp->seeked_to().connect([this](std::uint64_t position)
            {
                player.signals.seeked_to->emit(position);
            });

            connections.duration_changed = player_sp->duration().changed().connect([this](std::uint64_t duration)
            {
                player.properties.duration->set(duration);
            });

            connections.position_changed = player_sp->position().changed().connect([this](std::uint64_t position)
            {
                player.properties.position->set(position);
            });

            connections.playback_status_changed = player_sp->playback_status().changed().connect(
                [this, key, player_sp](core::ubuntu::media::Player::PlaybackStatus status)
            {
                const auto cp = service_skel_config.player_store->current_player().get();
                // If key points to the current player's key, then update status
                if (cp and key == cp->key())
                    player.properties.playback_status->set(mpris::Player::PlaybackStatus::from(status));
            });

            connections.loop_status_changed = player_sp->loop_status().changed().connect(
                [this](core::ubuntu::media::Player::LoopStatus status)
            {
                player.properties.loop_status->set(mpris::Player::LoopStatus::from(status));
            });

            connections.can_play_changed = player_sp->can_play().changed().connect(
                [this, key, player_sp](bool can_play)
            {
                const auto cp = service_skel_config.player_store->current_player().get();
                // If key points to the current player's key, then update can_play
                if (cp and key == cp->key())
                    player.properties.can_play->set(can_play);
            });

            connections.can_pause_changed = player_sp->can_pause().changed().connect(
                [this, key, player_sp](bool can_pause)
            {
                const auto cp = service_skel_config.player_store->current_player().get();
                // If key points to the current player's key, then update can_pause
                if (cp and key == cp->key())
                    player.properties.can_pause->set(can_pause);
            });

            connections.can_go_previous_changed = player_sp->can_go_previous().changed().connect(
                [this](bool can_go_previous)
            {
                player.properties.can_go_previous->set(can_go_previous);
            });

            connections.can_go_next_changed = player_sp->can_go_next().changed().connect(
                [this](bool can_go_next)
            {
                player.properties.can_go_next->set(can_go_next);
            });

            // NOTE: the metadata first gets updated in the PlayerImplementation constructor which connects
            // and reacts to the Engine track_meta_data changed signal. Setting the
            // meta_data_for_current_track here makes sure that the signal uses the MPRIS object path
            connections.meta_data_changed = player_sp->meta_data_for_current_track().changed().connect(
                    [this](const media::Track::MetaData& metadata)
            {
                player.properties.meta_data_for_current_track->set(metadata);
                mpris::Player::Dictionary dict; dict[mpris::Player::Properties::Metadata::name()]
                        = dbus::types::Variant::encode(metadata);
                player.signals.properties_changed->emit(std::make_tuple(
                                dbus::traits::Service<mpris::Player>::interface_name(),
                                dict,
                                mpris::Player::Skeleton::the_empty_list_of_invalidated_properties()));
            });

            // Sync property values between session and player mpris::Player instances
            // TODO Getters from media::Player actually return values from a
            // mpris::Player::Skeleton instance different from "player". Each of them use
            // different DBus object paths, /core/ubuntu/media/Service/sessions/<n>
            // and /org/mpris/MediaPlayer2 (this is the one enforced by the MPRIS spec).
            // Discuss why this is needed with tvoss.
            player.properties.duration->set(player_sp->duration().get());
            player.properties.position->set(player_sp->position().get());
            player.properties.playback_status->set(mpris::Player::PlaybackStatus::from(
                                                       player_sp->playback_status().get()));
            player.properties.loop_status->set(mpris::Player::LoopStatus::from(
                                                   player_sp->loop_status().get()));
            player.properties.can_play->set(player_sp->can_play().get());
            player.properties.can_pause->set(player_sp->can_pause().get());
            player.properties.can_go_previous->set(player_sp->can_go_previous().get());
            player.properties.can_go_next->set(player_sp->can_go_next().get());
        }

        void reset_current_player()
        {
            MH_TRACE("");
            // And announce that we can no longer be controlled.
            player.properties.can_control->set(false);
            player.properties.can_play->set(false);
            player.properties.can_pause->set(false);
            player.properties.can_go_previous->set(false);
            player.properties.can_go_next->set(false);

            // Reset to null event connections
            connections.seeked_to = the_empty_signal.connect([](){});
            connections.duration_changed = the_empty_signal.connect([](){});
            connections.position_changed = the_empty_signal.connect([](){});
            connections.playback_status_changed = the_empty_signal.connect([](){});
            connections.loop_status_changed = the_empty_signal.connect([](){});
            connections.can_play_changed = the_empty_signal.connect([](){});
            connections.can_pause_changed = the_empty_signal.connect([](){});
            connections.can_go_previous_changed = the_empty_signal.connect([](){});
            connections.can_go_next_changed = the_empty_signal.connect([](){});
            connections.meta_data_changed = the_empty_signal.connect([](){});
        }

        bool is_current_player(media::Player::PlayerKey key)

        {
            if (not service_skel_config.player_store)
                return false;
            if (not service_skel_config.player_store->current_player().get())
                return false;

            return key == service_skel_config.player_store->current_player().get()->key();
        }

        dbus::Bus::Ptr bus;
        dbus::Service::Ptr service;
        dbus::Object::Ptr object;

        mpris::MediaPlayer2::Skeleton media_player;
        mpris::Player::Skeleton player;
        mpris::Playlists::Skeleton playlists;

        // The CoverArtResolver used by the exported player.
        media::CoverArtResolver cover_art_resolver;

        media::ServiceSkeleton* impl;
        ServiceSkeleton::Configuration service_skel_config;

        // We track event connections.
        struct
        {
            core::Connection seeked_to
            {
                the_empty_signal.connect([](){})
            };
            core::Connection duration_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection position_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection playback_status_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection loop_status_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection can_play_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection can_pause_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection can_go_previous_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection can_go_next_changed
            {
                the_empty_signal.connect([](){})
            };
            core::Connection meta_data_changed
            {
                the_empty_signal.connect([](){})
            };
        } connections;
    } exported;
};

media::ServiceSkeleton::ServiceSkeleton(const Configuration& configuration)
    : dbus::Skeleton<media::Service>(the_session_bus()),
      d(new Private(this, configuration))
{
}

media::ServiceSkeleton::~ServiceSkeleton()
{
}

std::shared_ptr<media::Player> media::ServiceSkeleton::create_session(const media::Player::Configuration& config)
{
    return d->configuration.impl->create_session(config);
}

void media::ServiceSkeleton::detach_session(const std::string& uuid, const media::Player::Configuration& config)
{
    return d->configuration.impl->detach_session(uuid, config);
}

std::shared_ptr<media::Player> media::ServiceSkeleton::reattach_session(const std::string& uuid, const media::Player::Configuration& config)
{
    return d->configuration.impl->reattach_session(uuid, config);
}

void media::ServiceSkeleton::destroy_session(const std::string& uuid, const media::Player::Configuration& config)
{
    return d->configuration.impl->destroy_session(uuid, config);
}

std::shared_ptr<media::Player> media::ServiceSkeleton::create_fixed_session(const std::string& name, const media::Player::Configuration&config)
{
    return d->configuration.impl->create_fixed_session(name, config);
}

std::shared_ptr<media::Player> media::ServiceSkeleton::resume_session(media::Player::PlayerKey key)
{
    return d->configuration.impl->resume_session(key);
}

void media::ServiceSkeleton::equalizer_set_band(int band, double gain) {
    d->configuration.impl->equalizer_set_band(band, gain);
}

void media::ServiceSkeleton::set_current_player(media::Player::PlayerKey key)
{
    const std::shared_ptr<media::Player> player =
        d->configuration.player_store->player_for_key(key);
    // We only care to allow the MPRIS controls to apply to multimedia player (i.e. audio, video)
    if (player->audio_stream_role() == media::Player::AudioStreamRole::multimedia)
        d->exported.set_current_player(key);
}

bool media::ServiceSkeleton::is_current_player(media::Player::PlayerKey key) const
{
    return d->exported.is_current_player(key);
}

void media::ServiceSkeleton::reset_current_player()
{
    MH_TRACE("");
    d->exported.reset_current_player();
}

void media::ServiceSkeleton::pause_other_sessions(media::Player::PlayerKey key)
{
    MH_TRACE("");
    d->configuration.impl->pause_other_sessions(key);
}

void media::ServiceSkeleton::run()
{
    access_bus()->run();
}

void media::ServiceSkeleton::stop()
{
    access_bus()->stop();
}

const core::Signal<void>& media::ServiceSkeleton::service_disconnected() const
{
    throw std::runtime_error("This signal is only accessible from the ServiceStub");
    static const core::Signal<void> s;
    return s;
}

const core::Signal<void>& media::ServiceSkeleton::service_reconnected() const
{
    throw std::runtime_error("This signal is only accessible from the ServiceStub");
    static const core::Signal<void> s;
    return s;
}
