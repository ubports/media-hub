pkg_check_modules(PROCESS_CPP process-cpp REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/src

    ${PROCESS_CPP_INCLUDE_DIRS})

add_executable(
    service_acceptance_test
    service.cpp
  )

target_link_libraries(
    service_acceptance_test

    media-hub-common
    media-hub-client
    media-hub-service
    media-hub-test-framework

    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${PROCESS_CPP_LDFLAGS}

    gmock
    gmock_main
)

if (MEDIA_HUB_ENABLE_DBUS_TEST_RUNNER)
  add_test(service_acceptance_test ${DBUS_TEST_RUNNER_EXECUTABLE} --task=${CMAKE_CURRENT_BINARY_DIR}/service_acceptance_test)
else (MEDIA_HUB_ENABLE_DBUS_TEST_RUNNER)
  add_test(service_acceptance_test ${CMAKE_CURRENT_BINARY_DIR}/service_acceptance_test)
endif (MEDIA_HUB_ENABLE_DBUS_TEST_RUNNER)
