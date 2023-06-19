#pragma once
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

// NOLINTBEGIN
enum class Atom {
  eof,
  identifier,
  keyword_def,
  keyword_extern,
  keyword_if,
  keyword_then,
  keyword_else,
  keyword_for,
  keyword_in,
  keyword_var,
  number,
  semicolon,
  kComment,
  open,
  close,
  op,
  unknown,
  comma
};
// NOLINTEND

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

static const std::map<char, int> op_precedence = {
    //
    {':', 1},              // Sequencing operator.
    {'=', 2},              // Assignment
    {'|', 5},              // Logical OR
    {'&', 6},              // Logical AND
    {'<', 10}, {'>', 10},  // less-than, greater than
    {'+', 20}, {'-', 20},  //
    {'*', 40}, {'/', 40}   //
};
