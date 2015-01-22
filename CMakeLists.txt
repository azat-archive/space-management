cmake_minimum_required(VERSION 2.8)
project(SpaceManagement)

set(SM_MONITOR ON CACHE BOOL
    "Enable space management monitor, it will leave N percents of free disk space.")

# TODO: use cmake-external/Configure, for more complex
if (SM_MONITOR)
    add_definitions(-DSM_MONITOR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

add_library(space-management SHARED space-management.c)
target_link_libraries(space-management dl)

set(SM_TEST ${CMAKE_SOURCE_DIR}/test.sh)
set(SM_LIBRARY_PATH ${CMAKE_BINARY_DIR}/libspace-management.so)
add_custom_target(runtests COMMAND ${SM_TEST} ${SM_LIBRARY_PATH})
