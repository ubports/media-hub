include_directories(
    .
    ${CMAKE_SOURCE_DIR}/src
    ${PC_GSTREAMER_1_0_INCLUDE_DIRS}
)

add_library(mongoose mongoose.c)
set_target_properties(
    mongoose
    PROPERTIES COMPILE_FLAGS "-std=c99")

add_executable(
    test-gstreamer-engine

    libmedia-mock.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/cover_art_resolver.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/engine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/gstreamer/engine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/player_skeleton.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/player_implementation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/service_skeleton.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/service_implementation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/track_list_skeleton.cpp
    ${CMAKE_SOURCE_DIR}/src/core/media/track_list_implementation.cpp
    test-gstreamer-engine.cpp
)

target_link_libraries(
    test-gstreamer-engine

    media-hub-common
    media-hub-client
    call-monitor
    media-hub-test-framework

    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${DBUS_CPP_LDFLAGS}
    ${GLog_LIBRARY}
    ${PC_GSTREAMER_1_0_LIBRARIES}
    ${PROCESS_CPP_LDFLAGS}
    ${GIO_LIBRARIES}
    ${PROCESS_CPP_LIBRARIES}

    gmock
    gmock_main
    gtest

    mongoose
)

add_test(test-gstreamer-engine ${CMAKE_CURRENT_BINARY_DIR}/test-gstreamer-engine)
