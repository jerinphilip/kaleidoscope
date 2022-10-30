
#include "lexer.h"
#include "parser.h"
#include <cstdio>

int main() {
  Lexer lexer;
  Parser parser;

  fprintf(stderr, "> ");
  Atom symbol = lexer.read();
  while (symbol != Atom::eof) {

    switch (symbol) {

    case Atom::eof: {
      fprintf(stderr, "Parsed EOF\n");
      return 0;
    } break;

    case Atom::def: {
      // Handle definition
      fprintf(stderr, "Attempting to parse def ....\n");
      DefinitionPtr def = parser.definition(lexer);
      if (def) {
        fprintf(stderr, "Parsed def\n");
      } else {
        fprintf(stderr, "Failed to parse...\n");
        lexer.read();
      }
    } break;

    case Atom::extern_: {
      // Handle extern
      PrototypePtr expr = parser.extern_(lexer);
      if (expr) {
        fprintf(stderr, "Parsed extern\n");
      } else {
        lexer.read();
      }
    } break;

    case Atom::comment: {
      fprintf(stderr, "Parsed comment\n");
    } break;

    case Atom::unknown: {
      fprintf(stderr, "Parsed unknown\n");
    } break;

    case Atom::semicolon: {
      lexer.read();
    } break;

    default: {
      DefinitionPtr expr = parser.top(lexer);
      if (expr) {
        fprintf(stderr, "Parsed top-level expression\n");
      } else {
        lexer.read();
      }
    } break;
    }

    fprintf(stderr, "> ");
    symbol = lexer.read();
  }
  return 0;
}
