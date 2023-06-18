#include "lexer.h"

#include <memory>

namespace detail {
bool isOp(char c) {
  auto query = op_precedence.find(c);
  return query != op_precedence.end();
}
}  // namespace detail

std::string debug_atom(const Atom &atom) {
#define CASE_ATOM(atom) \
  case Atom::atom:      \
    return #atom

  switch (atom) {
    CASE_ATOM(kEof);
    CASE_ATOM(kIdentifier);
    CASE_ATOM(kKeywordDef);
    CASE_ATOM(kKeywordExtern);
    CASE_ATOM(kKeywordIf);
    CASE_ATOM(kKeywordThen);
    CASE_ATOM(kKeywordElse);
    CASE_ATOM(kKeywordFor);
    CASE_ATOM(kKeywordIn);
    CASE_ATOM(kKeywordVar);
    CASE_ATOM(kNumber);
    CASE_ATOM(kSemicolon);
    CASE_ATOM(kComment);
    CASE_ATOM(kOpen);
    CASE_ATOM(kClose);
    CASE_ATOM(kOp);
    CASE_ATOM(kUnknown);
    CASE_ATOM(kComma);
    default:
      return "unknown";
  }

#undef CASE_ATOM
}

Lexer::Lexer(std::string source)
    : source_(std::move(source)), source_file_(source_) {}

Atom Lexer::read() {
  // fprintf(stderr, "[lexer] Moving past %s\n", atom().c_str());
  // fprintf(stderr, "[lexer] next_ is %d\n", next_);

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

Atom Lexer::produce(Atom token) {
  type_ = token;
  return token;
}

char Lexer::step() {
  if (lookback_ == '\r' || lookback_ == '\n') {
    ++source_location_.line;
    source_location_.column = 0;
  } else {
    ++source_location_.column;
  }
  lookback_ = source_file_.get();
  return lookback_;
}

void Lexer::skip_spaces() {
  while (isspace(next_)) {
    next_ = step();
  }
}

char Lexer::advance() {
  current_ = next_;
  skip_spaces();
  return step();
}
