set(TEST test_client)
set(SOURCES
    dbusmock.h
    test_client.cpp
)

# TODO: use IMPORTED_TARGET when switching to Focal
pkg_check_modules(QTDBUSMOCK REQUIRED libqtdbusmock-1)
pkg_check_modules(QTDBUSTEST REQUIRED libqtdbustest-1)

add_executable(${TEST} ${SOURCES})
target_include_directories(${TEST} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${MediaHubQt_SOURCE_DIR}/..
    ${QTDBUSTEST_INCLUDE_DIRS}
    ${QTDBUSMOCK_INCLUDE_DIRS}
)

target_link_libraries(${TEST} PRIVATE
    MediaHub
    ${QTDBUSTEST_LIBRARIES}
    ${QTDBUSMOCK_LIBRARIES}
    Qt5::Core
    Qt5::DBus
    Qt5::Test
)

target_compile_definitions(${TEST} PRIVATE
    MEDIAHUB_MOCK_TEMPLATE="${CMAKE_CURRENT_SOURCE_DIR}/media-hub.py"
)

add_test(${TEST} ${CMAKE_CURRENT_BINARY_DIR}/${TEST})
