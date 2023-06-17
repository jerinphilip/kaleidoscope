cmake_minimum_required(VERSION 3.24)

project(kaleidoscope CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM 15.0 REQUIRED CONFIG)

add_subdirectory(kaleidoscope)
add_subdirectory(bin)

