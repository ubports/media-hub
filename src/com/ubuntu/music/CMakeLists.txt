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
  
  player_skeleton.cpp
  service_skeleton.cpp
  service_implementation.cpp
)

target_link_libraries(
  music-hub-service

  music-hub-common

  ${DBUS_LIBRARIES}
  ${GLog_LIBRARY}
)
