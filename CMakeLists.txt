cmake_minimum_required(VERSION 2.8)

project(ubuntu-media-hub)

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
    set(UBUNTU_MEDIA_HUB_VERSION_MINOR 5)
    set(UBUNTU_MEDIA_HUB_VERSION_PATCH 0)
  else ()
    set(UBUNTU_MEDIA_HUB_VERSION_MAJOR 5)
    set(UBUNTU_MEDIA_HUB_VERSION_MINOR 2)
    set(UBUNTU_MEDIA_HUB_VERSION_PATCH 0)
  endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fno-strict-aliasing -Wextra -fPIC -pthread")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/EnableCoverageReport.cmake)
include(cmake/FindGFlags.cmake)
include(cmake/FindGLog.cmake)
include(cmake/PrePush.cmake)
include(CTest)

include(GNUInstallDirs)

find_package(PkgConfig)
find_package(Boost COMPONENTS filesystem log system thread program_options REQUIRED)
find_package(GLog)
pkg_check_modules(DBUS dbus-1 REQUIRED)
pkg_check_modules(DBUS_CPP dbus-cpp REQUIRED)
pkg_check_modules(PROCESS_CPP process-cpp REQUIRED)
pkg_check_modules(PROPERTIES_CPP properties-cpp REQUIRED)
pkg_check_modules(PROCESS_CPP process-cpp REQUIRED)
pkg_check_modules(GIO gio-2.0 REQUIRED)
pkg_check_modules(HYBRIS_MEDIA libmedia REQUIRED)

include(CheckIncludeFiles)
check_include_files("hybris/media/media_codec_layer.h" MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
if (DEFINED MEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
  add_definitions(-DMEDIA_HUB_HAVE_HYBRIS_MEDIA_COMPAT_LAYER)
else()
  MESSAGE(STATUS "*libhybris not found*")
endif()

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

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${DBUS_CPP_INCLUDE_DIRS}
  ${GLog_INCLUDE_DIR}
  ${PROCESS_CPP_INCLUDE_DIRS}
  ${PROPERTIES_CPP_INCLUDE_DIRS}

  include/
)

add_subdirectory(include)
add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(tests)

# There's no nice way to format this. Thanks CMake.
add_test(LGPL-required
  /bin/sh -c "! grep -rl 'GNU General' ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include"
)
