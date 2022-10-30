
#include "ast.h"
#include "llvm/IR/Value.h"

using llvm::APFloat;
using llvm::ConstantFP;
using llvm::Function;
using llvm::Type;
using llvm::Value;

llvm::Value *LogErrorV(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  return nullptr;
}

Expr::~Expr() {}

Value *Number::codegen() const {
  return ConstantFP::get(CONTEXT, APFloat(value_));
}

Value *Variable::codegen() const {
  // Look this variable up in the function.
  Value *value = NAMED_VALUES[name_];
  if (!value)
    LogErrorV("Unknown variable name");
  return value;
}

Value *BinaryOp::codegen() const {
  Value *lhs = lhs_->codegen();
  Value *rhs = rhs_->codegen();
  if (!lhs || !rhs)
    return nullptr;

  switch (op_) {
  case Op::add:
    return BUILDER.CreateFAdd(lhs, rhs, "addtmp");
  case Op::sub:
    return BUILDER.CreateFSub(lhs, rhs, "subtmp");
  case Op::mul:
    return BUILDER.CreateFMul(lhs, rhs, "multmp");
  case Op::lt:
    lhs = BUILDER.CreateFCmpULT(lhs, rhs, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return BUILDER.CreateUIToFP(lhs, Type::getDoubleTy(CONTEXT), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

namespace function {

Value *Call::codegen() const {
  // Look up the name in the global module table.
  Function *fn = MODULE->getFunction(name_);
  if (!fn)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (fn->arg_size() != args_.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> arg_values;
  for (unsigned i = 0, e = args_.size(); i != e; ++i) {
    arg_values.push_back(args_[i]->codegen());
    if (!arg_values.back())
      return nullptr;
  }

  return BUILDER.CreateCall(fn, arg_values, "calltmp");
}
} // namespace function
