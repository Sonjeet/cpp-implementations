cmake_minimum_required(VERSION 3.26)
project(Vector)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(vector INTERFACE)
target_sources(vector PRIVATE vector.h)
target_include_directories(vector INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(main)
target_sources(main PRIVATE main.cpp)
target_link_libraries(main PRIVATE vector)

enable_testing()
add_subdirectory(tests)

