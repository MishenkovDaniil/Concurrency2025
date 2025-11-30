cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++-12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(thread_pool_project VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTS "Build unit tests" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(thread_pool STATIC
    src/thread_pool.cpp
    src/main.cpp
)

target_include_directories(thread_pool
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(thread_pool_demo
    src/main.cpp
)

target_link_libraries(thread_pool_demo PRIVATE thread_pool)
