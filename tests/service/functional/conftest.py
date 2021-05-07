import os
import os.path
import sys
import subprocess

from pathlib import Path
from subprocess import Popen
from time import sleep

import dbus
import dbus.mainloop.glib
import dbusmock
import pytest

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)


@pytest.fixture(scope="function")
def media_hub_service(request):
    """ Spawn a new media-hub service instance
    """
    service_name = "core.ubuntu.media.Service"
    bus = dbus.SessionBus()

    if 'SERVICE_BINARY' not in os.environ:
        print('"SERVICE_BINARY" environment variable must be set to full'
              'path of service executable.')

        sys.exit(1)

    environment = os.environ.copy()
    environment['MEDIA_HUB_MOCKED_DBUS'] = \
        "mock.org.freedesktop.dbus"
    environment['CORE_UBUNTU_MEDIA_SERVICE_AUDIO_SINK_NAME'] = 'fakesink'
    environment['CORE_UBUNTU_MEDIA_SERVICE_VIDEO_SINK_NAME'] = 'fakesink'

    # Spawn the service, and wait for it to appear on the bus
    args = [os.environ['SERVICE_BINARY']]
    if 'WRAPPER' in os.environ:
        args = os.environ['WRAPPER'].split() + args
    service = Popen(
        args,
        stdout=sys.stdout,
        stderr=sys.stderr,
        env=environment)

    for i in range(0, 100):
        if service.poll() is not None or bus.name_has_owner(service_name):
            break
        sleep(0.1)

    assert service.poll() is None, "service is not running"
    assert bus.name_has_owner(service_name)

    def teardown():
        service.terminate()
        service.wait()
    request.addfinalizer(teardown)

    return service


@pytest.fixture(scope="session")
def current_path():
    return Path(__file__).parent


@pytest.fixture(scope="session")
def data_path(request, current_path):
    return current_path.parent.joinpath('data')


@pytest.fixture(scope="function")
def service_name():
    return "core.ubuntu.media.Service"


@pytest.fixture(scope="function")
def request_name(request, service_name):
    """ Occupy the service bus name

    This is intended as a helper to test how the service behaves when its
    bus name is already taken.
    """

    bus = dbus.SessionBus()

    def teardown():
        # Make sure the name is no longer there
        bus.release_name(service_name)
    request.addfinalizer(teardown)

    bus.request_name(service_name, dbus.bus.NAME_FLAG_REPLACE_EXISTING)


@pytest.fixture(scope="function")
def dbus_monitor(request, bus_obj):
    """Add this fixture to your test to debug it"""
    monitor = Popen(['/usr/bin/dbus-monitor', '--session'], stdout=sys.stdout)
    sleep(1)

    def teardown():
        monitor.terminate()
        monitor.wait()
    request.addfinalizer(teardown)


@pytest.fixture(scope="function")
def apparmor_reply(request):
    return 'ret = { "LinuxSecurityLabel": "unconfined" }'


@pytest.fixture(scope="function")
def dbus_apparmor(request, bus_obj, apparmor_reply):
    """Mocks the D-Bus GetConnectionCredentials method"""
    bus_name = 'mock.org.freedesktop.dbus'
    object_path = '/org/freedesktop/DBus'
    mock = dbusmock.DBusTestCase.spawn_server(bus_name,
                                              object_path,
                                              'org.freedesktop.DBus',
                                              system_bus=False)
    mock_iface = dbus.Interface(bus_obj.get_object(bus_name, object_path),
                                dbusmock.MOCK_IFACE)
    mock_iface.AddMethod('org.freedesktop.DBus',
                         'GetConnectionCredentials', 's', 'a{sv}',
                         apparmor_reply)

    def teardown():
        mock.terminate()
        mock.wait()
    request.addfinalizer(teardown)

    return mock_iface


@pytest.fixture(scope="function")
def telepathy(request, bus_obj):
    """Mocks the Telepathy AccountManager service"""
    bus_name = 'org.freedesktop.Telepathy.AccountManager'
    object_path = '/org/freedesktop/Telepathy/AccountManager'
    mock = dbusmock.DBusTestCase.spawn_server(
            bus_name,
            object_path,
            'org.freedesktop.Telepathy.AccountManager',
            system_bus=False)
    mock_iface = dbus.Interface(bus_obj.get_object(bus_name, object_path),
                                dbusmock.MOCK_IFACE)

    def teardown():
        mock.terminate()
        mock.wait()
    request.addfinalizer(teardown)

    return mock_iface


@pytest.fixture(scope="function")
def media_hub_service_full(request, dbus_apparmor, telepathy,
                           media_hub_service):
    return media_hub_service


@pytest.fixture(scope="session")
def bus_obj(request):
    """ Create a new SystemBus

    The bus is returned, and can be used to interface with the service
    """
    start_dbus_daemon_command = [
        "dbus-daemon",
        "--session",
        "--nofork",
        "--print-address"
    ]

    dbus_daemon = Popen(
        start_dbus_daemon_command,
        shell=False,
        stdout=subprocess.PIPE)

    # Read the path of the newly created socket from STDOUT of
    # dbus-daemon, and set up the environment
    line = dbus_daemon.stdout.readline().decode('utf8')
    dbus_address = line.strip().split(",")[0]
    os.environ["DBUS_SESSION_BUS_ADDRESS"] = dbus_address
    # The PowerStateController uses the System bus, let it be found at the same
    # address
    os.environ["DBUS_SYSTEM_BUS_ADDRESS"] = dbus_address
    bus = dbus.SessionBus()

    def teardown():
        bus.close()

        dbus_daemon.kill()
        dbus_daemon.wait()

    request.addfinalizer(teardown)
    return bus
