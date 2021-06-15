import os
import re
import subprocess
import sys

from gi.repository import GLib
from time import sleep

import dbus
import pytest

import HttpServer
import MediaHub


class TestMediaHub:

    def test_no_dbus(self):
        environment = os.environ.copy()
        environment["DBUS_SESSION_BUS_ADDRESS"] = ""
        environment["DBUS_SYSTEM_BUS_ADDRESS"] = ""
        service = subprocess.Popen(
            os.environ["SERVICE_BINARY"],
            stdout=sys.stdout,
            stderr=sys.stderr,
            env=environment)
        service.wait(5)
        assert service.returncode == 1

    @pytest.mark.parametrize('service_name', [
        ('core.ubuntu.media.Service'),
        ('org.mpris.MediaPlayer2.MediaHub'),
    ])
    def test_steal_name(self, bus_obj, service_name, request_name):
        service = subprocess.Popen(
            os.environ["SERVICE_BINARY"],
            stderr=subprocess.PIPE,)
        service.wait(5)
        output = service.stderr.read()
        assert b'Failed to register' in output
        assert service.returncode == 1

    def test_create_session(self, bus_obj, media_hub_service):
        media_hub = MediaHub.Service(bus_obj)

        (object_path, uuid) = media_hub.create_session()
        (base_path, number) = object_path.rsplit('/', 1)
        assert base_path == '/core/ubuntu/media/Service/sessions'
        assert number.isdigit()

    def test_play_audio(
            self, bus_obj, media_hub_service_full, current_path,
            data_path):
        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'file://' + str(data_path.joinpath('test-audio.ogg'))
        player.open_uri(audio_file)
        player.play()

        assert player.wait_for_prop('PlaybackStatus', 'Playing')
        assert player.wait_for_prop('PlaybackStatus', 'Stopped')
        assert player.wait_for_signal('EndOfStream')

        # Check that powerd was invoked
        powerd = media_hub_service_full.powerd
        calls = powerd.GetMethodCalls('requestSysState')
        assert len(calls) == 1
        args = calls[0][1]
        assert args[0] == "media-hub-playback_lock"
        assert args[1] == 1
        for i in range(0, 50):
            calls = powerd.GetMethodCalls('clearSysState')
            if calls: break
            sleep(0.1)
        assert len(calls) == 1
        args = calls[0][1]
        assert args[0] == "powerd-cookie"

    @pytest.mark.parametrize('apparmor_reply', [
        ('ret = { "LinuxSecurityLabel": "my_app_1.0"}'),
        # Regression test for
        # https://github.com/ubports/media-hub/pull/28#issuecomment-851699674
        ('ret = { "LinuxSecurityLabel": "messaging-app (enforce)"}'),
    ])
    def test_play_audio_unauthorized(
            self, bus_obj, apparmor_reply, media_hub_service_full,
            data_path):
        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'file://' + str(data_path.joinpath('test-audio.ogg'))
        with pytest.raises(dbus.exceptions.DBusException) as exception:
            player.open_uri(audio_file)
        assert exception.value.get_dbus_name() == \
            MediaHub.Error.PermissionDenied

    def test_client_disconnection(
            self, bus_obj, media_hub_service_full, current_path,
            data_path):
        script_path = current_path.joinpath("media_client.py")
        client = subprocess.Popen(
            [sys.executable, str(script_path)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=sys.stderr)

        def writeline(stream, line):
            stream.write(line.encode('utf-8') + b'\n')
            stream.flush()

        def readline(stream):
            return stream.readline().strip().decode('utf-8')

        player = MediaHub.MprisPlayer(bus_obj)

        # Tell the client to play a song
        audio_file = 'file://' + str(data_path.joinpath('test-audio-1.ogg'))
        writeline(client.stdin, 'play {}'.format(audio_file))
        line = None
        while line != "OK":
            line = readline(client.stdout)

        # Wait a bit, then kill the client
        assert player.wait_for_prop('PlaybackStatus', 'Playing')
        client.stdin.close()
        client.wait(3)
        assert player.wait_for_prop('PlaybackStatus', 'Stopped')

    def test_low_battery(
            self, bus_obj, media_hub_service_full, data_path, dbus_monitor):
        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'file://' + str(data_path.joinpath('test-audio-1.ogg'))
        player.open_uri(audio_file)

        player.play()
        assert player.wait_for_prop('PlaybackStatus', 'Playing')

        # Now pretend that the system is in low power
        (battery_mock, battery_ctrl) = media_hub_service_full.battery

        battery_ctrl.set_is_warning(True)
        battery_ctrl.set_power_level('low')

        assert player.wait_for_prop('PlaybackStatus', 'Paused')

    def test_http_headers(self, bus_obj, media_hub_service_full):
        request = None

        class HttpHandler(HttpServer.HttpRequestHandler):
            def do_GET(self):
                print('HTTP server captured request!')
                nonlocal request
                request = self

        httpd = HttpServer.HttpServer(HttpHandler)

        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'http://127.0.0.1:8000/some-file.mp3'
        player.open_uri_extended(audio_file, { 'Cookie': 'biscuit', 'User-Agent': 'MediaHub' })
        player.play()
        httpd.handle_request()
        assert request.path == '/some-file.mp3'
        assert 'Cookie' in request.headers
        assert request.headers['Cookie'] == 'biscuit'
        assert request.headers['User-Agent'] == 'MediaHub'
        httpd.server_close()

    def test_http_authorization(self, bus_obj, media_hub_service_full):
        request = None

        class HttpHandler(HttpServer.HttpRequestHandler):
            def do_GET(self):
                print('HTTP server captured GET request!')
                nonlocal request
                request = self
                self.send_response(401)
                self.send_header('WWW-Authenticate', 'Basic realm="mp3z"')
                self.end_headers()

        httpd = HttpServer.HttpServer(HttpHandler)

        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'http://127.0.0.1:8000/some-file.mp3'
        player.open_uri_extended(audio_file, {
            'Authorization': 'Basic dG9tOlA0c3M6dyVyZCQ=',
        })
        player.play()
        # The first request will send the 401 error, which will cause the
        # GstSoupHTTPSrc element to retry with user and password
        httpd.handle_request()
        # Here we handle the second request
        httpd.handle_request()

        assert request.path == '/some-file.mp3'
        assert 'Authorization' in request.headers
        assert request.headers['Authorization'] == 'Basic dG9tOlA0c3M6dyVyZCQ='
        httpd.server_close()

    @pytest.mark.parametrize('file_name,expected_is_video,expected_is_audio',
        [
            ('test-audio.ogg', False, True),
            ('test-video.ogg', True, False),
        ])
    def test_play_remote_media(self, bus_obj, media_hub_service_full,
                               data_path, file_name,
                               expected_is_video, expected_is_audio):
        request = None

        class HttpHandler(HttpServer.HttpRequestHandler):
            def do_GET(self):
                print('HTTP server captured request!')
                self.send_response(200)
                self.send_header('Content-type', 'application/octet-stream')
                self.send_header('Content-Disposition', 'attachment; filename="song.ogg"')
                self.end_headers()

                audio_file = data_path.joinpath(self.path[1:])
                with audio_file.open('rb') as f:
                    self.wfile.write(f.read())

        httpd = HttpServer.HttpServer(HttpHandler)

        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'http://127.0.0.1:8000/' + file_name
        player.open_uri(audio_file)
        player.play()
        httpd.handle_request()
        assert player.wait_for_prop('PlaybackStatus', 'Playing')
        assert player.get_prop('IsAudioSource') == expected_is_audio
        assert player.get_prop('IsVideoSource') == expected_is_video
        httpd.server_close()

    def test_play_track_list(
            self, bus_obj, media_hub_service_full, current_path,
            data_path, dbus_monitor):
        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)
        track_list = MediaHub.TrackList(player)

        audio_file1 = 'file://' + str(data_path.joinpath('test-audio.ogg'))
        track_list.add_track(audio_file1)
        audio_file2 = 'file://' + str(data_path.joinpath('test-audio-1.ogg'))
        track_list.add_track(audio_file2)

        assert player.wait_for_prop('CanGoNext', True)
        assert player.wait_for_prop('CanGoPrevious', False)
        assert player.get_prop('IsAudioSource') == True

        player.play()

        assert player.wait_for_prop('PlaybackStatus', 'Playing')
        # The first track will finish soon
        assert player.wait_for_signal('AboutToFinish')
        # Next/Prev properties will swap:
        assert player.wait_for_prop('CanGoNext', False)
        assert player.wait_for_prop('CanGoPrevious', True)

    def test_loop(self, bus_obj, media_hub_service_full, data_path):
        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        player.set_prop('LoopStatus', dbus.String('Track', variant_level=1))
        assert player.get_prop('LoopStatus') == 'Track'
        assert player.get_prop('TypedLoopStatus') == 1

        audio_file = 'file://' + str(data_path.joinpath('test-audio.ogg'))
        player.open_uri(audio_file)

        player.play()
        assert player.wait_for_prop('PlaybackStatus', 'Playing')
        assert player.wait_for_signal('AboutToFinish')
        assert player.wait_for_prop('PlaybackStatus', 'Stopped')
        player.clear_signals()
        assert player.wait_for_prop('PlaybackStatus', 'Playing')
        assert player.wait_for_signal('AboutToFinish')
        assert player.wait_for_prop('PlaybackStatus', 'Stopped')
