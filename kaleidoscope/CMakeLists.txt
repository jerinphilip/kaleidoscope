
add_library(kaleidoscope STATIC
  lexer.cc
  parser.cc
  ast.cc)

target_include_directories(kaleidoscope PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(kaleidoscope PUBLIC ${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(kaleidoscope PUBLIC ${llvm_libs})
