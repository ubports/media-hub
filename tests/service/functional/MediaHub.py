import dbus

from gi.repository import GLib

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
    Ready = 1
    Playing = 2
    Paused = 3
    Stopped = 4

    def __init__(self, bus_obj, object_path):
        service_name = ServiceName
        session = bus_obj.get_object(service_name, object_path,
                                     introspect=False)
        self.interface_name = 'org.mpris.MediaPlayer2.Player'
        self.__player = dbus.Interface(session, self.interface_name)
        self.__properties = dbus.Interface(
            session,
            'org.freedesktop.DBus.Properties')
        self.props = {}
        self.__prop_callbacks = []

        self.loop = GLib.MainLoop()
        self.__properties.connect_to_signal('PropertiesChanged',
                                            self.__on_properties_changed)

    def __on_properties_changed(self, interface, changed, invalidated):
        assert interface == self.interface_name
        self.props.update(changed)
        for cb in self.__prop_callbacks:
            cb(interface, changed, invalidated)

    def wait_for_prop(self, name, value, timeout=3000):
        if self.props.get(name) == value:
            return True
        timer_id = GLib.timeout_add(timeout, self.loop.quit)
        def check_value(interface, changed, invalidated):
            if self.props.get(name) == value:
                GLib.source_remove(timer_id)
                self.loop.quit()
        self.on_properties_changed(check_value)
        self.loop.run()
        self.unsubscribe_properties_changed(check_value)
        return True if self.props.get(name) == value else False

    def get_prop(self, name):
        return self.__properties.Get(self.interface_name, name)

    def open_uri(self, uri):
        return self.__player.OpenUri(uri)

    def open_uri_extended(self, uri, http_headers):
        return self.__player.OpenUriExtended(uri, http_headers)

    def play(self):
        self.__player.Play()

    def on_properties_changed(self, callback):
        self.__prop_callbacks.append(callback)

    def unsubscribe_properties_changed(self, callback):
        self.__prop_callbacks.remove(callback)


class MprisPlayer(Player):

    def __init__(self, bus_obj):
        super().__init__(bus_obj, '/org/mpris/MediaPlayer2')
