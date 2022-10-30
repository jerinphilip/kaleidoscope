#pragma once
#include "ast.h"
#include <cstdio>
#include <memory>
#include <string>

enum class Token {
  eof = -1,
  def = -2,
  extern_ = -3,
  identifier = -4,
  number = -5,
  init = -6,
  semicolon = -7,
  comment = -8,
  unknown = -9,
  parenthesisOpen = -10
};

class Lexer {
public:
  Token read();
  const std::string &atom() const { return atom_; }
  char last_char() const { return last_char_; }
  Token current() const { return last_token_; }

private:
  Token set(Token token) {
    last_token_ = token;
    return token;
  }

  std::string atom_;
  char last_char_;
  Token last_token_;
};
