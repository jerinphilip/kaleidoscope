#pragma once
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

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

struct SourceLocation {
  int line = 0;
  int column = 0;
};

class Lexer {
 public:
  explicit Lexer(std::string source);
  Atom read();
  const std::string &atom() const { return atom_; }
  char next() const { return next_; }
  char current() const { return current_; }
  Atom type() const { return type_; }
  SourceLocation locate() const { return source_location_; }

 private:
  Atom produce(Atom token);
  char step();
  void skip_spaces();
  char advance();

  std::string atom_;
  char current_;
  char next_ = ' ';
  Atom type_;

  char lookback_ = ' ';

  SourceLocation source_location_;
  std::string source_;
  std::fstream source_file_;
};

static std::map<char, int> op_precedence = {{'=', 2},  {'<', 10}, {'+', 20},
                                            {'-', 20}, {'*', 40}, {'/', 40}};
