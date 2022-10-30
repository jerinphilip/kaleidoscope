
cmake_minimum_required(VERSION 3.24)

project(bergamot_translator CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)


add_library(kaleidoscope STATIC
  kaleidoscope/lexer.cc
  kaleidoscope/parser.cc
  kaleidoscope/ast.cc)

target_include_directories(kaleidoscope PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(kaleidoscope PUBLIC ${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(kaleidoscope PUBLIC ${llvm_libs})

add_executable(main kaleidoscope/main.cc)
target_link_libraries(main PRIVATE kaleidoscope)
