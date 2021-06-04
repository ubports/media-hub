import dbus

from dbus.mainloop.glib import DBusGMainLoop
from time import sleep

import MediaHub


class Client:
    """ This is a client app to test that the media-hub properly monitors its
    clients' lifetime"""
    def __init__(self):
        DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SessionBus()
        self.media_hub = MediaHub.Service(self.bus)

    def run(self):
        while True:
            try:
                line = input()
            except EOFError as error:
                break
            line = line.strip()
            if line.startswith('play '):
                url = line[5:]
                (object_path, uuid) = self.media_hub.create_session()
                player = MediaHub.Player(self.bus, object_path)
                player.open_uri(url)
                player.play()
                print('OK')
            # Handle other commands here:
            # elif line.startswith(...):


def main():
    client = Client()
    client.run()


if __name__ == "__main__":
    main()
