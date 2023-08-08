cmake_minimum_required(VERSION 3.18)
project(socket-library LANGUAGES C CXX)

# Default to C++20 if not explicitly specified otherwise
if(NOT DEFINED CMAKE_CXX_STANDARD)
    message(STATUS "No C++ version specified, defaulting to C++20")
    set(CMAKE_CXX_STANDARD 20)
endif()

# Include CMake modules in ./cmake/
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;")
include(cmx)
cmx_include_scripts()

# Library
cmx_add_library(socket-library "${CMAKE_SOURCE_DIR}/src")
target_include_directories(socket-library PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(socket-library-static PUBLIC "${CMAKE_SOURCE_DIR}/include")
cmx_include_fmt(socket-library PUBLIC)
cmx_include_kstd_core(socket-library PUBLIC)
cmx_include_fmt(socket-library-static PUBLIC)
cmx_include_kstd_core(socket-library-static PUBLIC)

# Tests
cmx_add_tests(socket-library-tests "${CMAKE_SOURCE_DIR}/test")
target_include_directories(socket-library-tests PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(socket-library-tests PRIVATE socket-library-static)
cmx_include_fmt(socket-library-tests PRIVATE)
cmx_include_kstd_core(socket-library-tests PRIVATE)