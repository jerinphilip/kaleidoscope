#pragma once
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "ast.h"

enum class Atom {
  kEof,
  kIdentifier,
  kKeywordDef,
  kKeywordExtern,
  kKeywordIf,
  kKeywordThen,
  kKeywordElse,
  kKeywordFor,
  kKeywordIn,
  kKeywordVar,
  kNumber,
  kSemicolon,
  kComment,
  kOpen,
  kClose,
  kOp,
  kUnknown,
  kComma
};

std::string debug_atom(const Atom &atom);

class Lexer {
 public:
  explicit Lexer(std::string source);
  Atom read();
  const std::string &atom() const { return atom_; }
  char next() const { return next_; }
  char current() const { return current_; }
  Atom type() const { return type_; }

 private:
  Atom produce(Atom token) {
    type_ = token;
    return token;
  }

  inline void skip_spaces() {
    while (isspace(next_)) {
      next_ = source_file_.get();
    }
  }

  char advance() {
    current_ = next_;
    skip_spaces();
    return source_file_.get();
  }

  std::string atom_;
  char current_;
  char next_ = ' ';
  Atom type_;

  std::string source_;
  std::fstream source_file_;
};

static std::map<char, int> op_precedence = {{'=', 2},  {'<', 10}, {'+', 20},
                                            {'-', 20}, {'*', 40}, {'/', 40}};
