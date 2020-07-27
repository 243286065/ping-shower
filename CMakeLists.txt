cmake_minimum_required(VERSION 3.8)

project(ping_shower)

message("-- system arch:  ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "current platform: ${CMAKE_SYSTEM_NAME} ")

add_definitions(-DOS_ARCH=${CMAKE_SYSTEM_NAME})

IF (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-DDEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")
    message(STATUS "Compile Debug")
ELSE()
    add_definitions(-DRELEASE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -pthread")
    message(STATUS "Compile Release")
ENDIF()

IF( ${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0601 /W4 /wd4068 /wd4702 /wd4996" )
ENDIF()
set(CMAKE_CXX_STANDARD 11)

IF (WIN32)
    add_definitions(-DOS_WIN=1)
ELSEIF (APPLE)
    add_definitions(-DOS_LINUX=1)
ELSEIF (UNIX)
    add_definitions(-DOS_LINUX=1)
ENDIF ()

set(PROJECT_OUTPUT_DIR ${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR})
message("-- output path:  ${PROJECT_OUTPUT_DIR} ")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake" )

file(MAKE_DIRECTORY ${PROJECT_OUTPUT_DIR}/lib)
file(MAKE_DIRECTORY ${PROJECT_OUTPUT_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/lib)

find_package(asio REQUIRED)
include_directories(${ASIO_INCLUDE})
include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/third_party/base/src)
include_directories(${PROJECT_SOURCE_DIR}/src/third_party/matplotlib-cpp)
include_directories(${PROJECT_SOURCE_DIR}/src/third_party/matplotlib-cpp)
include_directories("/usr/include/python2.7")

add_subdirectory(${PROJECT_SOURCE_DIR}/src/third_party/base)

set(
    ProgrameSrc
    src/main.cc
    src/net/ping/ping_client.h
    src/net/ping/ping_client.cc
    src/display/matplot_displayer.h
    src/display/matplot_displayer.cc
)

add_executable(${PROJECT_NAME} ${ProgrameSrc})
target_link_libraries(${PROJECT_NAME} base python2.7)