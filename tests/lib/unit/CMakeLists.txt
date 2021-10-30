set(TEST test_client)
set(SOURCES
    test_client.cpp
)

add_executable(test_error
    ${MediaHubQt_SOURCE_DIR}/error.cpp
    ${MediaHubQt_SOURCE_DIR}/error.h
    ${MediaHubQt_SOURCE_DIR}/error_p.h
    test_error.cpp
)
target_include_directories(test_error PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${MediaHubQt_SOURCE_DIR}/..
)
target_link_libraries(test_error PRIVATE Qt5::Core Qt5::DBus Qt5::Test)
add_test(test_error test_error)
