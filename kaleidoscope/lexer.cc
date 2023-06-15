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
      return produce(Atom::kKeywordDef);
    }

    if (atom_ == "extern") {
      return produce(Atom::kKeywordExtern);
    }

    if (atom_ == "if") {
      return produce(Atom::kKeywordIf);
    }

    if (atom_ == "then") {
      return produce(Atom::kKeywordThen);
    }

    if (atom_ == "else") {
      return produce(Atom::kKeywordElse);
    }

    if (atom_ == "for") {
      return produce(Atom::kKeywordFor);
    }

    if (atom_ == "in") {
      return produce(Atom::kKeywordIn);
    }

    if (atom_ == "var") {
      return produce(Atom::kKeywordVar);
    }

    return produce(Atom::kIdentifier);
  }

  // Number parsing logic.
  if (isdigit(next_) || next_ == '.') {
    atom_ = "";
    while (isdigit(next_) || next_ == '.') {
      atom_ += next_;
      next_ = advance();
    }

    return produce(Atom::kNumber);
  }

  // Comments
  if (next_ == '#') {
    atom_ = "";
    while (next_ != EOF && next_ != '\n' && next_ != '\r') {
      atom_ += next_;
      next_ = advance();
    }

    return produce(Atom::kComment);
  }

  if (next_ == '(') {
    atom_ = "(";
    next_ = advance();
    return produce(Atom::kOpen);
  }

  if (next_ == ')') {
    atom_ = ")";
    next_ = advance();
    return produce(Atom::kClose);
  }

  if (next_ == EOF) {
    atom_ = std::string(1, EOF);
    return produce(Atom::kEof);
  }

  if (next_ == ';') {
    atom_ = std::string(1, next_);
    next_ = advance();
    return produce(Atom::kSemicolon);
  }

  if (detail::isOp(next_)) {
    atom_ = std::string(1, next_);
    next_ = advance();
    return produce(Atom::kOp);
  }

  if (next_ == ',') {
    atom_ = std::string(1, next_);
    next_ = advance();
    return produce(Atom::kComma);
  }

  atom_ = std::string(1, next_);
  next_ = advance();
  return produce(Atom::kUnknown);
}
