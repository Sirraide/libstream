cmake_minimum_required(VERSION 3.0)
project(libstream_tests VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)

include(CTest)
add_executable(_dummy test.cc)
target_include_directories(_dummy PRIVATE "${PROJECT_SOURCE_DIR}/../include")
add_test(
    NAME libstream_tests
    COMMAND "${CMAKE_CXX_COMPILER}"
        -std=c++23
        "-I${PROJECT_SOURCE_DIR}/../include"
        -fsyntax-only
        "${PROJECT_SOURCE_DIR}/test.cc"
)