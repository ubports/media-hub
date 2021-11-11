pkg_check_modules(PC_GSTREAMER_1_0 REQUIRED gstreamer-1.0)
pkg_check_modules(PC_GSTREAMER_PBUTILS_1_0 REQUIRED gstreamer-pbutils-1.0)
pkg_check_modules(PC_PULSE_AUDIO REQUIRED libpulse libpulse-mainloop-glib)
pkg_check_modules(APPARMOR REQUIRED libapparmor)
include_directories(${PC_GSTREAMER_1_0_INCLUDE_DIRS} ${HYBRIS_MEDIA_CFLAGS} ${PC_PULSE_AUDIO_INCLUDE_DIRS})

file(GLOB MPRIS_HEADERS mpris/*.h)

SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

add_subdirectory(telephony)

add_library(
  media-hub-service

  logging.cpp

  dbus_property_notifier.cpp

  client_death_observer.cpp
  dbus_client_death_observer.cpp
  hybris_client_death_observer.cpp
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
  ${APPARMOR_LIBRARIES}
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
  ${APPARMOR_INCLUDE_DIRS}
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
