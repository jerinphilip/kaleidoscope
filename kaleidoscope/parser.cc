#include "parser.h"
#include "ast.h"

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
  // Consume the '('
  lexer.read();

  auto expr = expression(lexer);
  if (expr == nullptr) {
    return nullptr;
  }

  if (lexer.current() != ')') {
    return LogError("expected )");
  }

  // Consume the ')'
  lexer.read();
  return expr;
}

// identifierExpr =
//        | identifierName
//        | identifierName '(' expression* ')'
ExprPtr Parser::identifier(Lexer &lexer) {
  std::string identifier = lexer.atom();
  lexer.read(); // Consume identifier

  if (lexer.current() != '(') {
    return std::make_unique<Variable>(identifier);
  }

  lexer.read(); // Consume '('

  function::ArgExprs args;
  if (lexer.current() != ')') {
    while (true) {
      if (auto arg = expression(lexer)) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (lexer.current() == ')') {
        lexer.read(); // Consume ')'
        break;
      }

      if (lexer.current() != ',') {
        return LogError("Expected ')' or ',' in argument list");
      } else {
        lexer.read(); // Consume ','
      }
      fprintf(stderr, "FnArg: %s", lexer.atom().c_str());
    }
  }

  return std::make_unique<function::Call>(identifier, std::move(args));
}

// primary =
//       | identifierExpr
//       | numberExpr
//       | paranthesisExpr
ExprPtr Parser::primary(Lexer &lexer) {
  switch (lexer.type()) {
  default:
    fprintf(stderr, "Unknown token {%s}\n", lexer.atom().c_str());
    return LogError("Unknown token.");
  case Atom::identifier:
    return identifier(lexer);
  case Atom::number:
    return number(lexer);
  case Atom::open:
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

int resolve_precedence(char op) {
  auto query = OP_PRECEDENCE.find(op);
  if (query != OP_PRECEDENCE.end()) {
    return query->second;
  }
  return -1;
}

ExprPtr Parser::binOpRHS(Lexer &lexer, int expr_precedence, ExprPtr lhs) {
  while (true) {
    char binOp = lexer.current();
    int precedence = resolve_precedence(binOp);
    if (precedence < expr_precedence) {
      // Case of -1, when we can return just the primary expression.
      return lhs;
    }

    // Consume the pending binOp. If there is no binOp, the expression above
    // would have returned.
    lexer.read();

    // See what rhs has for us. Attempt to parse again.
    ExprPtr rhs = primary(lexer);
    if (rhs == nullptr) {
      return nullptr;
    }

    // Do we have a binOp behind the rhs?
    char nextBinOp = lexer.current();
    int next_precedence = resolve_precedence(nextBinOp);

    if (precedence < next_precedence) {
      // Recursive call, should take care of (op, operand)*
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
  if (op == '<')
    return Op::lt;

  std::abort();

  // We will never get here, hopefully?
  return Op::unknown;
}

PrototypePtr Parser::prototype(Lexer &lexer) {
  if (lexer.type() != Atom::identifier) {
    return LogErrorP("Expected function name in prototype");
  }

  std::string identifier = lexer.atom();
  fprintf(stderr, "Identifier %s\n", identifier.c_str());

  lexer.read();
  if (lexer.current() != '(') {
    return LogErrorP("Expected '(' in prototype");
  }

  // Consume '('; Should have argument next;
  lexer.read();

  std::vector<std::string> args;

  while (lexer.type() == Atom::identifier) {
    std::string arg = lexer.atom();
    args.push_back(arg);
    fprintf(stderr, "arg: %s\n", arg.c_str());
    lexer.read();
  }

  if (lexer.current() != ')') {
    return LogErrorP("Expected ')' in prototype");
  } else {
    lexer.read(); // Consume ')'
  }

  return std::make_unique<function::Prototype>(identifier, std::move(args));
}

DefinitionPtr Parser::definition(Lexer &lexer) {
  lexer.read(); // Consume `def`
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
