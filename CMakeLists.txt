cmake_minimum_required(VERSION 3.0)
project(libstream_tests VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(libstream INTERFACE)
target_include_directories(libstream INTERFACE include)