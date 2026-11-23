cmake_minimum_required(VERSION 3.26)
project(cpp_implementations)

set(ENABLE_SANITIZERS "Build with sanitisers e.g. ASan, UBSan etc." OFF)

if (ENABLE_SANITIZERS)
    add_compile_options(
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=undefined
    )
add_link_options(-fsanitize=address,undefined)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(cpp_impl)
target_sources(cpp_impl PRIVATE main.cpp)

enable_testing()
add_subdirectory(test)
