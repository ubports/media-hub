include_directories(
    ${CMAKE_SOURCE_DIR}/src)

add_executable(
    test_track_list
    test_track_list.cpp
  )

target_link_libraries(
    test_track_list

    media-hub-client

    ${CMAKE_THREAD_LIBS_INIT}
    ${DBUS_LIBRARIES}
    ${PROCESS_CPP_LDFLAGS}
)
