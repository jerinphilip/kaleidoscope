
project(bergamot_translator CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(kaleidoscope 
  kaleidoscope/lexer.cc
  kaleidoscope/parser.cc)

add_executable(main kaleidoscope/main.cc)
target_link_libraries(main kaleidoscope)
