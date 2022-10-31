#pragma once
#include "ast.h"
#include <cstdio>
#include <map>
#include <memory>
#include <string>

enum class Atom {
  eof,
  def,
  extern_,
  identifier,
  number,
  semicolon,
  comment,
  open,
  close,
  op,
  unknown,
  comma
};

class Lexer {
public:
  Lexer() : next_(' '){};
  Atom read();
  const std::string &atom() const { return atom_; }
  char next() const { return next_; }
  char current() const { return current_; }
  Atom type() const { return type_; }

private:
  Atom produce(Atom token) {
    fprintf(stderr, "Producing %s\n", atom_.c_str());
    type_ = token;
    while (isspace(next_)) {
      next_ = getchar();
    }

    fprintf(stderr, "Pointer seeking to %c and waiting...\n", next_);
    return token;
  }

  char advance() {
    current_ = next_;
    return getchar();
  }

  std::string atom_;
  char current_, next_;
  Atom type_;
};

static std::map<char, int> OP_PRECEDENCE = {
    {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}, {'/', 40}};
