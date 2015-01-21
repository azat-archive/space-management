cmake_minimum_required(VERSION 2.8)
project(SpaceManagement)

add_library(space-management SHARED space-management.c)
target_link_libraries(space-management dl)

add_custom_target(
    runtests COMMAND
    ${CMAKE_SOURCE_DIR}/test.sh &&
    env SPACE_MANAGEMENT_RETRIES=0 ${CMAKE_SOURCE_DIR}/test.sh && exit 1 || exit 0
)
