import os
import re
import subprocess
import sys

from gi.repository import GLib
from time import sleep

import dbus
import pytest

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

    def test_http_headers(self, bus_obj, dbus_monitor, media_hub_service_full):
        from http.server import HTTPServer, BaseHTTPRequestHandler
        request = None

        class HttpHandler(BaseHTTPRequestHandler):
            def __init(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            def do_GET(self):
                print('HTTP server captured request!')
                nonlocal request
                request = self

        httpd = HTTPServer(('', 8000), HttpHandler)

        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'http://localhost:8000/some-file.mp3'
        player.open_uri_extended(audio_file, { 'Cookie': 'biscuit', 'User-Agent': 'MediaHub' })
        player.play()
        httpd.handle_request()
        assert request.path == '/some-file.mp3'
        assert 'Cookie' in request.headers
        assert request.headers['Cookie'] == 'biscuit'
        assert request.headers['User-Agent'] == 'MediaHub'

    def test_http_authorization(self, bus_obj, media_hub_service_full):
        from http.server import HTTPServer, BaseHTTPRequestHandler
        request = None

        class HttpHandler(BaseHTTPRequestHandler):
            def __init(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            def do_GET(self):
                print('HTTP server captured GET request!')
                nonlocal request
                request = self
                self.send_response(401)
                self.send_header('WWW-Authenticate', 'Basic realm="mp3z"')
                self.end_headers()

        httpd = HTTPServer(('', 8000), HttpHandler)

        media_hub = MediaHub.Service(bus_obj)
        (object_path, uuid) = media_hub.create_session()
        player = MediaHub.Player(bus_obj, object_path)

        audio_file = 'http://localhost:8000/some-file.mp3'
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
