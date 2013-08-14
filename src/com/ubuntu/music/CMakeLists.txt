add_library(
  music_hub SHARED
  
  connection.cpp
  player.cpp
  service.cpp
  track.cpp
  track_list.cpp
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