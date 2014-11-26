pkg_check_modules(PC_GSTREAMER_1_0 REQUIRED gstreamer-1.0)
pkg_check_modules(PC_PULSE_AUDIO REQUIRED libpulse)
include_directories(${PC_GSTREAMER_1_0_INCLUDE_DIRS} ${HYBRIS_MEDIA_CFLAGS} ${PC_PULSE_AUDIO_INCLUDE_DIRS})

# We compile with all symbols visible by default. For the shipping library, we strip
# out all symbols that are not in core::ubuntu::media*
set(symbol_map "${CMAKE_SOURCE_DIR}/symbols.map")

file(GLOB_RECURSE MEDIA_HUB_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB MPRIS_HEADERS mpris/*.h)

message(STATUS ${MEDIA_HUB_HEADERS})

add_subdirectory(telephony)

add_library(
  media-hub-common SHARED

  the_session_bus.cpp
)

target_link_libraries(
  media-hub-common

  ${Boost_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
)
set_target_properties(
  media-hub-common

  PROPERTIES
  VERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}.${UBUNTU_MEDIA_HUB_VERSION_MINOR}.${UBUNTU_MEDIA_HUB_VERSION_PATCH}
  SOVERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}
  LINK_FLAGS "${ldflags} -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

add_library(
  media-hub-client SHARED

  ${MEDIA_HUB_HEADERS}

  player.cpp
  service.cpp
  track.cpp
  track_list.cpp

  player_stub.cpp
  service_stub.cpp
  track_list_stub.cpp

  video/hybris_gl_sink.cpp
  video/platform_default_sink.cpp
)

set_target_properties(
  media-hub-client

  PROPERTIES
  VERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}.${UBUNTU_MEDIA_HUB_VERSION_MINOR}.${UBUNTU_MEDIA_HUB_VERSION_PATCH}
  SOVERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}
  LINK_FLAGS "${ldflags} -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

target_link_libraries(
  media-hub-client

  media-hub-common

  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
  ${GLog_LIBRARY}
  ${HYBRIS_MEDIA_LIBRARIES}
)

install(
  TARGETS media-hub-client media-hub-common
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_library(
    media-hub-service

    ${MEDIA_HUB_HEADERS}
    ${MPRIS_HEADERS}

    client_death_observer.cpp
    hybris_client_death_observer.cpp
    cover_art_resolver.cpp
    engine.cpp

    apparmor/context.cpp
    apparmor/ubuntu.cpp

    audio/pulse_audio_output_observer.cpp
    audio/output_observer.cpp

    power/battery_observer.cpp
    power/state_controller.cpp

    recorder_observer.cpp
    hybris_recorder_observer.cpp

    gstreamer/engine.cpp
    gstreamer/playbin.cpp

    player_skeleton.cpp
    player_implementation.cpp
    service_skeleton.cpp
    service_implementation.cpp
    track_list_skeleton.cpp
    track_list_implementation.cpp
)

target_link_libraries(
  media-hub-service

  media-hub-client
  media-hub-common
  call-monitor
  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
  ${GLog_LIBRARY}
  ${PC_GSTREAMER_1_0_LIBRARIES}
  ${PROCESS_CPP_LDFLAGS}
  ${GIO_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${PC_PULSE_AUDIO_LIBRARIES}
)

include_directories(${PROJECT_SOURCE_DIR}/src/ ${HYBRIS_MEDIA_CFLAGS})

add_executable(
  media-hub-server

  server/server.cpp
)

target_link_libraries(
  media-hub-server

  media-hub-service
  media-hub-client
  call-monitor

  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
  ${GLog_LIBRARY}
  ${PC_GSTREAMER_1_0_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
)

install(
  TARGETS media-hub-server
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
