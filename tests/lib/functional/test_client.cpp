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

#include "MediaHub/Error"
#include "MediaHub/Player"
#include "MediaHub/TrackList"
#include "MediaHub/VideoSink"

#include "MediaHub/dbus_constants.h"

#include "dbusmock.h" // workarounds for various libqtdbusmock issues

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QPair>
#include <QProcess>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>
#include <libqtdbusmock/DBusMock.h>

namespace QTest {

template<>
char *toString(const QVariantMap &map)
{
    QJsonDocument doc(QJsonObject::fromVariantMap(map));
    return qstrdup(doc.toJson(QJsonDocument::Compact).data());
}

} // QTest namespace

using namespace lomiri::MediaHub;

const QDBusArgument &operator>>(const QDBusArgument &arg,
                                Player::Headers &headers)
{
    arg.beginMap();
    while (!arg.atEnd()) {
        QString key, value;
        arg.beginMapEntry();
        arg >> key >> value;
        arg.endMapEntry();
        headers.insert(key, value);
    }
    arg.endMap();
    return arg;
}

class MediaHubMock
{
public:
    MediaHubMock(QtDBusMock::DBusMock *mock): m_mock(mock) {
        m_mock->registerTemplate(MEDIAHUB_SERVICE_NAME,
                                 MEDIAHUB_MOCK_TEMPLATE,
                                 QDBusConnection::SessionBus);
    }

    void ensurePlayerSetup() const {
        if (m_playerPath.isEmpty()) {
            QDBusReply<QString> reply = serviceMock().call("GetLastPlayerPath");
            m_playerPath = reply.value();
        }
    }

    QString playerUuid() const {
        ensurePlayerSetup();
        QDBusReply<QString> reply =
            serviceMock().call("GetPlayerUuid", m_playerPath);
        return reply.value();
    }

    QString playerPath() const { return m_playerPath; }

    void setNextPlayerProperties(const QVariantMap &properties) {
        serviceMock().call("SetNextPlayerProperties", properties);
    }

    void setOpenUriError(const QString &code, const QString &message) {
        serviceMock().call("SetOpenUriError", code, message);
    }

    void setPlayerProperty(const QString &name, const QVariant &value) {
        ensurePlayerSetup();
        QDBusMessage msg =
            QDBusMessage::createMethodCall(MEDIAHUB_SERVICE_NAME,
                                           m_playerPath,
                                           FDO_PROPERTIES_INTERFACE,
                                           "Set");
        msg.setArguments({ MPRIS_PLAYER_INTERFACE, name, value });
        QDBusConnection::sessionBus().call(msg);
    }

    QVariant getPlayerProperty(const QString &name) {
        ensurePlayerSetup();
        QDBusMessage msg =
            QDBusMessage::createMethodCall(MEDIAHUB_SERVICE_NAME,
                                           m_playerPath,
                                           FDO_PROPERTIES_INTERFACE,
                                           "Get");
        msg.setArguments({ MPRIS_PLAYER_INTERFACE, name });
        QDBusReply<QVariant> reply = QDBusConnection::sessionBus().call(msg);
        return reply.value();
    }

    QList<QtDBusMock::MethodCall> getCalls(OrgFreedesktopDBusMockInterface &i,
                                           const QString &methodName) {
        auto calls = i.GetMethodCalls(methodName);
        calls.waitForFinished();
        /* qtdbusmock incorrectly assumes that every dbus map is a QVariantMap,
         * so we cannot just do `calls.value()` here
         */
        return QtDBusMock::parseCalls(calls.reply());
    }

    OrgFreedesktopDBusMockInterface &serviceMock() const {
        return m_mock->mockInterface(MEDIAHUB_SERVICE_NAME,
                                     MEDIAHUB_SERVICE_PATH,
                                     MEDIAHUB_SERVICE_INTERFACE,
                                     QDBusConnection::SessionBus);
    }

    OrgFreedesktopDBusMockInterface &playerMock() {
        ensurePlayerSetup();
        return m_mock->mockInterface(MEDIAHUB_SERVICE_NAME,
                                     m_playerPath,
                                     MPRIS_PLAYER_INTERFACE,
                                     QDBusConnection::SessionBus);
    }

    OrgFreedesktopDBusMockInterface &trackListMock() {
        ensurePlayerSetup();
        return m_mock->mockInterface(MEDIAHUB_SERVICE_NAME,
                                     m_playerPath + "/TrackList",
                                     MPRIS_TRACKLIST_INTERFACE,
                                     QDBusConnection::SessionBus);
    }

    void reset() {
        serviceMock().ClearCalls();
        serviceMock().call("Reset");
        m_playerPath.clear();
    }

private:
    QtDBusMock::DBusMock *m_mock;
    mutable QString m_playerPath;
};

class TestClient: public QObject
{
    Q_OBJECT

public:
    TestClient();

    void setupMocking() {
        if (!m_dbus) {
            m_dbus.reset(new QtDBusTest::DBusTestRunner());
        }
        if (!m_mock) {
            m_mock.reset(new QtDBusMock::DBusMock(*m_dbus.data()));
            // Uncomment for debugging
            QProcess::startDetached("dbus-monitor", { "--session" });
            //QProcess::startDetached("busctl", { "--user", "monitor" });
            m_mediaHub.reset(new MediaHubMock(m_mock.data()));
            m_dbus->startServices();
        }
        m_mediaHub->serviceMock();
    }

    void emitPlayerSignal(const QString &name, const QString &signature,
                          const QVariantList &args) {
        m_mediaHub->playerMock().EmitSignal(MPRIS_PLAYER_INTERFACE,
                                           name, signature, args);
    }

    QList<QtDBusMock::MethodCall> getServiceCalls(const QString &methodName) {
        return m_mediaHub->getCalls(m_mediaHub->serviceMock(), methodName);
    }

    QList<QtDBusMock::MethodCall> getPlayerCalls(const QString &methodName) {
        return m_mediaHub->getCalls(m_mediaHub->playerMock(), methodName);
    }

    QList<QtDBusMock::MethodCall> getTrackListCalls(const QString &methodName) {
        return m_mediaHub->getCalls(m_mediaHub->trackListMock(), methodName);
    }

    void resetMocking() {
        m_mediaHub->reset();
    }

private Q_SLOTS:
    void init();
    void cleanup();
    void testConstructor();
    void testInitialization();

    void testInitialProperties_data();
    void testInitialProperties();
    void testWritableProperties_data();
    void testWritableProperties();
    void testInvalidProperties();
    void testPlayerMetaData();

    void testOpenUri_data();
    void testOpenUri();
    void testOpenUriWithHeaders();
    void testPlayerMethods();
    void testPlayerSignals();

    void testTracklistConstructor();
    void testEmptyTracklist();
    void testOfflineTracklist();
    void testTracklistAddSingle();
    void testTracklistEditing();
    void testCurrentTrack();

    void testVideoSink();

private:
    QScopedPointer<QtDBusTest::DBusTestRunner> m_dbus;
    QScopedPointer<QtDBusMock::DBusMock> m_mock;
    QScopedPointer<MediaHubMock> m_mediaHub;
};

TestClient::TestClient():
    QObject()
{
    qDBusRegisterMetaType<QtDBusMock::MethodCall>();
    qDBusRegisterMetaType<QList<QtDBusMock::MethodCall>>();
    qDBusRegisterMetaType<QList<QtDBusMock::Method>>();
    qDBusRegisterMetaType<QMap<QString,QString>>();
}

void TestClient::init()
{
    // We always want to have a D-Bus session
    setupMocking();
}

void TestClient::cleanup()
{
    resetMocking();
}

void TestClient::testConstructor()
{
    Player player1;
    QScopedPointer<Player> player2(new Player(this));
}

void TestClient::testInitialization()
{
    QString uuid;
    {
        Player player;
        QCOMPARE(getServiceCalls("CreateSession").count(), 1);
        uuid = m_mediaHub->playerUuid();
        QCOMPARE(player.uuid(), uuid);
        QCOMPARE(getServiceCalls("DestroySession").count(), 0);
    }
    auto destroySessionCalls = getServiceCalls("DestroySession");
    QCOMPARE(destroySessionCalls.count(), 1);
    QCOMPARE(destroySessionCalls[0].args(), QVariantList { uuid });
}

void TestClient::testInitialProperties_data()
{
    QTest::addColumn<QVariantMap>("dbusProperties");
    QTest::addColumn<QVariantMap>("expectedProperties");

    QTest::newRow("default values") <<
        QVariantMap() <<
        QVariantMap {
            { "canPlay", false },
            { "canPause", false },
            { "canSeek", false },
            { "canGoPrevious", false },
            { "canGoNext", false },
            { "isVideoSource", false },
            { "isAudioSource", false },
            { "playbackStatus", Player::Null },
            { "shuffle", false },
            { "volume", 1.0 },
            { "metaDataForCurrentTrack", Track::MetaData() },
            { "playbackRate", 1.0 },
            { "minimumPlaybackRate", 1.0 },
            { "maximumPlaybackRate", 1.0 },
            { "orientation", Player::Rotate0 },
        };

    QTest::newRow("service values") <<
        QVariantMap {
            { "Position", 1234ULL },
            { "Duration", 4321ULL },
            { "LoopStatus", "Track" },
            { "AudioStreamRole", qint16(Player::MultimediaRole) },
            { "Orientation", qint16(Player::Rotate270) },
        }<<
        QVariantMap {
            { "position", 1234ULL },
            { "duration", 4321ULL },
            { "loopStatus", Player::LoopTrack },
            { "orientation", Player::Rotate270 },
        };
}

void TestClient::testInitialProperties()
{
    QFETCH(QVariantMap, dbusProperties);
    QFETCH(QVariantMap, expectedProperties);

    m_mediaHub->setNextPlayerProperties(dbusProperties);

    Player player;
    QVariantMap properties;
    for (auto i = expectedProperties.constBegin();
         i != expectedProperties.constEnd(); i++) {
        properties.insert(i.key(),
                          player.property(i.key().toUtf8().constData()));
    }
    QCOMPARE(properties, expectedProperties);
}

void TestClient::testWritableProperties_data()
{
    using Prop = QPair<QString,QVariant>;
    QTest::addColumn<Prop>("property");
    QTest::addColumn<Prop>("expectedDBusProperty");

    QTest::newRow("playbackRate") <<
        Prop("playbackRate", 0.6) <<
        Prop("PlaybackRate", 0.6);

    QTest::newRow("shuffle") <<
        Prop("shuffle", true) <<
        Prop("Shuffle", true);

    QTest::newRow("volume") <<
        Prop("volume", 0.7) <<
        Prop("Volume", 0.7);

    QTest::newRow("loop status 1") <<
        Prop("loopStatus", Player::LoopNone) <<
        Prop("LoopStatus", "None");

    QTest::newRow("loop status 2") <<
        Prop("loopStatus", Player::LoopTrack) <<
        Prop("LoopStatus", "Track");

    QTest::newRow("loop status 3") <<
        Prop("loopStatus", Player::LoopPlaylist) <<
        Prop("LoopStatus", "Playlist");

    QTest::newRow("audio stream role") <<
        Prop("audioStreamRole", Player::PhoneRole) <<
        Prop("AudioStreamRole", 3);
}

void TestClient::testWritableProperties()
{
    using Prop = QPair<QString,QVariant>;
    QFETCH(Prop, property);
    QFETCH(Prop, expectedDBusProperty);

    Player player;
    player.setProperty(property.first.toUtf8().constData(), property.second);
    QVariant value = m_mediaHub->getPlayerProperty(expectedDBusProperty.first);
    QCOMPARE(value, expectedDBusProperty.second);

    QTRY_COMPARE(player.property(property.first.toUtf8().constData()),
                 property.second);
}

void TestClient::testInvalidProperties()
{
    QTest::ignoreMessage(
        QtWarningMsg, QRegularExpression("Unknown loop status enum:.*43.*"));

    Player player;
    player.setLoopStatus(static_cast<Player::LoopStatus>(43));
    QCOMPARE(player.loopStatus(), Player::LoopNone);
}

void TestClient::testPlayerMetaData()
{
    Player player;
    QCOMPARE(player.metaDataForCurrentTrack(), Track::MetaData());
    QSignalSpy metadataChanged(&player,
                               &Player::metaDataForCurrentTrackChanged);

    QVariantMap userMetadata {
        { "mpris:length", quint64(333) },
        { "xesam:title", QStringLiteral("A song") },
    };
    QVariantMap dbusMetadata = userMetadata;
    dbusMetadata.insert("mpris:trackid",
                        QVariant::fromValue(QDBusObjectPath("/track/1")));
    m_mediaHub->setPlayerProperty("Metadata", dbusMetadata);

    QVERIFY(metadataChanged.wait(2000));
    QCOMPARE(QVariantMap(player.metaDataForCurrentTrack()), userMetadata);
}

void TestClient::testOpenUri_data()
{
    QTest::addColumn<QString>("errorName");
    QTest::addColumn<QString>("errorMessage");
    QTest::addColumn<int>("expectedCode");

    QTest::newRow("no error") <<
        QString() << QString() << int(Error::NoError);

    QTest::newRow("apparmor error") <<
        MPRIS_ERROR_PREFIX MPRIS_ERROR_INSUFFICIENT_APPARMOR_PERMISSIONS <<
        "AppArmor says no" <<
        int(Error::AccessDeniedError);

    QTest::newRow("apparmor error") <<
        MPRIS_ERROR_PREFIX MPRIS_ERROR_OOP_STREAMING_NOT_SUPPORTED <<
        "No streaming, sorry" <<
        int(Error::OutOfProcessBufferStreamingNotSupported);

    QTest::newRow("file not found") <<
        MPRIS_ERROR_PREFIX MPRIS_ERROR_URI_NOT_FOUND <<
        "Not found" <<
        int(Error::ResourceError);

    QTest::newRow("D-Bus error") <<
        FDO_ERROR_PREFIX FDO_ERROR_SERVICE_UNKNOWN <<
        "MediaHub is gone" <<
        int(Error::ServiceMissingError);
}

void TestClient::testOpenUri()
{
    QFETCH(QString, errorName);
    QFETCH(QString, errorMessage);
    QFETCH(int, expectedCode);

    Player player;
    if (!errorName.isEmpty()) {
        m_mediaHub->setOpenUriError(errorName, errorMessage);
    }
    QSignalSpy errorOccurred(&player, &Player::errorOccurred);

    player.openUri(QUrl::fromLocalFile("/tmp/file.mp3"));
    const auto openUriCalls = getPlayerCalls("OpenUriExtended");
    QCOMPARE(openUriCalls.count(), 1);
    const QVariantList openUriArgs = openUriCalls[0].args();
    Player::Headers dbusHeaders = qdbus_cast<Player::Headers>(openUriArgs[1]);
    QCOMPARE(openUriArgs[0], "file:///tmp/file.mp3");
    QVERIFY(dbusHeaders.isEmpty());

    if (!errorName.isEmpty()) {
        QTRY_COMPARE(errorOccurred.count(), 1);
        const Error error = errorOccurred.at(0).at(0).value<Error>();
        QVERIFY(error.isError());
        QCOMPARE(int(error.code()), expectedCode);
        QCOMPARE(error.message(), errorMessage);
    } else {
        QTest::qWait(5);
        QCOMPARE(errorOccurred.count(), 0);
    }
}

void TestClient::testOpenUriWithHeaders()
{
    Player player;
    Player::Headers headers {
        { "User-Agent", "The Test" },
        { "Cookie", "Why not" },
    };
    player.openUri(QUrl("http://example.com/file.mp3"), headers);
    const auto openUriCalls = getPlayerCalls("OpenUriExtended");
    QCOMPARE(openUriCalls.count(), 1);
    const QVariantList openUriArgs = openUriCalls[0].args();
    Player::Headers dbusHeaders = qdbus_cast<Player::Headers>(openUriArgs[1]);
    QCOMPARE(openUriArgs[0], "http://example.com/file.mp3");
    QCOMPARE(dbusHeaders, headers);
}

void TestClient::testPlayerMethods()
{
    Player player;

    player.goToNext();
    QCOMPARE(getPlayerCalls("Next").count(), 1);

    player.goToPrevious();
    QCOMPARE(getPlayerCalls("Previous").count(), 1);

    player.play();
    QCOMPARE(getPlayerCalls("Play").count(), 1);

    player.pause();
    QCOMPARE(getPlayerCalls("Pause").count(), 1);

    player.stop();
    QCOMPARE(getPlayerCalls("Stop").count(), 1);

    player.seekTo(7654321);
    auto calls = getPlayerCalls("Seek");
    QCOMPARE(calls.count(), 1);
    QCOMPARE(calls[0].args(), QVariantList { quint64(7654321) });
}

void TestClient::testPlayerSignals()
{
    Player player;

    QSignalSpy seekedTo(&player, &Player::seekedTo);
    QSignalSpy aboutToFinish(&player, &Player::aboutToFinish);
    QSignalSpy endOfStream(&player, &Player::endOfStream);
    QSignalSpy videoDimensionChanged(&player, &Player::videoDimensionChanged);
    QSignalSpy errorOccurred(&player, &Player::errorOccurred);
    QSignalSpy bufferingChanged(&player, &Player::bufferingChanged);

    QCOMPARE(seekedTo.count(), 0);
    emitPlayerSignal("Seeked", "t", { quint64(654321) });
    QTRY_COMPARE(seekedTo.count(), 1);
    QCOMPARE(seekedTo.at(0).at(0).toULongLong(), 654321ULL);

    QCOMPARE(aboutToFinish.count(), 0);
    emitPlayerSignal("AboutToFinish", "", {});
    QTRY_COMPARE(aboutToFinish.count(), 1);

    QCOMPARE(endOfStream.count(), 0);
    emitPlayerSignal("EndOfStream", "", {});
    QTRY_COMPARE(endOfStream.count(), 1);

    QCOMPARE(videoDimensionChanged.count(), 0);
    emitPlayerSignal("VideoDimensionChanged", "uu",
                     { quint32(3), quint32(4) });
    QTRY_COMPARE(videoDimensionChanged.count(), 1);
    QCOMPARE(videoDimensionChanged.at(0).at(0).toSize(), QSize(4, 3));

    QCOMPARE(errorOccurred.count(), 0);
    emitPlayerSignal("Error", "q", { DBusConstants::NetworkError });
    QTRY_COMPARE(errorOccurred.count(), 1);
    Error expectedError(Error::NetworkError, "A network error occurred.");
    QCOMPARE(errorOccurred.at(0).at(0), QVariant::fromValue(expectedError));

    QCOMPARE(bufferingChanged.count(), 0);
    emitPlayerSignal("Buffering", "i", { int(32) });
    QTRY_COMPARE(bufferingChanged.count(), 1);
    QCOMPARE(bufferingChanged.at(0).at(0).toInt(), 32);
}

void TestClient::testTracklistConstructor()
{
    TrackList track1;
    QScopedPointer<TrackList> track2(new TrackList(this));
}

void TestClient::testEmptyTracklist()
{
    TrackList track;
    QCOMPARE(track.canEditTracks(), true);
    QCOMPARE(track.currentTrack(), -1);
    QVERIFY(track.tracks().isEmpty());
}

void TestClient::testOfflineTracklist()
{
    QTest::ignoreMessage(QtWarningMsg,
                         "Operating on disconnected playlist not supported!");

    TrackList track;
    track.addTracksWithUriAt(QVector<QUrl> {
        QUrl::fromLocalFile("/tmp/song1.mp3"),
        QUrl::fromLocalFile("/tmp/song2.mp3"),
        QUrl::fromLocalFile("/tmp/song3.mp3"),
        QUrl::fromLocalFile("/tmp/song4.mp3"),
    }, -1);
}

void TestClient::testTracklistAddSingle()
{
    Player player;
    TrackList trackList;
    QSignalSpy tracksAdded(&trackList, &TrackList::tracksAdded);

    player.setTrackList(&trackList);
    QCOMPARE(player.trackList(), &trackList);

    trackList.addTrackWithUriAt(QUrl("http://me.com/song.mp3"), 0, true);
    const auto calls = getTrackListCalls("AddTrack");
    QCOMPARE(calls.count(), 1);
    QVariantList expectedArgs {
        "http://me.com/song.mp3",
        "/org/mpris/MediaPlayer2/TrackList/NoTrack",
        true,
    };
    QCOMPARE(calls[0].args(), expectedArgs);
    QCOMPARE(tracksAdded.count(), 0);

    m_mediaHub->trackListMock().EmitSignal(MPRIS_TRACKLIST_INTERFACE,
                                           "TrackAdded", "s",
                                           { "/track/id/1" });
    QVERIFY(tracksAdded.wait());
    QCOMPARE(tracksAdded[0][0].toInt(), 0);
    QCOMPARE(tracksAdded[0][1].toInt(), 0);

    QCOMPARE(trackList.tracks().count(), 1);
    QCOMPARE(trackList.tracks()[0].uri(), QUrl("http://me.com/song.mp3"));
}

void TestClient::testTracklistEditing()
{
    Player player;
    TrackList trackList;
    QSignalSpy tracksAdded(&trackList, &TrackList::tracksAdded);
    QSignalSpy trackRemoved(&trackList, &TrackList::trackRemoved);
    QSignalSpy trackMoved(&trackList, &TrackList::trackMoved);
    QSignalSpy trackListReset(&trackList, &TrackList::trackListReset);

    player.setTrackList(&trackList);
    QCOMPARE(player.trackList(), &trackList);

    QCOMPARE(trackList.canEditTracks(), true);

    trackList.addTracksWithUriAt({
        QUrl("http://me.com/song1.mp3"),
        QUrl("http://me.com/song2.mp3"),
        QUrl("http://me.com/song3.mp3"),
        QUrl("http://me.com/song4.mp3"),
    }, 0);
    auto calls = getTrackListCalls("AddTracks");
    QCOMPARE(calls.count(), 1);
    QVariantList expectedArgs {
        QVariantList {
            "http://me.com/song1.mp3",
            "http://me.com/song2.mp3",
            "http://me.com/song3.mp3",
            "http://me.com/song4.mp3",
        },
        "/org/mpris/MediaPlayer2/TrackList/NoTrack",
    };
    QCOMPARE(calls[0].args(), expectedArgs);
    QCOMPARE(tracksAdded.count(), 0);

    QStringList ids {
        "/track/id/1",
        "/track/id/2",
        "/track/id/3",
        "/track/id/4",
    };
    m_mediaHub->trackListMock().EmitSignal(MPRIS_TRACKLIST_INTERFACE,
                                           "TracksAdded", "as", { ids });
    QVERIFY(tracksAdded.wait());
    QCOMPARE(tracksAdded[0][0].toInt(), 0);
    QCOMPARE(tracksAdded[0][1].toInt(), 3);

    QCOMPARE(trackList.tracks().count(), 4);
    QCOMPARE(trackList.tracks()[0].uri(), QUrl("http://me.com/song1.mp3"));
    QCOMPARE(trackList.tracks()[3].uri(), QUrl("http://me.com/song4.mp3"));

    /*
     * Test reordering
     */
    trackList.moveTrack(1, 0);
    calls = getTrackListCalls("MoveTrack");
    QCOMPARE(calls.count(), 1);
    expectedArgs = QVariantList {
        "/track/id/2",
        "/track/id/1",
    };
    QCOMPARE(calls[0].args(), expectedArgs);

    m_mediaHub->trackListMock().EmitSignal(MPRIS_TRACKLIST_INTERFACE,
                                           "TrackMoved", "ss", {
        "/track/id/2",
        "/track/id/1",
    });
    QVERIFY(trackMoved.wait());
    QCOMPARE(trackMoved[0][0].toInt(), 1);
    QCOMPARE(trackMoved[0][1].toInt(), 0);
    // Verify that they actually moved
    QCOMPARE(trackList.tracks()[0].uri(), QUrl("http://me.com/song2.mp3"));
    QCOMPARE(trackList.tracks()[1].uri(), QUrl("http://me.com/song1.mp3"));

    /*
     * Test deletion
     */
    trackList.removeTrack(2);
    calls = getTrackListCalls("RemoveTrack");
    QCOMPARE(calls.count(), 1);
    expectedArgs = QVariantList {
        "/track/id/3",
    };
    QCOMPARE(calls[0].args(), expectedArgs);

    m_mediaHub->trackListMock().EmitSignal(MPRIS_TRACKLIST_INTERFACE,
                                           "TrackRemoved", "s", {
        "/track/id/3",
    });
    QVERIFY(trackRemoved.wait());
    QCOMPARE(trackRemoved[0][0].toInt(), 2);
    // Verify that it was actually moved
    QCOMPARE(trackList.tracks()[0].uri(), QUrl("http://me.com/song2.mp3"));
    QCOMPARE(trackList.tracks()[1].uri(), QUrl("http://me.com/song1.mp3"));
    QCOMPARE(trackList.tracks()[2].uri(), QUrl("http://me.com/song4.mp3"));

    /*
     * Test reset
     */
    trackList.reset();
    calls = getTrackListCalls("Reset");
    QCOMPARE(calls.count(), 1);
    QVERIFY(calls[0].args().isEmpty());

    m_mediaHub->trackListMock().EmitSignal(MPRIS_TRACKLIST_INTERFACE,
                                           "TrackListReset", "", {});
    QVERIFY(trackListReset.wait());
    QCOMPARE(trackListReset.count(), 1);
    // Verify that it was actually reset
    QCOMPARE(trackList.tracks().count(), 0);
    QCOMPARE(trackList.currentTrack(), -1);
}

void TestClient::testCurrentTrack()
{
    Player player;
    TrackList trackList;
    QSignalSpy tracksAdded(&trackList, &TrackList::tracksAdded);
    QSignalSpy currentTrackChanged(&trackList, &TrackList::currentTrackChanged);

    player.setTrackList(&trackList);

    trackList.addTracksWithUriAt({
        QUrl("http://me.com/song1.mp3"),
        QUrl("http://me.com/song2.mp3"),
        QUrl("http://me.com/song3.mp3"),
        QUrl("http://me.com/song4.mp3"),
    }, 0);

    QStringList ids {
        "/track/id/0",
        "/track/id/1",
        "/track/id/2",
        "/track/id/3",
    };
    m_mediaHub->trackListMock().EmitSignal(MPRIS_TRACKLIST_INTERFACE,
                                           "TracksAdded", "as", { ids });
    QVERIFY(tracksAdded.wait());
    QCOMPARE(trackList.tracks().count(), 4);

    /* Change current track */
    trackList.goTo(1);
    auto calls = getTrackListCalls("GoTo");
    QCOMPARE(calls.count(), 1);
    QCOMPARE(calls[0].args(), QVariantList { "/track/id/1" });

    QCOMPARE(currentTrackChanged.count(), 1);
    QCOMPARE(trackList.currentTrack(), 1);

    /* Now let the service emit a spontaneous signal */
    m_mediaHub->trackListMock().EmitSignal(MPRIS_TRACKLIST_INTERFACE,
                                           "TrackChanged", "s",
                                           { "/track/id/3" });
    QVERIFY(currentTrackChanged.wait());
    QCOMPARE(trackList.currentTrack(), 3);
}

void TestClient::testVideoSink()
{
    const uint32_t textureId = 0xabcd1234;

    Player player;
    VideoSink &videoSink = player.createGLTextureVideoSink(textureId);
    QCOMPARE(videoSink.transformationMatrix(), QMatrix4x4());
    QVERIFY(videoSink.swapBuffers());

    auto calls = getPlayerCalls("CreateVideoSink");
    QCOMPARE(calls.count(), 1);
    QCOMPARE(calls[0].args(), QVariantList { quint32(textureId) });
}

QTEST_GUILESS_MAIN(TestClient)

#include "test_client.moc"
