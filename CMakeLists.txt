cmake_minimum_required(VERSION 3.24)

project(kaleidoscope CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)

add_subdirectory(kaleidoscope)

add_executable(main main.cc)
target_link_libraries(main PRIVATE kaleidoscope)
