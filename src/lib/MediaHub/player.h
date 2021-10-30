/*
 * Copyright © 2021 UBports Foundation.
 *
 * Contact: Alberto Mardegan <mardy@users.sourceforge.net>
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
 */

#ifndef LOMIRI_MEDIAHUB_PLAYER_H
#define LOMIRI_MEDIAHUB_PLAYER_H

#include "track.h"

#include <QObject>
#include <QScopedPointer>
#include <QString>

class QUrl;

namespace lomiri {
namespace MediaHub {

class Error;
class Service;
class TrackList;
class VideoSink;

class PlayerPrivate;
class MH_EXPORT Player: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Player)
    Q_PROPERTY(bool canPlay READ canPlay NOTIFY controlsChanged)
    Q_PROPERTY(bool canPause READ canPause NOTIFY controlsChanged)
    Q_PROPERTY(bool canSeek READ canSeek NOTIFY controlsChanged)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY controlsChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY controlsChanged)

    Q_PROPERTY(bool isVideoSource READ isVideoSource NOTIFY sourceTypeChanged)
    Q_PROPERTY(bool isAudioSource READ isAudioSource NOTIFY sourceTypeChanged)

    Q_PROPERTY(PlaybackStatus playbackStatus READ playbackStatus
               NOTIFY playbackStatusChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle
               NOTIFY shuffleChanged)
    Q_PROPERTY(Volume volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(Track::MetaData metaDataForCurrentTrack
               READ metaDataForCurrentTrack
               NOTIFY metaDataForCurrentTrackChanged)

    Q_PROPERTY(PlaybackRate playbackRate
               READ playbackRate WRITE setPlaybackRate
               NOTIFY playbackRateChanged)
    Q_PROPERTY(PlaybackRate minimumPlaybackRate READ minimumPlaybackRate
               NOTIFY minimumPlaybackRateChanged)
    Q_PROPERTY(PlaybackRate maximumPlaybackRate READ maximumPlaybackRate
               NOTIFY maximumPlaybackRateChanged)

    Q_PROPERTY(quint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(quint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(Orientation orientation READ orientation
               NOTIFY orientationChanged)

    Q_PROPERTY(LoopStatus loopStatus READ loopStatus WRITE setLoopStatus
               NOTIFY loopStatusChanged)
    Q_PROPERTY(AudioStreamRole audioStreamRole
               READ audioStreamRole WRITE setAudioStreamRole
               NOTIFY audioStreamRoleChanged)

public:
    typedef double PlaybackRate;
    typedef double Volume;
    typedef QMap<QString, QString> Headers;

    enum PlaybackStatus {
        Null,
        Ready,
        Playing,
        Paused,
        Stopped,
    };
    Q_ENUM(PlaybackStatus)

    enum LoopStatus {
        LoopNone,
        LoopTrack,
        LoopPlaylist,
    };
    Q_ENUM(LoopStatus)

    /**
     * Audio stream role types used to categorize audio playback.
     * multimedia is the default role type and will be automatically
     * paused by media-hub when other types need to play.
     */
    enum AudioStreamRole {
        AlarmRole,
        AlertRole,
        MultimediaRole,
        PhoneRole,
    };
    Q_ENUM(AudioStreamRole)

    enum Orientation {
        Rotate0,
        Rotate90,
        Rotate180,
        Rotate270,
    };
    Q_ENUM(Orientation)

    /* Do we need a link to the Service? We could call CreateSession
     * internally.
     */
    Player(QObject *parent = nullptr);
    virtual ~Player();

    QString uuid() const;

    /*
     * Unused methods:
     * - reconnect
     * - abandon
     */

    void setTrackList(TrackList *trackList);
    TrackList *trackList() const;

    // The returned object is owned by the Player
    VideoSink &createGLTextureVideoSink(uint32_t textureId);

    void openUri(const QUrl &uri, const Headers &headers = {});
    void goToNext();
    void goToPrevious();
    void play();
    void pause();
    void stop();
    void seekTo(uint64_t microseconds);

    /*
     * Property accessors
     */
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canGoPrevious() const;
    bool canGoNext() const;

    bool isVideoSource() const;
    bool isAudioSource() const;

    PlaybackStatus playbackStatus() const;
    void setPlaybackRate(PlaybackRate rate);
    PlaybackRate playbackRate() const;
    void setShuffle(bool shuffle);
    bool shuffle() const;
    void setVolume(Volume volume);
    Volume volume() const;
    Track::MetaData metaDataForCurrentTrack() const;
    PlaybackRate minimumPlaybackRate() const;
    PlaybackRate maximumPlaybackRate() const;
    quint64 position() const;
    quint64 duration() const;
    Orientation orientation() const;
    void setLoopStatus(LoopStatus loopStatus);
    LoopStatus loopStatus() const;
    void setAudioStreamRole(AudioStreamRole role);
    AudioStreamRole audioStreamRole() const;

Q_SIGNALS:
    void controlsChanged();
    void sourceTypeChanged();
    void playbackStatusChanged();
    void backendChanged();
    void metaDataForCurrentTrackChanged();
    void loopStatusChanged();
    void playbackRateChanged();
    void shuffleChanged();
    void volumeChanged();
    void minimumPlaybackRateChanged();
    void maximumPlaybackRateChanged();
    void positionChanged(quint64 microseconds);
    void durationChanged(quint64 microseconds);
    void audioStreamRoleChanged();
    void orientationChanged();

    void seekedTo(quint64 microseconds);
    void aboutToFinish();
    void endOfStream();
    void videoDimensionChanged(const QSize &size);
    /** Signals all errors and warnings (typically from GStreamer and below) */
    void errorOccurred(const Error &error);
    void bufferingChanged(int percent);
    void serviceDisconnected();
    void serviceReconnected();

private:
    Q_DECLARE_PRIVATE(Player)
    QScopedPointer<PlayerPrivate> d_ptr;
};

} // namespace MediaHub
} // namespace lomiri

#endif // LOMIRI_MEDIAHUB_PLAYER_H
