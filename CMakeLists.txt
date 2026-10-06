cmake_minimum_required(VERSION 3.10)
project(jaanbd)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(WANT_ZLIB FALSE)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(ENET_TEST OFF)
set(ZLIB_BUILD_TESTING OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(BUILD_EXAMPLES OFF)

CPMAddPackage("gh:raysan5/raylib#6.0")
CPMAddPackage("gh:zpl-c/enet#v2.6.5")
CPMAddPackage(
    NAME "zlib"
    GIT_REPOSITORY "https://github.com/madler/zlib"
    GIT_TAG "v1.3.2"
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
)
CPMAddPackage(
    NAME "libxml2"
    GIT_REPOSITORY "https://github.com/gnome/libxml2"
    GIT_TAG "v2.15.4"
    OPTIONS
        "LIBXML2_WITH_ICONV OFF"
        "BUILD_SHARED_LIBS OFF"
)
CPMAddPackage(
    NAME "tmx"
    GIT_REPOSITORY "https://github.com/baylej/tmx"
    GIT_TAG "tmx_1.10.1"
    OPTIONS
        "ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR}"
        "ZLIB_LIBRARY zlib"
)

file(GLOB Client
    ${CMAKE_SOURCE_DIR}/src/client/*.cpp
)

file(GLOB Server
    ${CMAKE_SOURCE_DIR}/src/server/*.cpp
)

file(GLOB Shared
    ${CMAKE_SOURCE_DIR}/src/shared/*.cpp
)

add_executable(${PROJECT_NAME} ${Client} ${Shared})
add_executable(${PROJECT_NAME}-server ${Server} ${Shared})

target_link_libraries(${PROJECT_NAME} PUBLIC raylib enet_static tmx)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/src/shared
    ${enet_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}-server PUBLIC enet_static)
target_include_directories(${PROJECT_NAME}-server PUBLIC
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/shared
    ${raylib_SOURCE_DIR}/src
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)