#pragma once
#include "ast.h"
#include "lexer.h"

ExprPtr LogError(const char *message);
std::unique_ptr<function::Prototype> LogErrorP(const char *message);
Op op_from_keyword(char op);
std::string keyword_from_op(Op op);

int resolve_precedence(char op);

class Parser {
 public:
  // numberExpr = number
  static ExprPtr number(Lexer &lexer);

  // paranthesisExpr = '(' expression ')'
  ExprPtr paranthesis(Lexer &lexer);

  // identifierExpr =
  //        | identifierName
  //        | identifierName '(' expression* ')'
  ExprPtr identifier(Lexer &lexer);

  // primary =
  //       | identifierExpr
  //       | numberExpr
  //       | paranthesisExpr
  ExprPtr primary(Lexer &lexer);

  // expression =
  //       | primary
  //       | primary `op` expression
  ExprPtr expression(Lexer &lexer);

  // binOpRHS = ('+' primary)*
  ExprPtr binOpRHS(Lexer &lexer, int expr_precedence, ExprPtr lhs);

  // prototype = id '(' id* ')'
  static PrototypePtr prototype(Lexer &lexer);

  /// definition = 'def' prototype expression
  DefinitionPtr definition(Lexer &lexer);

  /// external = 'extern' prototype
  static PrototypePtr extern_(Lexer &lexer);

  /// top = expression
  DefinitionPtr top(Lexer &lexer);

  /// if = `if` expression `then` expression `else` expression
  ExprPtr if_then_else(Lexer &lexer);

  /// for =
  ///     | `for` identifier = expr, expr [, expr]  `in` expr
  ///
  ExprPtr for_in(Lexer &lexer);

  /// varExpr := `var` identifer
  ///               ('=' expression)?
  ///               (',' identifier '=' expression)?*
  ///               `in` expression
  ExprPtr var(Lexer &lexer);
};
