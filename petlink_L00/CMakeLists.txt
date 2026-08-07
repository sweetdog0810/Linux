cmake_minimum_required(VERSION 3.16)
project(petlink_l00 VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(petlink_l00
    src/main.cpp
    src/command_parser.cpp
    src/device_state.cpp
    src/logger.cpp
    src/system_info.cpp
)

target_include_directories(petlink_l00 PRIVATE include)
target_compile_options(petlink_l00 PRIVATE -Wall -Wextra -Wpedantic)
