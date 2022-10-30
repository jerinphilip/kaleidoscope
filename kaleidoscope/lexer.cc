#include "lexer.h"
#include <memory>

Token Lexer::read() {
  char last_char_ = ' ';

  // Skip leading whitespaces.
  while (isspace(last_char_)) {
    last_char_ = getchar();
  }

  // Identifier parsing logic.
  if (isalpha(last_char_)) {
    atom_ = last_char_;
    last_char_ = getchar();
    while (isalnum(last_char_)) {
      atom_ += last_char_;
      last_char_ = getchar();
    }

    if (atom_ == "def") {
      return set(Token::def);
    }

    if (atom_ == "extern") {
      return set(Token::extern_);
    }

    return set(Token::identifier);
  }

  // Number parsing logic.
  if (isdigit(last_char_) || last_char_ == '.') {
    atom_ = "";
    while (isdigit(last_char_) || last_char_ == '.') {
      atom_ += last_char_;
      last_char_ = getchar();
    }

    return set(Token::number);
  }

  // Comments
  if (last_char_ == '#') {
    // Skip comments.
    while (last_char_ != EOF && last_char_ != '\n' && last_char_ != '\r') {
      last_char_ = getchar();
    }

    atom_ = "";
    return set(Token::comment);
  }

  if (last_char_ == EOF) {
    atom_ = EOF;
    return set(Token::eof);
  }

  atom_ = last_char_;
  return set(Token::unknown);
}

ExprPtr LogError(const char *message) {
  fprintf(stderr, "LogError: %s\n", message);
  return nullptr;
}

std::unique_ptr<function::Prototype> LogErrorP(const char *message) {
  LogError(message);
  return nullptr;
}

// numberExpr = number
ExprPtr Parser::number(Lexer &lexer) {
  double value = strtod(lexer.atom().c_str(), 0);
  auto result = std::make_unique<Number>(value);
  lexer.read();
  return result;
}

// paranthesisExpr = '(' expression ')'
ExprPtr Parser::paranthesis(Lexer &lexer) {
  lexer.read();
  auto expr = expression(lexer);
  if (expr == nullptr) {
    return nullptr;
  }

  if (lexer.last_char() != ')') {
    return LogError("expected )");
  }

  lexer.read();
  return expr;
}

// identifierExpr =
//        | identifierName
//        | identifierName '(' expression* ')'
ExprPtr Parser::identifier(Lexer &lexer) {
  std::string identifier = lexer.atom();
  lexer.read();

  if (lexer.last_char() != '(') {
    return std::make_unique<Variable>(identifier);
  }

  lexer.read();

  function::ArgExprs args;

  if (lexer.last_char() != ')') {
    while (true) {
      if (auto arg = expression(lexer)) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (lexer.last_char() == ')') {
        break;
      }

      if (lexer.last_char() != ',') {
        return LogError("Expected ')' or ',' in argument list");
      }

      lexer.read();
    }
  }
  lexer.read();
  return std::make_unique<function::Call>(identifier, std::move(args));
}

// primary =
//       | identifierExpr
//       | numberExpr
//       | paranthesisExpr
ExprPtr Parser::primary(Lexer &lexer) {
  switch (lexer.current()) {
  default:
    return LogError("Unknown token.");
  case Token::identifier:
    return identifier(lexer);
  case Token::number:
    return number(lexer);
  case Token::parenthesisOpen:
    return paranthesis(lexer);
  }
}

ExprPtr Parser::expression(Lexer &lexer) {
  auto lhs = primary(lexer);
  if (lhs == nullptr) {
    return nullptr;
  }

  return binOpRHS(lexer, 0, std::move(lhs));
}

int resolve_precedence(Lexer &lexer) {
  auto query = OP_PRECEDENCE.find(lexer.last_char());
  if (query != OP_PRECEDENCE.end()) {
    return query->second;
  }
  return -1;
}

ExprPtr Parser::binOpRHS(Lexer &lexer, int expr_precedence, ExprPtr lhs) {
  while (true) {
    int precedence = resolve_precedence(lexer);
    if (precedence < expr_precedence) {
      return lhs;
    }

    char binOp = lexer.last_char();
    lexer.read();

    ExprPtr rhs = primary(lexer);
    if (rhs == nullptr) {
      return nullptr;
    }

    int next_precedence = resolve_precedence(lexer);
    if (precedence < next_precedence) {
      // What is this precedence + 1?
      rhs = binOpRHS(lexer, precedence + 1, std::move(rhs));
      if (rhs == nullptr)
        return nullptr;
    }

    lhs = std::make_unique<BinaryOp>(op_from_keyword(binOp), std::move(lhs),
                                     std::move(rhs));
  }
}

Op op_from_keyword(char op) {
  if (op == '+')
    return Op::add;
  if (op == '-')
    return Op::sub;
  if (op == '/')
    return Op::div;
  if (op == '*')
    return Op::mul;

  std::abort();

  // We will never get here, hopefully?
  return Op::unknown;
}

PrototypePtr Parser::prototype(Lexer &lexer) {
  if (lexer.current() != Token::identifier) {
    return LogErrorP("Expected function name in prototype");
  }

  std::string identifier = lexer.atom();
  lexer.read();

  if (lexer.last_char() == '(') {
    return LogErrorP("Expected '(' in prototype");
  }

  std::vector<std::string> args;

  lexer.read();
  while (lexer.current() == Token::identifier) {
    args.push_back(identifier);
    lexer.read();
  }
  if (lexer.last_char() != ')') {
    return LogErrorP("Expected ')' in prototype");
  }

  lexer.read();

  return std::make_unique<function::Prototype>(identifier, std::move(args));
}

ExprPtr Parser::definition(Lexer &lexer) {
  lexer.read();
  std::unique_ptr<function::Prototype> prototype_expr = prototype(lexer);
  if (prototype_expr == nullptr) {
    return nullptr;
  }

  ExprPtr body = expression(lexer);
  if (body != nullptr) {
    return std::make_unique<function::Definition>(std::move(prototype_expr),
                                                  std::move(body));
  }

  return nullptr;
}

DefinitionPtr Parser::top(Lexer &lexer) {
  ExprPtr expr = expression(lexer);
  if (expr != nullptr) {
    PrototypePtr prototype_expr =
        std::make_unique<function::Prototype>("__anon_expr", function::Args());
    return std::make_unique<function::Definition>(std::move(prototype_expr),
                                                  std::move(expr));
  }
  return nullptr;
}

PrototypePtr Parser::extern_(Lexer &lexer) {
  lexer.read();
  return prototype(lexer);
}
