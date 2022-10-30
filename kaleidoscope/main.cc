
#include "lexer.h"
#include <cstdio>

int main() {
  Parser parser;
  Token token = Token::init;
  while (token != Token::eof) {
    fprintf(stderr, "> ");
    token = parser.read();
    switch (token) {

    case Token::eof:
      fprintf(stderr, "Parsed EOF\n");
      break;

    case Token::def:
      // Handle definition
      fprintf(stderr, "Parsed def\n");
      break;

    case Token::extern_:
      // Handle extern
      fprintf(stderr, "Parsed extern\n");
      break;

    case Token::comment:
      fprintf(stderr, "Parsed comment\n");
      break;

    case Token::unknown:
      fprintf(stderr, "Parsed unknown\n");
      break;

    case Token::init:
      break;
    default:
      break;
    }
  }
  return 0;
}
