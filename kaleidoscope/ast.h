#pragma once
#include <memory>
#include <string>
#include <vector>

class Expr {
public:
  virtual ~Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

enum class Op { add, sub, mul, div, mod, unknown };

class Number : public Expr {
public:
  Number(double value) : value_(value) {}

private:
  double value_;
};

class Variable : public Expr {
public:
  Variable(const std::string &name) : name_(name) {}

private:
  std::string name_;
};

class BinaryOp : public Expr {
public:
  BinaryOp(Op op, ExprPtr lhs, ExprPtr rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

private:
  Op op_;
  ExprPtr lhs_, rhs_;
};

namespace function {
using ArgExprs = std::vector<ExprPtr>;
using Args = std::vector<std::string>;

class Prototype;
class Definition;
class Call;

} // namespace function

using PrototypePtr = std::unique_ptr<function::Prototype>;
using DefinitionPtr = std::unique_ptr<function::Definition>;

namespace function {

class Prototype : public Expr {
public:
  Prototype(const std::string &name, Args args)
      : name_(name), args_(std::move(args)) {}

private:
  std::string name_;
  Args args_;
};

class Definition : public Expr {
public:
  Definition(PrototypePtr prototype, ExprPtr body)
      : prototype_(std::move(prototype)), body_(std::move(body)) {}

private:
  PrototypePtr prototype_;
  ExprPtr body_;
};

class Call : public Expr {
public:
  Call(const std::string &name, ArgExprs args)
      : name_(name), args_(std::move(args)) {}

private:
  std::string name_;
  ArgExprs args_;
};
} // namespace function
