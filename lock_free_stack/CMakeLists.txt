cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++-12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address -fno-omit-frame-pointer")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")
# set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")

project(lf_stack VERSION 0.1 LANGUAGES CXX)

add_library(lf_stack STATIC
    src/stack.cpp
    src/hp.cpp
)

target_include_directories(lf_stack
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(lf_stack_demo
    src/main.cpp
)

add_executable(lf_stack_benchmark
    src/benchmark.cpp
)

target_link_libraries(lf_stack_demo PRIVATE lf_stack pthread)
target_link_libraries(lf_stack_benchmark PRIVATE lf_stack pthread)
