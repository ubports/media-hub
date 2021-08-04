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

#include "player_implementation.h"

#include <unistd.h>
#include <ctime>

#include "client_death_observer.h"
#include "engine.h"
#include "logging.h"
#include "track_list_implementation.h"
#include "xesam.h"

#include <QDateTime>
#include <QTimer>
#include <QVector>

#include "gstreamer/engine.h"

#include <memory>
#include <exception>
#include <mutex>

namespace media = core::ubuntu::media;

using namespace std;
using namespace media;

namespace core {
namespace ubuntu {
namespace media {

class PlayerImplementationPrivate
{
    Q_DECLARE_PUBLIC(PlayerImplementation)

public:
    enum class wakelock_clear_t
    {
        WAKELOCK_CLEAR_INACTIVE,
        WAKELOCK_CLEAR_DISPLAY,
        WAKELOCK_CLEAR_SYSTEM,
        WAKELOCK_CLEAR_INVALID
    };

    PlayerImplementationPrivate(const media::PlayerImplementation::Configuration &config,
                                PlayerImplementation *q);
    ~PlayerImplementationPrivate();

    void onStateChanged(Engine::State state)
    {
        /*
         * Wakelock state logic:
         * PLAYING->READY or PLAYING->PAUSED or PLAYING->STOPPED: delay 4 seconds and try to clear current wakelock type
         * ANY STATE->PLAYING: request a new wakelock (system or display)
         */
        MH_DEBUG() << "Setting state:" << state;
        switch(state)
        {
        case Engine::State::ready:
        {
            if (previous_state == Engine::State::playing)
            {
                m_wakeLockTimer.start();
            }
            break;
        }
        case Engine::State::playing:
        {
            // We update the track metadata prior to updating the playback status.
            // Some MPRIS clients expect this order of events.
            const auto trackMetadata = m_engine->trackMetadata();
            media::Track::MetaData metadata = trackMetadata.second;
            // Setting this with second resolution makes sure that the track_meta_data property changes
            // and thus the track_meta_data().changed() signal gets sent out over dbus. Otherwise the
            // Property caching mechanism would prevent this.
            metadata.setLastUsed(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
            update_mpris_metadata(trackMetadata.first, metadata);

            MH_INFO("Requesting power state");
            request_power_state();
            break;
        }
        case Engine::State::stopped:
        {
            if (previous_state ==  Engine::State::playing)
            {
                m_wakeLockTimer.start();
            }
            break;
        }
        case Engine::State::paused:
        {
            if (previous_state == Engine::State::playing)
            {
                m_wakeLockTimer.start();
            }
            break;
        }
        default:
            break;
        };

        // Keep track of the previous Engine playback state:
        previous_state = state;
    }

    void request_power_state()
    {
        Q_Q(PlayerImplementation);
        MH_TRACE("");
        if (q->isVideoSource())
        {
            MH_INFO("Request display on.");
            if (!m_holdsDisplayOn) {
                power_state_controller->requestDisplayOn();
                m_holdsDisplayOn = true;
            }
        }
        else
        {
            MH_INFO("Request system state active.");
            if (!m_holdsSystemActive) {
                power_state_controller->requestSystemState(media::power::SystemState::active);
                m_holdsSystemActive = true;
            }
        }
    }

    void clear_wakelock(const wakelock_clear_t &wakelock)
    {
        MH_TRACE("");
        switch (wakelock)
        {
            case wakelock_clear_t::WAKELOCK_CLEAR_INACTIVE:
                break;
            case wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM:
                MH_INFO("Release system state active.");
                if (m_holdsSystemActive) {
                    power_state_controller->releaseSystemState(media::power::SystemState::active);
                    m_holdsSystemActive = false;
                }
                break;
            case wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY:
                MH_INFO("Release display on.");
                if (m_holdsDisplayOn) {
                    power_state_controller->releaseDisplayOn();
                    m_holdsDisplayOn = false;
                }
                break;
            case wakelock_clear_t::WAKELOCK_CLEAR_INVALID:
            default:
                MH_WARNING("Can't clear invalid wakelock type");
        }
    }

    void clear_wakelocks()
    {
        // Clear both types of wakelocks (display and system)
        clear_wakelock(wakelock_clear_t::WAKELOCK_CLEAR_SYSTEM);
        clear_wakelock(wakelock_clear_t::WAKELOCK_CLEAR_DISPLAY);
    }

    void on_client_died()
    {
        Q_Q(PlayerImplementation);
        m_engine->reset();

        // If the client disconnects, make sure both wakelock types
        // are cleared
        clear_wakelocks();
        m_trackList->reset();

        // And tell the outside world that the client has gone away
        Q_EMIT q->clientDisconnected();
    }

    void open_first_track_from_tracklist(const media::Track::Id& id)
    {
        const QUrl uri = m_trackList->query_uri_for_track(id);
        if (!uri.isEmpty())
        {
            // Using a TrackList for playback, added tracks via add_track(), but open_uri hasn't
            // been called yet to load a media resource
            MH_INFO("Calling d->m_engine->open_resource_for_uri() for first track added only: %s",
                    qUtf8Printable(uri.toString()));
            MH_INFO("\twith a Track::Id: %s", qUtf8Printable(id));
            static const bool do_pipeline_reset = true;
            m_engine->open_resource_for_uri(uri, do_pipeline_reset);
        }
    }

    void update_mpris_properties()
    {
        Q_Q(PlayerImplementation);
        const bool has_previous = m_trackList->hasPrevious()
                            or m_trackList->loopStatus() != Player::LoopStatus::none;
        const bool has_next = m_trackList->hasNext()
                        or m_trackList->loopStatus() != Player::LoopStatus::none;
        const auto n_tracks = m_trackList->tracks().count();
        const bool has_tracks = (n_tracks > 0) ? true : false;

        MH_INFO("Updating MPRIS TrackList properties:");
        MH_INFO("\tTracks: %d", n_tracks);
        MH_INFO("\thas_previous: %d", has_previous);
        MH_INFO("\thas_next: %d", has_next);

        // Update properties
        m_canPlay = has_tracks;
        m_canPause = has_tracks;
        m_canGoPrevious = has_previous;
        m_canGoNext = has_next;
        Q_EMIT q->mprisPropertiesChanged();
    }

    QUrl get_uri_for_album_artwork(const QUrl &uri,
            const media::Track::MetaData& metadata)
    {
        QUrl art_uri;
        bool is_local_file = false;
        if (not uri.isEmpty())
            is_local_file = uri.isLocalFile();

        // If the track has a full image or preview image or is a video and it is a local file,
        // then use the thumbnailer cache
        if (  (metadata.value(tags::PreviewImage::name).toBool()
           or (metadata.value(tags::Image::name).toBool())
           or m_engine->isVideoSource())
           and is_local_file)
        {
            art_uri = "image://thumbnailer/" + uri.path();
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
    void update_mpris_metadata(const QUrl &uri, const media::Track::MetaData &md)
    {
        Q_Q(PlayerImplementation);
        media::Track::MetaData metadata{md};
        if (not metadata.isSet(media::Track::MetaData::TrackIdKey))
        {
            const QString current_track = m_trackList->current();
            if (not current_track.isEmpty())
            {
                const int last_slash = current_track.lastIndexOf('/');
                const QStringRef track_id = current_track.midRef(last_slash + 1);
                if (not track_id.isEmpty())
                    metadata.setTrackId("/org/mpris/MediaPlayer2/Track/" + track_id);
                else
                    MH_WARNING("Failed to set MPRIS track id since the id value is NULL");
            }
            else
                MH_WARNING("Failed to set MPRIS track id since the id value is NULL");
        }

        if (not metadata.isSet(media::Track::MetaData::TrackArtlUrlKey))
            metadata.setArtUrl(get_uri_for_album_artwork(uri, metadata));

        if (not metadata.isSet(media::Track::MetaData::TrackLengthKey))
        {
            // Duration is in nanoseconds, MPRIS spec requires microseconds
            metadata.setTrackLength(m_engine->duration() / 1000);
        }

        // not needed, and change frequently:
        metadata.remove(QStringLiteral("bitrate"));
        metadata.remove(QStringLiteral("minimum-bitrate"));
        metadata.remove(QStringLiteral("maximum-bitrate"));

        m_metadataForCurrentTrack = metadata;
        Q_EMIT q->metadataForCurrentTrackChanged();
    }

    bool is_multimedia_role() const
    {
        return m_engine->audioStreamRole() == media::Player::AudioStreamRole::multimedia;
    }

    Player::Client m_client;
    ClientDeathObserver::Ptr m_clientDeathObserver;
    media::power::StateController::Ptr power_state_controller;

    QScopedPointer<Engine> m_engine;
    QSharedPointer<TrackListImplementation> m_trackList;
    bool m_holdsSystemActive = false;
    bool m_holdsDisplayOn = false;
    Engine::State previous_state;
    std::atomic<bool> doing_abandon;
    // Initialize default values for Player interface properties
    bool m_canPlay = false;
    bool m_canPause = false;
    bool m_canGoPrevious = false;
    bool m_canGoNext = false;
    bool m_shuffle = false;
    double m_playbackRate = 1.f;
    Player::LoopStatus m_loopStatus = Player::LoopStatus::none;
    int64_t m_position = 0;
    int64_t m_duration = 0;
    bool m_doingOpenUri = false;
    Player::AudioStreamRole m_audioStreamRole = Player::AudioStreamRole::multimedia;
    Player::Lifetime m_lifetime = Player::Lifetime::normal;
    QTimer m_abandonTimer;
    QTimer m_wakeLockTimer;
    Track::MetaData m_metadataForCurrentTrack;
    media::PlayerImplementation *q_ptr;
};

}}} // namespace

PlayerImplementationPrivate::PlayerImplementationPrivate(
        const media::PlayerImplementation::Configuration &config,
        PlayerImplementation *q):
    m_client(config.client),
    m_clientDeathObserver(config.client_death_observer),
    power_state_controller(media::power::StateController::instance()),
    m_engine(new gstreamer::Engine(config.client.key)),
    // TODO: set the path on the TrackListSkeleton!
    // dbus::types::ObjectPath(config.parent.session->path().as_string() + "/TrackList")),
    m_trackList(QSharedPointer<TrackListImplementation>::create(
        m_engine->metadataExtractor())),
    previous_state(Engine::State::stopped),
    doing_abandon(false),
    q_ptr(q)
{
    // Poor man's logging of release/acquire events.
    QObject::connect(power_state_controller.data(),
                     &power::StateController::displayOnAcquired,
                     q, []() {
        MH_INFO("Acquired display ON state");
    });

    QObject::connect(power_state_controller.data(),
                     &power::StateController::displayOnReleased,
                     q, []() {
        MH_INFO("Released display ON state");
    });

    QObject::connect(power_state_controller.data(),
                     &power::StateController::systemStateAcquired,
                     q, [](media::power::SystemState state)
    {
        MH_INFO() << "Acquired new system state:" << state;
    });

    QObject::connect(power_state_controller.data(),
                     &power::StateController::systemStateAcquired,
                     q, [](media::power::SystemState state)
    {
        MH_INFO() << "Released system state:" << state;
    });

    QObject::connect(m_engine.data(), &Engine::stateChanged,
                     q, [this]() {
        onStateChanged(m_engine->state());
    });

    // Initialize default values for Player interface properties
    m_engine->setAudioStreamRole(Player::AudioStreamRole::multimedia);
    m_engine->setLifetime(Player::Lifetime::normal);

    // Make sure that the Position property gets updated from the Engine
    // every time the client requests position
    QObject::connect(m_engine.data(), &Engine::positionChanged,
                     q, [this, q]() {
        m_trackList->setCurrentPosition(m_engine->position());
        Q_EMIT q->positionChanged();
    });

    // Make sure that the Duration property gets updated from the Engine
    // every time the client requests duration
    QObject::connect(m_engine.data(), &Engine::durationChanged,
                     q, &PlayerImplementation::durationChanged);

    // When the value of the orientation Property is changed in the Engine by playbin,
    // update the Player's cached value
    QObject::connect(m_engine.data(), &Engine::orientationChanged,
                     q, &PlayerImplementation::orientationChanged);

    QObject::connect(m_engine.data(), &Engine::trackMetadataChanged,
                     q, [this]() {
        const auto md = m_engine->trackMetadata();
        update_mpris_metadata(md.first, md.second);
    });

    QObject::connect(m_engine.data(), &Engine::endOfStream,
                     q, [q, this]()
    {
        if (doing_abandon)
            return;

        Q_EMIT q->endOfStream();

        // Make sure that the TrackList keeps advancing. The logic for what gets played next,
        // if anything at all, occurs in TrackListSkeleton::next()
        m_trackList->next();
    });

    QObject::connect(m_engine.data(), &Engine::clientDisconnected,
                     q, [this]()
    {
        on_client_died();
    });

    QObject::connect(m_engine.data(), &Engine::seekedTo,
                     q, &PlayerImplementation::seekedTo);
    QObject::connect(m_engine.data(), &Engine::bufferingChanged,
                     q, &PlayerImplementation::bufferingChanged);
    QObject::connect(m_engine.data(), &Engine::playbackStatusChanged,
                     q, &PlayerImplementation::playbackStatusChanged);
    QObject::connect(m_engine.data(), &Engine::aboutToFinish,
                     q, &PlayerImplementation::aboutToFinish);
    QObject::connect(m_engine.data(), &Engine::videoDimensionChanged,
                     q, &PlayerImplementation::videoDimensionChanged);
    QObject::connect(m_engine.data(), &Engine::errorOccurred,
                     q, &PlayerImplementation::errorOccurred);

    QObject::connect(m_trackList.data(), &TrackListImplementation::endOfTrackList,
                     q, [this]()
    {
        if (m_engine->state() != gstreamer::Engine::State::stopped)
        {
            MH_INFO("End of tracklist reached, stopping playback");
            m_engine->stop();
        }
    });

    QObject::connect(m_trackList.data(), &TrackListImplementation::onGoToTrack,
                     q, [this](const media::Track::Id &id)
    {
        // Store whether we should restore the current playing state after loading the new uri
        const bool auto_play = m_engine->playbackStatus() == media::Player::playing;

        const QUrl uri = m_trackList->query_uri_for_track(id);
        if (!uri.isEmpty())
        {
            MH_INFO("Setting next track on playbin (on_go_to_track signal): %s",
                    qUtf8Printable(uri.toString()));
            MH_INFO("\twith a Track::Id: %s", qUtf8Printable(id));
            static const bool do_pipeline_reset = true;
            m_engine->open_resource_for_uri(uri, do_pipeline_reset);
        }

        if (auto_play)
        {
            MH_DEBUG("Restoring playing state");
            m_engine->play();
        }
    });

    QObject::connect(m_trackList.data(), &TrackListImplementation::trackAdded,
                     q, [this](const media::Track::Id &id)
    {
        MH_TRACE("** Track was added, handling in PlayerImplementation");
        if (!m_doingOpenUri && m_trackList->tracks().count() == 1)
            open_first_track_from_tracklist(id);

        update_mpris_properties();
    });

    QObject::connect(m_trackList.data(), &TrackListImplementation::tracksAdded,
                     q, [this](const QVector<QUrl> &tracks)
    {
        MH_TRACE("** Track was added, handling in PlayerImplementation");
        // If the two sizes are the same, that means the TrackList was previously empty and we need
        // to open the first track in the TrackList so that is_audio_source() and is_video_source()
        // will function correctly.
        /* FIXME: we are passing a URL to a method expecting a track ID, so
         * this will not work; on the other hand, the code has always been like
         * this, so let's fix it later. */
        if (not tracks.isEmpty() and m_trackList->tracks().count() == tracks.count())
            open_first_track_from_tracklist(tracks.front().toString());

        update_mpris_properties();
    });

    QObject::connect(m_trackList.data(),
                     &TrackListImplementation::trackRemoved,
                     q, [this]() { update_mpris_properties(); });

    QObject::connect(m_trackList.data(),
                     &TrackListImplementation::trackListReset,
                     q, [this]() { update_mpris_properties(); });

    QObject::connect(m_trackList.data(),
                     &TrackListImplementation::trackChanged,
                     q, [this]() { update_mpris_properties(); });

    QObject::connect(m_trackList.data(),
                     &TrackListImplementation::trackListReplaced,
                     q, [this]() { update_mpris_properties(); });

    // Everything is setup, we now subscribe to death notifications.
    m_clientDeathObserver->registerForDeathNotifications(m_client);
    QObject::connect(m_clientDeathObserver.data(),
                     &ClientDeathObserver::clientDied,
                     q, [this](const media::Player::Client &died)
    {
        if (doing_abandon)
            return;

        if (died.key != m_client.key)
            return;

        m_abandonTimer.start();
    });

    m_abandonTimer.setSingleShot(true);
    m_abandonTimer.setInterval(1000);
    m_abandonTimer.callOnTimeout(q, [this]() { on_client_died(); });

    m_wakeLockTimer.setSingleShot(true);
    int wakelockTimeout =
        qEnvironmentVariableIsSet("MEDIA_HUB_WAKELOCK_TIMEOUT") ?
        qEnvironmentVariableIntValue("MEDIA_HUB_WAKELOCK_TIMEOUT") : 4000;
    m_wakeLockTimer.setInterval(wakelockTimeout);
    m_wakeLockTimer.setTimerType(Qt::VeryCoarseTimer);
    m_wakeLockTimer.callOnTimeout(q, [this]() {
        clear_wakelocks();
    });
}

PlayerImplementationPrivate::~PlayerImplementationPrivate()
{
    // Make sure that we don't hold on to the wakelocks if media-hub-server
    // ever gets restarted manually or automatically
    clear_wakelocks();
}

PlayerImplementation::PlayerImplementation(const Configuration &config,
                                           QObject *parent):
    QObject(parent),
    d_ptr(new media::PlayerImplementationPrivate(config, this))
{
    QObject::connect(this, &QObject::objectNameChanged,
                     this, [this](const QString &name) {
        Q_D(PlayerImplementation);
        d->m_trackList->setObjectName(name + "/TrackList");
    });
}

media::PlayerImplementation::~PlayerImplementation()
{
}

AVBackend::Backend PlayerImplementation::backend() const
{
    return media::AVBackend::get_backend_type();
}

const Player::Client &PlayerImplementation::client() const
{
    Q_D(const PlayerImplementation);
    return d->m_client;
}

bool PlayerImplementation::canPlay() const
{
    Q_D(const PlayerImplementation);
    return d->m_canPlay;
}

bool PlayerImplementation::canPause() const
{
    Q_D(const PlayerImplementation);
    return d->m_canPause;
}

bool PlayerImplementation::canSeek() const
{
    Q_D(const PlayerImplementation);
    return true;
}

bool PlayerImplementation::canGoPrevious() const
{
    Q_D(const PlayerImplementation);
    return d->m_canGoPrevious;
}

bool PlayerImplementation::canGoNext() const
{
    Q_D(const PlayerImplementation);
    return d->m_canGoNext;
}

void PlayerImplementation::setPlaybackRate(double rate)
{
    Q_UNUSED(rate);
    MH_WARNING("Setting playback rate not implemented");
}

double PlayerImplementation::playbackRate() const
{
    return 1.0;
}

double PlayerImplementation::minimumRate() const
{
    return 1.0;
}

double PlayerImplementation::maximumRate() const
{
    return 1.0;
}

void PlayerImplementation::setLoopStatus(Player::LoopStatus status)
{
    Q_D(PlayerImplementation);
    MH_INFO() << "LoopStatus:" << status;
    d->m_trackList->setLoopStatus(status);
}

Player::LoopStatus PlayerImplementation::loopStatus() const
{
    Q_D(const PlayerImplementation);
    return d->m_trackList->loopStatus();
}

void PlayerImplementation::setShuffle(bool shuffle)
{
    Q_D(PlayerImplementation);
    d->m_trackList->setShuffle(shuffle);
}

bool PlayerImplementation::shuffle() const
{
    Q_D(const PlayerImplementation);
    return d->m_trackList->shuffle();
}

void PlayerImplementation::setVolume(double volume)
{
    Q_D(PlayerImplementation);
    d->m_engine->setVolume(volume);
    Q_EMIT volumeChanged();
}

double PlayerImplementation::volume() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->volume();
}

Player::PlaybackStatus PlayerImplementation::playbackStatus() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->playbackStatus();
}

bool PlayerImplementation::isVideoSource() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->isVideoSource();
}

bool PlayerImplementation::isAudioSource() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->isAudioSource();
}

QSize PlayerImplementation::videoDimension() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->videoDimension();
}

Player::Orientation PlayerImplementation::orientation() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->orientation();
}

Track::MetaData PlayerImplementation::metadataForCurrentTrack() const
{
    Q_D(const PlayerImplementation);
    return d->m_metadataForCurrentTrack;
}

uint64_t PlayerImplementation::position() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->position();
}

uint64_t PlayerImplementation::duration() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->duration();
}

void PlayerImplementation::setAudioStreamRole(Player::AudioStreamRole role)
{
    Q_D(PlayerImplementation);
    d->m_engine->setAudioStreamRole(role);
}

Player::AudioStreamRole PlayerImplementation::audioStreamRole() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->audioStreamRole();
}

void PlayerImplementation::setLifetime(Player::Lifetime lifetime)
{
    Q_D(PlayerImplementation);
    d->m_engine->setLifetime(lifetime);
}

Player::Lifetime PlayerImplementation::lifetime() const
{
    Q_D(const PlayerImplementation);
    return d->m_engine->lifetime();
}

void media::PlayerImplementation::reconnect()
{
    Q_D(PlayerImplementation);
    d->m_clientDeathObserver->registerForDeathNotifications(d->m_client);
}

void media::PlayerImplementation::abandon()
{
    Q_D(PlayerImplementation);
    // Signal client disconnection due to abandonment of player
    d->doing_abandon = true;
    d->on_client_died();
}

QSharedPointer<TrackListImplementation> media::PlayerImplementation::trackList()
{
    Q_D(PlayerImplementation);
    return d->m_trackList;
}

media::Player::PlayerKey media::PlayerImplementation::key() const
{
    Q_D(const PlayerImplementation);
    return d->m_client.key;
}

void media::PlayerImplementation::create_gl_texture_video_sink(std::uint32_t texture_id)
{
    Q_D(PlayerImplementation);
    d->m_engine->create_video_sink(texture_id);
}

bool PlayerImplementation::open_uri(const QUrl &uri)
{
    return open_uri(uri, Headers());
}

bool PlayerImplementation::open_uri(const QUrl &uri, const Headers &headers)
{
    Q_D(PlayerImplementation);
    d->m_doingOpenUri = true;
    d->m_trackList->reset();

    // If empty uri, give the same meaning as QMediaPlayer::setMedia("")
    if (uri.isEmpty())
    {
        MH_DEBUG("Resetting current media");
        return true;
    }

    const bool ret = d->m_engine->open_resource_for_uri(uri, headers);
    // Don't set new track as the current track to play since we're calling open_resource_for_uri above
    static const bool make_current = false;
    d->m_trackList->add_track_with_uri_at(uri, TrackListImplementation::afterEmptyTrack(), make_current);
    d->m_doingOpenUri = false;

    return ret;
}

void PlayerImplementation::next()
{
    Q_D(PlayerImplementation);
    d->m_trackList->next();
}

void PlayerImplementation::previous()
{
    Q_D(PlayerImplementation);
    d->m_trackList->previous();
}

void PlayerImplementation::play()
{
    MH_TRACE("");
    Q_D(PlayerImplementation);
    if (d->is_multimedia_role())
    {
        Q_EMIT playbackRequested();
    }

    d->m_engine->play();
}

void PlayerImplementation::pause()
{
    MH_TRACE("");
    Q_D(PlayerImplementation);
    d->m_engine->pause();
}

void PlayerImplementation::stop()
{
    MH_TRACE("");
    Q_D(PlayerImplementation);
    d->m_engine->stop();
}

void PlayerImplementation::seek_to(const std::chrono::microseconds& ms)
{
    Q_D(PlayerImplementation);
    d->m_engine->seek_to(ms);
}
