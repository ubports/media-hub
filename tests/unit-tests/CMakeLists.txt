include_directories(
    .
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(
    test-gstreamer-engine
    test-gstreamer-engine.cpp
)

target_link_libraries(
    test-gstreamer-engine

    music-hub-common
    music-hub-client
    music-hub-service
    music-hub-test-framework

    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${DBUS_LIBRARIES}
    
    gmock
    gmock_main
    gtest
)

add_test(test-gstreamer-engine ${CMAKE_CURRENT_BINARY_DIR}/test-gstreamer-engine)
