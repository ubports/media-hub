include_directories(
    .
    ${CMAKE_SOURCE_DIR}/src
    ${PC_GSTREAMER_1_0_INCLUDE_DIRS}
)

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
    ${PC_PULSE_AUDIO_LIBRARIES}

    gmock
    gmock_main
    gtest
)

add_test(test-gstreamer-engine ${CMAKE_CURRENT_BINARY_DIR}/test-gstreamer-engine)
