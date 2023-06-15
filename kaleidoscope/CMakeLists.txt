add_library(kaleidoscope STATIC lexer.cc parser.cc ast.cc libkl.cc)

target_include_directories(kaleidoscope PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(kaleidoscope PUBLIC ${LLVM_DEFINITIONS})


# llvm_map_components_to_libnames is outdated, so try avoiding it. Adding this
# code to your CMakeList file should fetch all the llvm libraries at configure
# time.

# If you intend to fetch libraries of specific components, just replace all
# with the component names.

# https://stackoverflow.com/a/62939671

execute_process(COMMAND llvm-config --libs all
                OUTPUT_VARIABLE llvm_libraries 
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)

target_link_libraries(kaleidoscope PUBLIC ${llvm_libraries})
target_include_directories(kaleidoscope PUBLIC ${CMAKE_SOURCE_DIR})
