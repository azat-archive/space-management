cmake_minimum_required(VERSION 2.8)
project(SpaceManagement)

add_library(space-management SHARED space-management.c)
target_link_libraries(space-management dl)

set(SM_TEST ${CMAKE_SOURCE_DIR}/test.sh)
set(SM_LIBRARY_PATH ${CMAKE_BINARY_DIR}/libspace-management.so)
add_custom_target(
    runtests COMMAND
    ${SM_TEST} ${SM_LIBRARY_PATH} &&
    (env SPACE_MANAGEMENT_RETRIES=0 ${SM_TEST} ${SM_LIBRARY_PATH} && exit 1 || exit 0)
)
