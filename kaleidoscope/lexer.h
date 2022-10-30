#include "ast.h"
#include <cstdio>
#include <map>
#include <memory>
#include <string>

enum class Token {
  eof = -1,
  def = -2,
  extern_ = -3,
  identifier = -4,
  number = -5,
  init = -6,
  semicolon = -7,
  comment = -8,
  unknown = -9,
  parenthesisOpen = -10
};

class Lexer {
public:
  Token read();
  const std::string &atom() const { return atom_; }
  char last_char() const { return last_char_; }
  Token current() const { return last_token_; }

private:
  Token set(Token token) {
    last_token_ = token;
    return token;
  }

  std::string atom_;
  char last_char_;
  Token last_token_;
};

ExprPtr LogError(const char *message);
std::unique_ptr<function::Prototype> LogErrorP(const char *message);
Op op_from_keyword(char op);

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
  ExprPtr definition(Lexer &lexer);

  /// external = 'extern' prototype
  PrototypePtr extern_(Lexer &lexer);

  /// top = expression
  DefinitionPtr top(Lexer &lexer);
};

static std::map<char, int> OP_PRECEDENCE = {
    {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}, {'/', 40}};
