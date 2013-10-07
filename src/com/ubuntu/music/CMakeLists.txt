pkg_check_modules(PC_GSTREAMER_1_0 REQUIRED gstreamer-1.0)

include_directories(${PC_GSTREAMER_1_0_INCLUDE_DIRS})

add_library(
  music-hub-common SHARED

  the_session_bus.cpp
)

target_link_libraries(
  music-hub-common

  ${Boost_LIBRARIES}
  ${DBUS_LIBRARIES}
)

add_library(
  music-hub-client SHARED
  
  connection.cpp
  player.cpp
  service.cpp
  track.cpp
  track_list.cpp

  player_stub.cpp
  service_stub.cpp
  track_list_stub.cpp
)

set_target_properties(
  music-hub-client

  PROPERTIES
  VERSION ${UBUNTU_MUSIC_HUB_VERSION_MAJOR}.${UBUNTU_MUSIC_HUB_VERSION_MINOR}.${UBUNTU_MUSIC_HUB_VERSION_PATCH}
  SOVERSION ${UBUNTU_MUSIC_HUB_VERSION_MAJOR}
)

target_link_libraries(
  music-hub-client

  music-hub-common

  ${DBUS_LIBRARIES}
  ${GLog_LIBRARY}
)

install(
  TARGETS music-hub-client
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_library(
    music-hub-service

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
  music-hub-service

  music-hub-common

  ${DBUS_LIBRARIES}
  ${GLog_LIBRARY}
  ${PC_GSTREAMER_1_0_LIBRARIES}
)
