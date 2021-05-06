pkg_check_modules(PC_GSTREAMER_1_0 REQUIRED gstreamer-1.0)
pkg_check_modules(PC_GSTREAMER_PBUTILS_1_0 REQUIRED gstreamer-pbutils-1.0)
pkg_check_modules(PC_PULSE_AUDIO REQUIRED libpulse libpulse-mainloop-glib)
include_directories(${PC_GSTREAMER_1_0_INCLUDE_DIRS} ${HYBRIS_MEDIA_CFLAGS} ${PC_PULSE_AUDIO_INCLUDE_DIRS})

# We compile with all symbols visible by default. For the shipping library, we strip
# out all symbols that are not in core::ubuntu::media*
set(symbol_map "${CMAKE_SOURCE_DIR}/symbols.map")

file(GLOB_RECURSE MEDIA_HUB_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB MPRIS_HEADERS mpris/*.h)

message(STATUS ${MEDIA_HUB_HEADERS})

SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

add_subdirectory(telephony)

add_library(
  media-hub-common SHARED

  backend.cpp
  the_session_bus.cpp
  util/utils.cpp
)

target_link_libraries(
  media-hub-common

  ${Boost_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
  ${PC_GSTREAMER_1_0_LIBRARIES}
  Qt5::Core
)

target_include_directories(media-hub-common PUBLIC
  ${Boost_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${DBUS_CPP_INCLUDE_DIRS}
  ${GLog_INCLUDE_DIR}
  ${PROCESS_CPP_INCLUDE_DIRS}
  ${PROPERTIES_CPP_INCLUDE_DIRS}

  ${PROJECT_SOURCE_DIR}/include/
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

  logger/logger.cpp

  player.cpp
  service.cpp
  track.cpp
  track_list.cpp
  metadata.cpp

  player_stub.cpp
  service_stub.cpp
  track_list_stub.cpp

  video/hybris_gl_sink.cpp
  video/egl_sink.cpp
  video/platform_default_sink.cpp
)

target_include_directories(media-hub-client PUBLIC
  ${PROJECT_SOURCE_DIR}/include/
  ${PROJECT_SOURCE_DIR}/src/
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

  ${Boost_LDFLAGS}
  ${Boost_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
  ${GLog_LIBRARY}
  ${GIO_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${EGL_LIBRARIES}
  ${GL_LIBRARIES}
)

install(
  TARGETS media-hub-client media-hub-common
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_library(
  media-hub-service

  logging.cpp

  dbus_property_notifier.cpp

  client_death_observer.cpp
  hybris_client_death_observer.cpp
  stub_client_death_observer.cpp
  engine.cpp
  track_metadata.cpp

  apparmor/context.cpp
  apparmor/ubuntu.cpp

  audio/pulse_audio_output_observer.cpp
  audio/ostream_reporter.cpp
  audio/output_observer.cpp

  power/battery_observer.cpp
  power/state_controller.cpp

  recorder_observer.cpp
  hybris_recorder_observer.cpp
  stub_recorder_observer.cpp

  gstreamer/engine.cpp
  gstreamer/playbin.cpp

  mpris/media_player2.cpp

  player_skeleton.cpp
  player_implementation.cpp
  service_skeleton.cpp
  service_implementation.cpp
  track_list_skeleton.cpp
  track_list_implementation.cpp
)

target_link_libraries(
  media-hub-service

  call-monitor
  ${PC_GSTREAMER_1_0_LIBRARIES}
  ${PC_GSTREAMER_PBUTILS_1_0_LIBRARIES}
  ${GIO_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${PC_PULSE_AUDIO_LIBRARIES}
  Qt5::Core
  Qt5::DBus
)

target_include_directories(media-hub-service PRIVATE
  ${PROJECT_SOURCE_DIR}/src/
  ${HYBRIS_MEDIA_CFLAGS}
  ${PC_GSTREAMER_1_0_INCLUDE_DIRS}
)

add_executable(
  media-hub-server

  backend.cpp
  server/server.cpp
)

target_link_libraries(
  media-hub-server

  media-hub-service
  call-monitor

  ${PC_GSTREAMER_1_0_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
)

target_include_directories(media-hub-server PRIVATE
  ${PROJECT_SOURCE_DIR}/src/
)

install(
  TARGETS media-hub-server
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
