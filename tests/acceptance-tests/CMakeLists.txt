include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(
  service_acceptance_test
  service.cpp)

target_link_libraries(
    service_acceptance_test

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

if (MUSIC_HUB_ENABLE_DBUS_TEST_RUNNER)
  add_test(service_acceptance_test ${DBUS_TEST_RUNNER_EXECUTABLE} --task=${CMAKE_CURRENT_BINARY_DIR}/service_acceptance_test)
else (MUSIC_HUB_ENABLE_DBUS_TEST_RUNNER)
  add_test(service_acceptance_test ${CMAKE_CURRENT_BINARY_DIR}/service_acceptance_test)
endif (MUSIC_HUB_ENABLE_DBUS_TEST_RUNNER)
