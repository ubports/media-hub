pkg_check_modules(PC_GSTREAMER_1_0 REQUIRED gstreamer-1.0)
pkg_check_modules(PC_GSTREAMER_PBUTILS_1_0 REQUIRED gstreamer-pbutils-1.0)
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

  ${MEDIA_HUB_HEADERS}
  ${MPRIS_HEADERS}

  logger/logger.cpp

  client_death_observer.cpp
  hashed_keyed_player_store.cpp
  hybris_client_death_observer.cpp
  stub_client_death_observer.cpp
  cover_art_resolver.cpp
  engine.cpp
  metadata.cpp

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
  gstreamer/meta_data_support.cpp
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
  ${Boost_LDFLAGS}
  ${Boost_LIBRARIES}
  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
  ${GLog_LIBRARY}
  ${PC_GSTREAMER_1_0_LIBRARIES}
  ${PC_GSTREAMER_PBUTILS_1_0_LIBRARIES}
  ${PROCESS_CPP_LDFLAGS}
  ${GIO_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${PC_PULSE_AUDIO_LIBRARIES}
)

include_directories(${PROJECT_SOURCE_DIR}/src/ ${HYBRIS_MEDIA_CFLAGS} ${PC_GSTREAMER_1_0_INCLUDE_DIRS})

add_executable(
  media-hub-server

  backend.cpp
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
