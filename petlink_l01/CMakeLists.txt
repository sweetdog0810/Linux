cmake_minimum_required(VERSION 3.16)
project(petlink_linux_learning VERSION 0.2.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(COMMON_SOURCES
    src/command_parser.cpp
    src/device_state.cpp
    src/logger.cpp
    src/system_info.cpp
)

add_executable(petlink_l00
    src/main.cpp
    ${COMMON_SOURCES}
)

target_include_directories(petlink_l00 PRIVATE include)
target_compile_options(petlink_l00 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(petlink_l01
    src/l01_main.cpp
    src/serial_finder.cpp
    src/serial_port.cpp
    ${COMMON_SOURCES}
)

target_include_directories(petlink_l01 PRIVATE include)
target_compile_options(petlink_l01 PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(petlink_l01 PRIVATE Threads::Threads)
