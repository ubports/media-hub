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
    test-gstreamer-engine.cpp
)

target_link_libraries(
    test-gstreamer-engine

    media-hub-common
    media-hub-client
    media-hub-service
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
    ${PC_PULSE_AUDIO_LIBRARIES}

    gmock
    gmock_main
    gtest

    mongoose
)

add_test(test-gstreamer-engine ${CMAKE_CURRENT_BINARY_DIR}/test-gstreamer-engine)

#-----------------------------------------

add_executable(
    test-player-store

    libmedia-mock.cpp
    test-player-store.cpp
)

target_link_libraries(
    test-player-store

    media-hub-common
    media-hub-client
    media-hub-service
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
    ${PC_PULSE_AUDIO_LIBRARIES}

    gmock
    gmock_main
    gtest
)

add_test(test-player-store ${CMAKE_CURRENT_BINARY_DIR}/test-player-store)
