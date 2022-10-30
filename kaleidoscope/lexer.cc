#include "lexer.h"
#include <memory>

Token Lexer::read() {
  char last_char_ = ' ';

  // Skip leading whitespaces.
  while (isspace(last_char_)) {
    last_char_ = getchar();
  }

  // Identifier parsing logic.
  if (isalpha(last_char_)) {
    atom_ = last_char_;
    last_char_ = getchar();
    while (isalnum(last_char_)) {
      atom_ += last_char_;
      last_char_ = getchar();
    }

    if (atom_ == "def") {
      return set(Token::def);
    }

    if (atom_ == "extern") {
      return set(Token::extern_);
    }

    return set(Token::identifier);
  }

  // Number parsing logic.
  if (isdigit(last_char_) || last_char_ == '.') {
    atom_ = "";
    while (isdigit(last_char_) || last_char_ == '.') {
      atom_ += last_char_;
      last_char_ = getchar();
    }

    return set(Token::number);
  }

  // Comments
  if (last_char_ == '#') {
    // Skip comments.
    while (last_char_ != EOF && last_char_ != '\n' && last_char_ != '\r') {
      last_char_ = getchar();
    }

    atom_ = "";
    return set(Token::comment);
  }

  if (last_char_ == EOF) {
    atom_ = EOF;
    return set(Token::eof);
  }

  atom_ = last_char_;
  return set(Token::unknown);
}
