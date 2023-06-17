add_library(kaleidoscope STATIC lexer.cc parser.cc ast.cc libkl.cc codegen_context.cc) 

target_include_directories(kaleidoscope PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(kaleidoscope PUBLIC ${LLVM_DEFINITIONS})


# Much as I hate doing this, I don't find another way.
set(LLVM_LIBRARIES "-lLLVM-15")

target_link_libraries(kaleidoscope PUBLIC ${LLVM_LIBRARIES})
target_include_directories(kaleidoscope PUBLIC ${CMAKE_SOURCE_DIR})
