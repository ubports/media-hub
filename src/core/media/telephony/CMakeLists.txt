SET (CMAKE_INCLUDE_CURRENT_DIR ON)
SET (CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
pkg_check_modules(TP_QT5 REQUIRED TelepathyQt5)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -fPIC -pthread")

include_directories(
    ${TP_QT5_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/src/
)

add_library(call-monitor STATIC
    call_monitor.cpp
    qtbridge.cpp
)

target_link_libraries(call-monitor
    ${TP_QT5_LIBRARIES}
)

qt5_use_modules(call-monitor Core DBus)
