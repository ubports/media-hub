import dbus


ServiceName = 'core.ubuntu.media.Service'


class Error:
    CreatingSession = 'core.ubuntu.media.Service.Error.CreatingSession'
    DetachingSession = 'core.ubuntu.media.Service.Error.DetachingSession'
    ReattachingSession = 'core.ubuntu.media.Service.Error.ReattachingSession'
    DestroyingSession = 'core.ubuntu.media.Service.Error.DestroyingSession'
    CreatingFixedSession = 'core.ubuntu.media.Service.Error.CreatingFixedSession'
    ResumingSession = 'core.ubuntu.media.Service.Error.ResumingSession'
    PlayerKeyNotFound = 'core.ubuntu.media.Service.Error.PlayerKeyNotFound'
    PermissionDenied = 'mpris.Player.Error.InsufficientAppArmorPermissions'


class Service(object):
    """ Wrapper of the dbus interface of the media-hub service """

    def __init__(self, bus_obj):
        service_name = ServiceName
        object_path = '/core/ubuntu/media/Service'
        main_object = bus_obj.get_object(service_name, object_path)
        self.__service = dbus.Interface(
            main_object,
            'core.ubuntu.media.Service')

    def create_session(self):
        return self.__service.CreateSession()

    def detach_session(self, uuid):
        return self.__service.DetachSession(uuid)

    def reattach_session(self, uuid):
        return self.__service.ReattachSession(uuid)

    def destroy_session(self, uuid):
        return self.__service.DestroySession(uuid)

    def create_fixed_session(self, name):
        return self.__service.CreateFixedSession(name)

    def resume_session(self, key):
        return self.__service.ResumeSession(key)

    def pause_other_sessions(self, key):
        return self.__service.PauseOtherSessions(key)


class Player(object):

    def __init__(self, bus_obj, object_path):
        service_name = ServiceName
        session = bus_obj.get_object(service_name, object_path,
                                     introspect=False)
        self.__player = dbus.Interface(
            session,
            'org.mpris.MediaPlayer2.Player')
        self.__properties = dbus.Interface(
            session,
            'org.freedesktop.DBus.Properties')

    def open_uri(self, uri):
        return self.__player.OpenUri(uri)

    def play(self):
        self.__player.Play()

    def on_properties_changed(self, callback):
        self.__properties.connect_to_signal('PropertiesChanged', callback)

    def on_playback_status_changed(self, callback):
        self.__player.connect_to_signal('PlaybackStatusChanged', callback)


class MprisPlayer(Player):

    def __init__(self, bus_obj):
        super().__init__(bus_obj, '/org/mpris/MediaPlayer2')
