find_package(Threads)

include_directories(${CMAKE_SOURCE_DIR}/include)

option(
    MUSIC_HUB_ENABLE_DBUS_TEST_RUNNER 
    "Rely on dbus test runner to start a private session for testing purposes" 
    ON
)

if (MUSIC_HUB_ENABLE_DBUS_TEST_RUNNER)
    find_program(DBUS_TEST_RUNNER_EXECUTABLE dbus-test-runner)
    message(STATUS "Executing test suite under dbus-test-runner")
endif (MUSIC_HUB_ENABLE_DBUS_TEST_RUNNER)

# Build with system gmock and embedded gtest
set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")

add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")
include_directories (
  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
)

add_subdirectory(acceptance-tests)