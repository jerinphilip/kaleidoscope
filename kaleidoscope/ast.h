#pragma once
#include "llvm_connector.h"
#include "llvm/IR/Value.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

llvm::Value *LogErrorV(const char *str);

class Expr {
public:
  virtual ~Expr();
  virtual llvm::Value *codegen(LLVMConnector &llvms) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

enum class Op { add, sub, mul, div, mod, lt, unknown };

class Number : public Expr {
public:
  Number(double value) : value_(value) {}
  llvm::Value *codegen(LLVMConnector &llvms) const final;

private:
  double value_;
};

class Variable : public Expr {
public:
  Variable(const std::string &name) : name_(name) {}
  llvm::Value *codegen(LLVMConnector &llvms) const final;

private:
  std::string name_;
};

class BinaryOp : public Expr {
public:
  BinaryOp(Op op, ExprPtr lhs, ExprPtr rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  llvm::Value *codegen(LLVMConnector &llvms) const final;

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

class Prototype {
public:
  Prototype(const std::string &name, Args args)
      : name_(name), args_(std::move(args)) {}

  llvm::Function *codegen(LLVMConnector &llvms) const;

  const std::string &name() { return name_; };

private:
  std::string name_;
  Args args_;
};

class Definition {
public:
  Definition(PrototypePtr prototype, ExprPtr body)
      : prototype_(std::move(prototype)), body_(std::move(body)) {}

  llvm::Function *codegen(LLVMConnector &llvms) const;

private:
  PrototypePtr prototype_;
  ExprPtr body_;
};

class Call : public Expr {
public:
  Call(const std::string &name, ArgExprs args)
      : name_(name), args_(std::move(args)) {}
  llvm::Value *codegen(LLVMConnector &llvms) const final;

private:
  std::string name_;
  ArgExprs args_;
};
} // namespace function

class IfThenElse : public Expr {
public:
  IfThenElse(ExprPtr condition, ExprPtr then, ExprPtr otherwise)
      : condition_(std::move(condition)), then_(std::move(then)),
        otherwise_(std::move(otherwise)) {}

  llvm::Value *codegen(LLVMConnector &llvms) const final;

private:
  ExprPtr condition_;
  ExprPtr then_;
  ExprPtr otherwise_;
};

class For : public Expr {
public:
  For(std::string var, ExprPtr start, ExprPtr end, ExprPtr step, ExprPtr body)
      : var_(std::move(var)), start_(std::move(start)), end_(std::move(end)),
        step_(std::move(step)), body_(std::move(body)) {}

  llvm::Value *codegen(LLVMConnector &llvms) const final;

private:
  std::string var_;

  ExprPtr start_;
  ExprPtr end_;
  ExprPtr step_;

  ExprPtr body_;
};
