#pragma once
#include "llvm/IR/Value.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

class LLVMStuff {
public:
  LLVMStuff(const std::string name)
      : context_(), module_(name, context_), builder_(context_) {}

  llvm::LLVMContext &context() { return context_; }
  llvm::Module &module() { return module_; };
  llvm::IRBuilder<> &builder() { return builder_; }

  llvm::Value *lookup(const std::string &name) {
    auto query = named_values_.find(name);
    if (query == named_values_.end()) {
      return nullptr;
    }
    return query->second;
  }

  void set(const std::string &name, llvm::Value *value) {
    named_values_[name] = value;
  }

  void clear() { named_values_.clear(); }

private:
  llvm::LLVMContext context_;
  llvm::Module module_;
  llvm::IRBuilder<> builder_;
  std::map<std::string, llvm::Value *> named_values_;
};

llvm::Value *LogErrorV(const char *str);

class Expr {
public:
  virtual ~Expr();
  virtual llvm::Value *codegen(LLVMStuff &llvms) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

enum class Op { add, sub, mul, div, mod, lt, unknown };

class Number : public Expr {
public:
  Number(double value) : value_(value) {}
  llvm::Value *codegen(LLVMStuff &llvms) const final;

private:
  double value_;
};

class Variable : public Expr {
public:
  Variable(const std::string &name) : name_(name) {}
  llvm::Value *codegen(LLVMStuff &llvms) const final;

private:
  std::string name_;
};

class BinaryOp : public Expr {
public:
  BinaryOp(Op op, ExprPtr lhs, ExprPtr rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  llvm::Value *codegen(LLVMStuff &llvms) const final;

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

  llvm::Function *codegen(LLVMStuff &llvms) const;

  const std::string &name() { return name_; };

private:
  std::string name_;
  Args args_;
};

class Definition {
public:
  Definition(PrototypePtr prototype, ExprPtr body)
      : prototype_(std::move(prototype)), body_(std::move(body)) {}

  llvm::Function *codegen(LLVMStuff &llvms) const;

private:
  PrototypePtr prototype_;
  ExprPtr body_;
};

class Call : public Expr {
public:
  Call(const std::string &name, ArgExprs args)
      : name_(name), args_(std::move(args)) {}
  llvm::Value *codegen(LLVMStuff &llvms) const final;

private:
  std::string name_;
  ArgExprs args_;
};
} // namespace function
