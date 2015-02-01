cmake_minimum_required(VERSION 2.8)
project(SpaceManagement)

include(FindPkgConfig)

set(SM_MONITOR ON CACHE BOOL
    "Enable space management monitor, it will leave N percents of free disk space.")

# TODO: use cmake-external/Configure, for more complex
if (SM_MONITOR)
    add_definitions(-DSM_MONITOR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
endif()

add_library(space-management SHARED space-management.c)
target_link_libraries(space-management dl)

pkg_search_module(FUSE REQUIRED fuse)
find_path(SYS_QUEUE NAMES "sys/queue.h")
if ((NOT "${FUSE_CFLAGS}" STREQUAL "") AND
    (NOT "${FUSE_LIBRARIES}" STREQUAL "") AND
    (NOT "${SYS_QUEUE}" STREQUAL ""))
    string(REPLACE ";" " " FUSE_CFLAGS "${FUSE_CFLAGS}")
    add_executable(fusefs fusefs.c)
    set_property(TARGET fusefs PROPERTY COMPILE_FLAGS ${FUSE_CFLAGS})
    target_link_libraries(fusefs ${FUSE_LIBRARIES})
endif()

set(SM_TEST ${CMAKE_SOURCE_DIR}/test.sh)
set(SM_LIBRARY_PATH ${CMAKE_BINARY_DIR}/libspace-management.so)
add_custom_target(runtests COMMAND ${SM_TEST} ${SM_LIBRARY_PATH})
