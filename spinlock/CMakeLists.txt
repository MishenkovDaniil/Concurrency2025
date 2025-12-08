cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++-12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(spinlock_project VERSION 0.1 LANGUAGES C)

option(BUILD_TESTS "Build unit tests" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(spinlock STATIC
    src/spinlock_tas.c
    src/spinlock_ttas.c
    src/ticket_lock.c
    src/main.c
)

target_include_directories(spinlock
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(spinlock_demo
    src/main.c
)

target_link_libraries(spinlock_demo PRIVATE spinlock)

# Компиляция thread_num_test
add_library(thread_spinlock_test SHARED
    tests/thread_num_test.c
    src/spinlock_tas.c
    src/spinlock_ttas.c
    src/ticket_lock.c
)

target_include_directories(thread_spinlock_test
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(thread_spinlock_test PRIVATE pthread)
