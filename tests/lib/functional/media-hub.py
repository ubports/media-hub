'''media-hub mock template

This creates the expected methods and properties of the
core.ubuntu.media.Service service.
'''

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.  See http://www.gnu.org/copyleft/lgpl.html for the full text
# of the license.

__author__ = 'Alberto Mardegan'
__email__ = 'mardy@users.sourceforge.net'
__copyright__ = '(c) 2021 UBports Foundation'
__license__ = 'LGPL 3+'

import dbus

from dbusmock import MOCK_IFACE
import dbusmock

BUS_NAME = 'core.ubuntu.media.Service'
MAIN_OBJ = '/core/ubuntu/media/Service'
MEDIAHUB_SERVICE_IFACE = 'core.ubuntu.media.Service'
MAIN_IFACE = MEDIAHUB_SERVICE_IFACE
MPRIS_PLAYER_INTERFACE = 'org.mpris.MediaPlayer2.Player'
MPRIS_TRACKLIST_INTERFACE = 'org.mpris.MediaPlayer2.TrackList'
SYSTEM_BUS = False

#
# D-Bus errors
#
MPRIS_ERROR_PREFIX = 'mpris.Player.Error.'
MPRIS_ERROR_CREATING_SESSION = MPRIS_ERROR_PREFIX + 'CreatingSession'
MPRIS_ERROR_DESTROYING_SESSION = MPRIS_ERROR_PREFIX + 'DestroyingSession'
MPRIS_ERROR_INSUFFICIENT_APPARMOR_PERMISSIONS = \
        MPRIS_ERROR_PREFIX + 'InsufficientAppArmorPermissions'
MPRIS_ERROR_OOP_STREAMING_NOT_SUPPORTED = \
        MPRIS_ERROR_PREFIX + 'OutOfProcessBufferStreamingNotSupported'
MPRIS_ERROR_URI_NOT_FOUND = MPRIS_ERROR_PREFIX + 'UriNotFound'


def create_session(self):
    player_path = '/player/' + str(self.next_player_id)
    player_uuid = 'uuid_' + str(self.next_player_id)
    self.next_player_id += 1
    props = {
        'CanPlay': False,
        'CanPause': False,
        'CanSeek': False,
        'CanGoPrevious': False,
        'CanGoNext': False,
        'IsVideoSource': False,
        'IsAudioSource': False,
        'PlaybackStatus': 'Stopped',
        'PlaybackRate': 1.0,
        'Volume': 1.0,
        'Shuffle': False,
        'LoopStatus': 'None',
        'AudioStreamRole': 2,
        'Metadata': dbus.Array([], signature='a{sv}'),
    }
    props.update(self.player_properties_override)
    if self.player_properties_override_empty:
        props = {}
    self.player_properties_override = {}
    self.player_properties_override_empty = False

    methods = [
        ('Key', '', 'u', 'ret = 0xdeadbeef'),
        ('OpenUriExtended', 'sa{ss}', 'b', 'ret = self.open_uri_extended(self, *args)'),
        ('Next', '', '', ''),
        ('Previous', '', '', ''),
        ('Play', '', '', ''),
        ('Pause', '', '', ''),
        ('Stop', '', '', ''),
        ('Seek', 't', '', ''),
        ('CreateVideoSink', 'u', '', ''),
    ]
    self.AddObject(player_path, MPRIS_PLAYER_INTERFACE, props, methods)
    player = dbusmock.get_object(player_path)
    player.uuid = player_uuid
    player.open_uri_error = None
    player.open_uri_extended = open_uri_extended

    create_track_list(self, player_path)

    self.last_player_path = player_path
    return (player_path, player_uuid)


def create_track_list(self, player_path):
    track_list_path = player_path + '/TrackList'
    props = {
        'CanEditTracks': True,
    }
    methods = [
        ('AddTrack', 'ssb', '', ''),
        ('AddTracks', 'ass', '', ''),
        ('MoveTrack', 'ss', '', ''),
        ('RemoveTrack', 's', '', ''),
        ('Reset', '', '', ''),
        ('GoTo', 's', '', 'self.go_to(self, *args)'),
    ]
    self.AddObject(track_list_path, MPRIS_TRACKLIST_INTERFACE, props, methods)
    track_list = dbusmock.get_object(track_list_path)
    track_list.go_to = go_to


def destroy_session(self, uuid):
    pass


def open_uri_extended(self, uri, headers):
    if self.open_uri_error:
        raise dbus.exceptions.DBusException(self.open_uri_error_text,
                                            name=self.open_uri_error)
    return True


def go_to(self, track_id):
    self.EmitSignal(MPRIS_TRACKLIST_INTERFACE, 'TrackChanged', 's', (track_id,))


def load(mock, parameters):
    mock.last_player_path = ''
    mock.next_player_id = 1
    mock.player_properties_override = {}
    mock.player_properties_override_empty = False

    mock.create_session = create_session
    mock.destroy_session = destroy_session
    mock.AddMethods(MEDIAHUB_SERVICE_IFACE, [
        ('CreateSession', '', 'os', 'ret = self.create_session(self)'),
        ('DestroySession', 's', '', 'ret = self.destroy_session(self, args[0])'),
    ])


@dbus.service.method(MOCK_IFACE, in_signature='a{sv}', out_signature='')
def SetNextPlayerProperties(self, properties):
    if not properties:
        self.player_properties_override_empty = True
    else:
        self.player_properties_override = properties


@dbus.service.method(MOCK_IFACE, in_signature='', out_signature='s')
def GetLastPlayerPath(self):
    return self.last_player_path


@dbus.service.method(MOCK_IFACE, in_signature='s', out_signature='s')
def GetPlayerUuid(self, path):
    return dbusmock.get_object(path).uuid


@dbus.service.method(MOCK_IFACE, in_signature='ss', out_signature='')
def SetOpenUriError(self, error, message):
    player = dbusmock.get_object(self.last_player_path)
    player.open_uri_error = error
    player.open_uri_error_text = message
