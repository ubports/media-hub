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

        # Watch for PlaybackStatusChanged:
        player = MediaHub.MprisPlayer(bus_obj)
        loop = GLib.MainLoop()
        playback_statuses = []

        def propertis_changed_cb(interface, changed, invalidated):
            if 'PlaybackStatus' not in changed:
                return
            playback_status = str(changed['PlaybackStatus'])
            playback_statuses.append(playback_status)
            if len(playback_statuses) > 2 and playback_status == 'Stopped':
                loop.quit()
        player.on_properties_changed(propertis_changed_cb)

        # Wait for the signal up to 3 seconds
        timer_id = GLib.timeout_add(3000, loop.quit)
        loop.run()
        GLib.source_remove(timer_id)
        assert 'Playing' in playback_statuses
        assert 'Stopped' in playback_statuses
        playing_index = playback_statuses.index('Playing')
        assert playback_statuses[playing_index + 1] == 'Stopped'

    @pytest.mark.skipif('True')
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

        # Watch for PlaybackStatusChanged:
        player = MediaHub.MprisPlayer(bus_obj)
        loop = GLib.MainLoop()
        playback_statuses = []

        def propertis_changed_cb(interface, changed, invalidated):
            if 'PlaybackStatus' not in changed:
                return
            playback_status = str(changed['PlaybackStatus'])
            playback_statuses.append(playback_status)
            if len(playback_statuses) > 2 and playback_status == 'Stopped':
                print('Quitting main loop')
                loop.quit()
        player.on_properties_changed(propertis_changed_cb)

        # Tell the client to play a song
        audio_file = 'file://' + str(data_path.joinpath('test-audio-1.ogg'))
        writeline(client.stdin, 'play {}'.format(audio_file))
        line = None
        while line != "OK":
            line = readline(client.stdout)

        # Wait a bit, then kill the client
        sleep(0.5)
        client.stdin.close()
        client.wait(3)

        # Verify that the file is no longer playing
        # Wait for the signal up to 3 seconds
        timer_id = GLib.timeout_add(3000, loop.quit)
        loop.run()
        GLib.source_remove(timer_id)

        assert 'Playing' in playback_statuses
        assert 'Stopped' in playback_statuses
        playing_index = playback_statuses.index('Playing')
        assert playback_statuses[playing_index + 1] == 'Stopped'
