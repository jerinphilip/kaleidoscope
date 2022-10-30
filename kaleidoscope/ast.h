#pragma once
#include "llvm/IR/Value.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

static llvm::LLVMContext CONTEXT;
static llvm::IRBuilder<> BUILDER(CONTEXT);
static std::unique_ptr<llvm::Module> MODULE;
static std::map<std::string, llvm::Value *> NAMED_VALUES;

llvm::Value *LogErrorV(const char *str);

class Expr {
public:
  virtual ~Expr();
  virtual llvm::Value *codegen() const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

enum class Op { add, sub, mul, div, mod, lt, unknown };

class Number : public Expr {
public:
  Number(double value) : value_(value) {}
  llvm::Value *codegen() const final;

private:
  double value_;
};

class Variable : public Expr {
public:
  Variable(const std::string &name) : name_(name) {}
  llvm::Value *codegen() const final;

private:
  std::string name_;
};

class BinaryOp : public Expr {
public:
  BinaryOp(Op op, ExprPtr lhs, ExprPtr rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  llvm::Value *codegen() const final;

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

  llvm::Function *codegen() const;

private:
  std::string name_;
  Args args_;
};

class Definition {
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
  llvm::Value *codegen() const final;

private:
  std::string name_;
  ArgExprs args_;
};
} // namespace function
