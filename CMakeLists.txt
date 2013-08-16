cmake_minimum_required(VERSION 2.8)

project(ubuntu-music-hub)

set(UBUNTU_MUSIC_HUB_VERSION_MAJOR 0)
set(UBUNTU_MUSIC_HUB_VERSION_MINOR 0)
set(UBUNTU_MUSIC_HUB_VERSION_PATCH 1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -pedantic -Wextra -fPIC -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -fno-strict-aliasing -pedantic -Wextra -fPIC -pthread")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/EnableCoverageReport.cmake)
include(cmake/FindGFlags.cmake)
include(cmake/FindGLog.cmake)
include(cmake/PrePush.cmake)
include(CTest)

include(GNUInstallDirs)

find_package(PkgConfig)
find_package(Boost 1.49 COMPONENTS system program_options)
find_package(GLog)
pkg_check_modules(DBUS dbus-1 REQUIRED)
pkg_check_modules(DBUS_CPP dbus-cpp REQUIRED)

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
  ${DBUS_CPP_INCLUDE_DIRS}/dbus
  ${GLog_INCLUDE_DIR}
  
  include/
)

add_subdirectory(include)
add_subdirectory(doc)
add_subdirectory(src)

# There's no nice way to format this. Thanks CMake.
add_test(LGPL-required
  /bin/sh -c "! grep -rl 'GNU General' ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include"
)