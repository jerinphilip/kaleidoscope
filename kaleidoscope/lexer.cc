#include "lexer.h"

#include <memory>

namespace detail {
bool isOp(char c) {
  auto query = OP_PRECEDENCE.find(c);
  return query != OP_PRECEDENCE.end();
}
}  // namespace detail

Atom Lexer::read() {
  // fprintf(stderr, "[lexer] Moving past %s\n", atom().c_str());

  // Skip leading whitespaces.
  skip_spaces();

  // Identifier parsing logic.
  if (isalpha(next_)) {
    atom_ = std::string(1, next_);
    next_ = advance();
    while (isalnum(next_)) {
      atom_ += next_;
      next_ = advance();
    }

    if (atom_ == "def") {
      return produce(Atom::keyword_def);
    }

    if (atom_ == "extern") {
      return produce(Atom::keyword_extern);
    }

    if (atom_ == "if") {
      return produce(Atom::keyword_if);
    }

    if (atom_ == "then") {
      return produce(Atom::keyword_then);
    }

    if (atom_ == "else") {
      return produce(Atom::keyword_else);
    }

    if (atom_ == "for") {
      return produce(Atom::keyword_for);
    }

    if (atom_ == "in") {
      return produce(Atom::keyword_in);
    }

    if (atom_ == "var") {
      return produce(Atom::keyword_var);
    }

    return produce(Atom::identifier);
  }

  // Number parsing logic.
  if (isdigit(next_) || next_ == '.') {
    atom_ = "";
    while (isdigit(next_) || next_ == '.') {
      atom_ += next_;
      next_ = advance();
    }

    return produce(Atom::number);
  }

  // Comments
  if (next_ == '#') {
    atom_ = "";
    while (next_ != EOF && next_ != '\n' && next_ != '\r') {
      atom_ += next_;
      next_ = advance();
    }

    return produce(Atom::comment);
  }

  if (next_ == '(') {
    atom_ = "(";
    next_ = advance();
    return produce(Atom::open);
  }

  if (next_ == ')') {
    atom_ = ")";
    next_ = advance();
    return produce(Atom::close);
  }

  if (next_ == EOF) {
    atom_ = std::string(1, EOF);
    return produce(Atom::eof);
  }

  if (next_ == ';') {
    atom_ = std::string(1, next_);
    next_ = advance();
    return produce(Atom::semicolon);
  }

  if (detail::isOp(next_)) {
    atom_ = std::string(1, next_);
    next_ = advance();
    return produce(Atom::op);
  }

  if (next_ == ',') {
    atom_ = std::string(1, next_);
    next_ = advance();
    return produce(Atom::comma);
  }

  atom_ = std::string(1, next_);
  next_ = advance();
  return produce(Atom::unknown);
}
