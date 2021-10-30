project(MediaHubQt)

set(CLIENT_LIB MediaHub)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")

set(PUBLIC_HEADERS
    Error error.h
    Player player.h
    Track track.h
    TrackList track_list.h
    VideoSink video_sink.h
    global.h
)

add_library(${CLIENT_LIB} SHARED
    ${PUBLIC_HEADERS}

    dbus_constants.h
    egl_video_sink.cpp
    egl_video_sink.h
    error.cpp
    error_p.h
    hybris_video_sink.cpp
    hybris_video_sink.h
    logging.cpp
    logging.h
    player.cpp
    socket_types.h
    track_list.cpp
    track_list_p.h
    video_sink.cpp
    video_sink_p.h
)

pkg_check_modules(PC_GSTREAMER_1_0 REQUIRED gstreamer-1.0)

target_include_directories(${CLIENT_LIB} PRIVATE
    ${PC_GSTREAMER_1_0_INCLUDE_DIRS}
)

set_target_properties(${CLIENT_LIB} PROPERTIES
    VERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}.${UBUNTU_MEDIA_HUB_VERSION_MINOR}.${UBUNTU_MEDIA_HUB_VERSION_PATCH}
    SOVERSION ${UBUNTU_MEDIA_HUB_VERSION_MAJOR}
)

target_link_libraries(${CLIENT_LIB}
    ${HYBRIS_MEDIA_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GL_LIBRARIES}
    ${PC_GSTREAMER_1_0_LIBRARIES}
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
)

install(
    TARGETS ${CLIENT_LIB}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    FILES ${PUBLIC_HEADERS}
    DESTINATION include/${CLIENT_LIB}
)

configure_file(${CLIENT_LIB}.pc.in ${CLIENT_LIB}.pc @ONLY)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${CLIENT_LIB}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
