pkg_check_modules(PC_GSTREAMER_1_0 REQUIRED gstreamer-1.0)

include_directories(${PC_GSTREAMER_1_0_INCLUDE_DIRS} ${HYBRIS_MEDIA_CFLAGS})

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
)

add_library(
  media-hub-client SHARED

  player.cpp
  service.cpp
  track.cpp
  track_list.cpp

  player_stub.cpp
  service_stub.cpp
  track_list_stub.cpp
)

set_target_properties(
  media-hub-client

  PROPERTIES
  VERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}.${UBUNTU_MEDIA_HUB_VERSION_MINOR}.${UBUNTU_MEDIA_HUB_VERSION_PATCH}
  SOVERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}
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

    engine.cpp
    gstreamer/engine.cpp

    player_skeleton.cpp
    player_implementation.cpp
    service_skeleton.cpp
    service_implementation.cpp
    track_list_skeleton.cpp
    track_list_implementation.cpp
)

target_link_libraries(
  media-hub-service

  media-hub-common

  ${DBUS_LIBRARIES}
  ${DBUS_CPP_LDFLAGS}
  ${GLog_LIBRARY}
  ${PC_GSTREAMER_1_0_LIBRARIES}
  ${GIO_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
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
