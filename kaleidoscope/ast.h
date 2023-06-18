#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lexer.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

class CodegenContext;

llvm::Value *LogErrorV(const char *str);

class Expr {
 public:
  explicit Expr(SourceLocation source_location);
  virtual ~Expr();
  virtual llvm::Value *codegen(CodegenContext &codegen_ctx) const = 0;
  virtual const SourceLocation &location() const;
  virtual llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent);

 private:
  SourceLocation source_location_;
};

using ExprPtr = std::unique_ptr<Expr>;

// NOLINTNEXTLINE
enum class Op { add, sub, mul, div, mod, lt, unknown };

class Number : public Expr {
 public:
  Number(double value, SourceLocation source_location);
  llvm::Value *codegen(CodegenContext &codegen_context) const final;
  llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent) final;

 private:
  double value_;
};

class Variable : public Expr {
 public:
  Variable(std::string name, SourceLocation source_location);
  llvm::Value *codegen(CodegenContext &codegen_context) const final;
  llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent) final;

 private:
  std::string name_;
};

class VarIn : public Expr {
 public:
  using Assignment = std::pair<std::string, ExprPtr>;
  VarIn(std::vector<Assignment> assignments, ExprPtr body,
        SourceLocation source_location);
  llvm::Value *codegen(CodegenContext &codegen_context) const final;
  llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent_level) final;

 private:
  std::vector<Assignment> assignments_;
  ExprPtr body_;
};

class BinaryOp : public Expr {
 public:
  BinaryOp(Op op, ExprPtr lhs, ExprPtr rhs, SourceLocation source_location);
  llvm::Value *codegen(CodegenContext &codegen_context) const final;
  llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent_level) final;

 private:
  Op op_;
  ExprPtr lhs_, rhs_;
};

class IfThenElse : public Expr {
 public:
  IfThenElse(ExprPtr condition, ExprPtr then, ExprPtr otherwise,
             SourceLocation source_location);
  llvm::Value *codegen(CodegenContext &codegen_context) const final;
  llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent_level) final;

 private:
  ExprPtr condition_;
  ExprPtr then_;
  ExprPtr otherwise_;
};

class For : public Expr {
 public:
  For(std::string var, ExprPtr start, ExprPtr end, ExprPtr step, ExprPtr body,
      SourceLocation source_location);
  llvm::Value *codegen(CodegenContext &codegen_context) const final;
  llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent_level) final;

 private:
  std::string var_;

  ExprPtr start_;
  ExprPtr end_;
  ExprPtr step_;

  ExprPtr body_;
};

namespace function {
using ArgExprs = std::vector<ExprPtr>;
using Args = std::vector<std::string>;

class Prototype;
class Definition;
class Call;

}  // namespace function

using PrototypePtr = std::unique_ptr<function::Prototype>;
using DefinitionPtr = std::unique_ptr<function::Definition>;

namespace function {

class Prototype {
 public:
  Prototype(std::string name, Args args);
  llvm::Function *codegen(CodegenContext &codegen_context) const;
  const std::string &name() { return name_; };

 private:
  std::string name_;
  Args args_;
};

class Definition {
 public:
  Definition(PrototypePtr prototype, ExprPtr body);
  llvm::Function *codegen(CodegenContext &codegen_context) const;

 private:
  PrototypePtr prototype_;
  ExprPtr body_;
};

class Call : public Expr {
 public:
  Call(std::string name, ArgExprs args, SourceLocation source_location);
  llvm::Value *codegen(CodegenContext &codegen_context) const final;
  llvm::raw_ostream &dump(llvm::raw_ostream &out, int indent_level) final;

 private:
  std::string name_;
  ArgExprs args_;
};

}  // namespace function
