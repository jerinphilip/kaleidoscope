#include "lexer.h"
#include <memory>

namespace detail {
bool isOp(char c) {
  auto query = OP_PRECEDENCE.find(c);
  return query != OP_PRECEDENCE.end();
}
} // namespace detail

Atom Lexer::read() {
  // fprintf(stderr, "[lexer] Moving past %s\n", atom().c_str());

  // Skip leading whitespaces.
  while (isspace(next_)) {
    next_ = advance();
  }

  // Identifier parsing logic.
  if (isalpha(next_)) {
    atom_ = std::string(1, next_);
    next_ = advance();
    while (isalnum(next_)) {
      atom_ += next_;
      next_ = advance();
    }

    if (atom_ == "def") {
      return set(Atom::def);
    }

    if (atom_ == "extern") {
      return set(Atom::extern_);
    }

    return set(Atom::identifier);
  }

  // Number parsing logic.
  if (isdigit(next_) || next_ == '.') {
    atom_ = "";
    while (isdigit(next_) || next_ == '.') {
      atom_ += next_;
      next_ = advance();
    }

    return set(Atom::number);
  }

  // Comments
  if (next_ == '#') {
    atom_ = "";
    while (next_ != EOF && next_ != '\n' && next_ != '\r') {
      atom_ += next_;
      next_ = advance();
    }

    return set(Atom::comment);
  }

  if (next_ == '(') {
    atom_ = "(";
    next_ = advance();
    return set(Atom::open);
  }

  if (next_ == ')') {
    atom_ = ")";
    next_ = advance();
    return set(Atom::close);
  }

  if (next_ == EOF) {
    atom_ = std::string(1, EOF);
    return set(Atom::eof);
  }

  if (next_ == ';') {
    atom_ = std::string(1, next_);
    next_ = advance();
    return set(Atom::semicolon);
  }

  if (detail::isOp(next_)) {
    atom_ = std::string(1, next_);
    next_ = advance();
    return set(Atom::op);
  }

  if (next_ == ',') {
    atom_ = std::string(1, next_);
    next_ = advance();
    return set(Atom::comma);
  }

  atom_ = std::string(1, next_);
  next_ = advance();
  return set(Atom::unknown);
}
