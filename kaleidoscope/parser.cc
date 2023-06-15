#include "parser.h"

#include <memory>

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
  double value = strtod(lexer.atom().c_str(), nullptr);
  auto result = std::make_unique<Number>(value);
  lexer.read();
  return result;
}

// paranthesisExpr = '(' expression ')'
// NOLINTNEXTLINE(misc-no-recursion)
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
// NOLINTNEXTLINE(misc-no-recursion)
ExprPtr Parser::identifier(Lexer &lexer) {
  std::string identifier = lexer.atom();
  lexer.read();  // Consume identifier

  if (lexer.current() != '(') {
    return std::make_unique<Variable>(identifier);
  }

  lexer.read();  // Consume '('

  function::ArgExprs args;
  if (lexer.current() != ')') {
    while (true) {
      if (auto arg = expression(lexer)) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (lexer.current() == ')') {
        lexer.read();  // Consume ')'
        break;
      }

      if (lexer.current() != ',') {
        return LogError("Expected ')' or ',' in argument list");
      }
      lexer.read();  // Consume ','

      // fprintf(stderr, "FnArg: %s", lexer.atom().c_str());
    }
  }

  return std::make_unique<function::Call>(identifier, std::move(args));
}

// primary =
//       | identifierExpr
//       | numberExpr
//       | paranthesisExpr
//       | ifExpr
//       | forExpr
//       | varExpr
// NOLINTNEXTLINE(misc-no-recursion)
ExprPtr Parser::primary(Lexer &lexer) {
  switch (lexer.type()) {
    default:
      // fprintf(stderr, "Unknown token {%s}\n", lexer.atom().c_str());
      char error_buffer[100];
      snprintf(error_buffer, 100, "Unknown token {%s}", lexer.atom().c_str());
      return LogError(error_buffer);
    case Atom::kIdentifier:
      return identifier(lexer);
    case Atom::kNumber:
      return number(lexer);
    case Atom::kOpen:
      return paranthesis(lexer);
    case Atom::kKeywordIf:
      return if_then_else(lexer);
    case Atom::kKeywordFor:
      return for_in(lexer);
    case Atom::kKeywordVar:
      return var(lexer);
  }
}

// NOLINTNEXTLINE(misc-no-recursion)
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

// NOLINTNEXTLINE(misc-no-recursion)
ExprPtr Parser::binOpRHS(Lexer &lexer, int expr_precedence, ExprPtr lhs) {
  while (true) {
    char bin_op = lexer.current();
    // fprintf(stderr, "binOp: %c\n", binOp);
    int precedence = resolve_precedence(bin_op);
    // fprintf(stderr, "binOp-prec: %d, expr-prec: %d\n", precedence,
    //         expr_precedence);
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
    char next_bin_op = lexer.current();
    int next_precedence = resolve_precedence(next_bin_op);
    // fprintf(stderr, "nextBinOp: %c\n", nextBinOp);
    // fprintf(stderr, "binOp-prec: %d, nextBinOp-prec: %d\n", precedence,
    //         next_precedence);

    if (precedence < next_precedence) {
      // Recursive call, should take care of (op, operand)*
      rhs = binOpRHS(lexer, precedence + 1, std::move(rhs));
      if (rhs == nullptr) return nullptr;
    }

    lhs = std::make_unique<BinaryOp>(op_from_keyword(bin_op), std::move(lhs),
                                     std::move(rhs));
  }
}

Op op_from_keyword(char op) {
  if (op == '+') return Op::add;
  if (op == '-') return Op::sub;
  if (op == '/') return Op::div;
  if (op == '*') return Op::mul;
  if (op == '<') return Op::lt;

  std::abort();

  // We will never get here, hopefully?
  return Op::unknown;
}

PrototypePtr Parser::prototype(Lexer &lexer) {
  if (lexer.type() != Atom::kIdentifier) {
    return LogErrorP("Expected function name in prototype");
  }

  std::string identifier = lexer.atom();
  // fprintf(stderr, "Identifier %s\n", identifier.c_str());

  lexer.read();
  if (lexer.current() != '(') {
    return LogErrorP("Expected '(' in prototype");
  }

  // Consume '('; Should have argument next;
  lexer.read();

  std::vector<std::string> args;

  while (lexer.type() == Atom::kIdentifier) {
    std::string arg = lexer.atom();
    args.push_back(arg);
    // fprintf(stderr, "arg: %s\n", arg.c_str());
    lexer.read();
  }

  if (lexer.current() != ')') {
    return LogErrorP("Expected ')' in prototype");
  }
  lexer.read();  // Consume ')'

  return std::make_unique<function::Prototype>(identifier, std::move(args));
}

DefinitionPtr Parser::definition(Lexer &lexer) {
  lexer.read();  // Consume `def`
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

// NOLINTNEXTLINE(misc-no-recursion)
ExprPtr Parser::if_then_else(Lexer &lexer) {
  lexer.read();  // Consume `if`.
  ExprPtr condition = expression(lexer);

  if (!condition) {
    return nullptr;
  }

  if (lexer.type() != Atom::kKeywordThen) {
    return LogError("Expected `then`");
  }

  lexer.read();  // Consume `then`.
  ExprPtr then = expression(lexer);
  if (!then) {
    return nullptr;
  }

  if (lexer.type() != Atom::kKeywordElse) {
    return LogError("Expected `else`");
  }

  lexer.read();  // Consume `else`.

  ExprPtr otherwise = expression(lexer);

  if (!otherwise) {
    return nullptr;
  }

  return std::make_unique<IfThenElse>(std::move(condition), std::move(then),
                                      std::move(otherwise));
}

// NOLINTNEXTLINE(misc-no-recursion)
ExprPtr Parser::for_in(Lexer &lexer) {
  lexer.read();  // consume `for`.

  if (lexer.type() != Atom::kIdentifier) {
    return LogError("expected identifier after `for`");
  }

  std::string identifier = lexer.atom();
  lexer.read();  // consume identifier.

  if (lexer.current() != '=') return LogError("expected '=' after for");
  lexer.read();  // consume '='.

  auto start = expression(lexer);
  if (!start) {
    return nullptr;
  }

  if (lexer.current() != ',') {
    return LogError("expected ',' after for start value");
  }

  lexer.read();

  auto end = expression(lexer);
  if (!end) {
    return nullptr;
  }

  // The step value is optional.
  ExprPtr step = nullptr;
  if (lexer.current() == ',') {
    lexer.read();
    step = expression(lexer);
    if (!step) {
      return nullptr;
    }
  }

  if (lexer.type() != Atom::kKeywordIn) {
    return LogError("expected 'in' after for");
  }
  lexer.read();  // consume 'in'.

  auto body = expression(lexer);
  if (!body) {
    return nullptr;
  }

  return std::make_unique<For>(identifier, std::move(start), std::move(end),
                               std::move(step), std::move(body));
}

// NOLINTNEXTLINE(misc-no-recursion)
ExprPtr Parser::var(Lexer &lexer) {
  lexer.read();  // consume `var`.
  std::vector<VarIn::Assignment> assignments;

  if (lexer.type() != Atom::kIdentifier) {
    return LogError("Expected at least one identifier.");
  }

  // Read the variable name and assignment list.
  while (true) {
    std::string identifier = lexer.atom();
    lexer.read();  // consume identifier.

    // Optional initializer.
    ExprPtr init;
    if (lexer.current() == '=') {
      // Consume `=`
      lexer.read();
      init = expression(lexer);

      if (!init) {
        return nullptr;
      }
    }

    assignments.emplace_back(identifier, std::move(init));

    // Do we have more comma separated variables?
    // If not, break.
    if (lexer.current() != ',') {
      break;
    }

    lexer.read();  // Consume the `,`
    if (lexer.type() != Atom::kIdentifier) {
      return LogError("Expected identifier list after `var`.");
    }
  }

  // Read `in` expression to follow.
  if (lexer.type() != Atom::kKeywordIn) {
    return LogError("Expected `in` keyword after `var`");
  }

  lexer.read();  // Consume `in`.

  ExprPtr body = expression(lexer);
  if (!body) {
    return nullptr;
  }

  return std::make_unique<VarIn>(std::move(assignments), std::move(body));

  return nullptr;
}
