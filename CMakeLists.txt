cmake_minimum_required(VERSION 2.8)

project(ubuntu-media-hub)

option(ENABLE_DOC "Build documentation" ON)
option(ENABLE_TESTS "Build tests" ON)

# We haven't received version information via the packaging setup.
# For that, we try to determine sensible values on our own, ensuring
# plain old invocations to cmake still work as expected.
if (NOT DEFINED UBUNTU_MEDIA_HUB_VERSION_MAJOR)
  find_program(LSB_RELEASE lsb_release)
  execute_process(
    COMMAND ${LSB_RELEASE} -c -s
    OUTPUT_VARIABLE DISTRO_CODENAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # We explicitly ignore errors and only check if we are building for vivid.
  # For all other cases:
  #   - releases other than vivid
  #   - other distros
  #   - errors
  # we define the version to be 5.0.0
  if (${DISTRO_CODENAME} STREQUAL "vivid")
    set(UBUNTU_MEDIA_HUB_VERSION_MAJOR 4)
    set(UBUNTU_MEDIA_HUB_VERSION_MINOR 6)
    set(UBUNTU_MEDIA_HUB_VERSION_PATCH 0)
  else ()
    set(UBUNTU_MEDIA_HUB_VERSION_MAJOR 5)
    set(UBUNTU_MEDIA_HUB_VERSION_MINOR 2)
    set(UBUNTU_MEDIA_HUB_VERSION_PATCH 0)
  endif()
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-strict-aliasing -Wextra -fPIC -pthread")
set(CMAKE_AUTOMOC ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/EnableCoverageReport.cmake)
include(cmake/PrePush.cmake)
include(CTest)

include(GNUInstallDirs)

find_package(PkgConfig)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Test REQUIRED)
pkg_check_modules(GIO gio-2.0 REQUIRED)
pkg_check_modules(HYBRIS_MEDIA libmedia REQUIRED)
pkg_check_modules(EGL egl REQUIRED)
pkg_check_modules(GL gl REQUIRED)

#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

option (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS "Disable errors when local typedefs are unused" ON)
if (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-local-typedefs")
endif (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS)

add_definitions(
  -DQT_NO_KEYWORDS
)

if (ENABLE_DOC)
  add_subdirectory(doc)
endif (ENABLE_DOC)
add_subdirectory(src)
if (ENABLE_TESTS)
  add_subdirectory(tests)
  enable_coverage_report(
    TARGETS media-hub-server media-hub-service MediaHub
    FILTER tests/* build/*
  )
endif (ENABLE_TESTS)

# There's no nice way to format this. Thanks CMake.
add_test(LGPL-required
  /bin/sh -c "! grep -rl 'GNU General' ${PROJECT_SOURCE_DIR}/src"
)
