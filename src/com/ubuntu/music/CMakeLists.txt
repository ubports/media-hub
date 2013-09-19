add_library(
  music_hub SHARED
  
  connection.cpp
  player.cpp
  service.cpp
  track.cpp
  track_list.cpp

  player_stub.cpp
  service_stub.cpp
)

set_target_properties(
  music_hub

  PROPERTIES
  VERSION ${UBUNTU_MUSIC_HUB_VERSION_MAJOR}.${UBUNTU_MUSIC_HUB_VERSION_MINOR}.${UBUNTU_MUSIC_HUB_VERSION_PATCH}
  SOVERSION ${UBUNTU_MUSIC_HUB_VERSION_MAJOR}
)

target_link_libraries(
  music_hub

  ${DBUS_LIBRARIES}
  ${GLog_LIBRARY}
)

install(
  TARGETS music_hub
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_executable(
  music_hub_service
  
  player_skeleton.cpp
)

target_link_libraries(
  music_hub_service
  
  music_hub
)