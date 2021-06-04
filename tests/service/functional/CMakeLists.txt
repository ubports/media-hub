project(ServiceFunctionalTests)

find_program(PYTEST_COMMAND NAMES py.test-3 py.test)
add_test(test_service ${PYTEST_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/ -vvv)

set(environment
    "SERVICE_BINARY=${CMAKE_BINARY_DIR}/src/core/media/media-hub-server"
)
set_tests_properties(test_service PROPERTIES ENVIRONMENT "${environment}")
