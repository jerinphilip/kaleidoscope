#pragma once
#include "ast.h"
#include "lexer.h"

ExprPtr LogError(const char *message);
std::unique_ptr<function::Prototype> LogErrorP(const char *message);
Op op_from_keyword(char op);
int resolve_precedence(char op);

class Parser {
public:
  // numberExpr = number
  ExprPtr number(Lexer &lexer);

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
  PrototypePtr prototype(Lexer &lexer);

  /// definition = 'def' prototype expression
  DefinitionPtr definition(Lexer &lexer);

  /// external = 'extern' prototype
  PrototypePtr extern_(Lexer &lexer);

  /// top = expression
  DefinitionPtr top(Lexer &lexer);
};
