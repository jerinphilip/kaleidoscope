#include "lexer.h"

#include <memory>

namespace detail {
bool isOp(char c) {
  auto query = op_precedence.find(c);
  return query != op_precedence.end();
}
}  // namespace detail

std::string debug_atom(const Atom &atom) {
  switch (atom) {
      // clang-format off
    case Atom::eof            : return "eof";
    case Atom::identifier     : return "identifier";
    case Atom::keyword_def    : return "keyword_def";
    case Atom::keyword_extern : return "keyword_extern";
    case Atom::keyword_if     : return "keyword_if";
    case Atom::keyword_then   : return "keyword_then";
    case Atom::keyword_else   : return "keyword_else";
    case Atom::keyword_for    : return "keyword_for";
    case Atom::keyword_in     : return "keyword_in";
    case Atom::keyword_var    : return "keyword_var";
    case Atom::number         : return "number";
    case Atom::semicolon      : return "semicolon";
    case Atom::kComment       : return "kComment";
    case Atom::open           : return "open";
    case Atom::close          : return "close";
    case Atom::op             : return "op";
    case Atom::unknown        : return "unknown";
    case Atom::comma          : return "comma";
    default                   : return "unknown";
      // clang-format on
  }
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

    return produce(Atom::kComment);
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

Atom Lexer::produce(Atom token) {
  auto escape = [](char c) -> std::string {
    if (c == EOF) {
      return "eof";
    }

    if (c == '\r' || c == '\n') {
      return "\\n";
    }

    return std::string(1, c);
  };

  std::string datom = debug_atom(token);
  std::string dcurrent = escape(current_);
  std::string dnext = escape(next_);

  // fprintf(stderr, "Producing: <%s> as atom: <%s>, current: <%s>, next:
  // <%s>\n",
  //         atom_.c_str(), datom.c_str(), dcurrent.c_str(), dnext.c_str());
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
