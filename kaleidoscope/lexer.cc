#include "lexer.h"

Token Parser::read() {
  char last_char = ' ';

  // Skip leading whitespaces.
  while (isspace(last_char)) {
    last_char = getchar();
  }

  // Identifier parsing logic.
  if (isalpha(last_char)) {
    atom_ = last_char;
    last_char = getchar();
    while (isalnum(last_char)) {
      atom_ += last_char;
      last_char = getchar();
    }

    if (atom_ == "def") {
      return Token::def;
    }

    if (atom_ == "extern") {
      return Token::extern_;
    }
    return Token::identifier;
  }

  // Number parsing logic.
  if (isdigit(last_char) || last_char == '.') {
    atom_ = "";
    while (isdigit(last_char) || last_char == '.') {
      atom_ += last_char;
      last_char = getchar();
    }

    return Token::number;
  }

  // Comments
  if (last_char == '#') {
    // Skip comments.
    while (last_char != EOF && last_char != '\n' && last_char != '\r') {
      last_char = getchar();
    }

    atom_ = "";
    return Token::comment;
  }

  if (last_char == EOF) {
    atom_ = EOF;
    return Token::eof;
  }

  atom_ = last_char;
  return Token::unknown;
}
