#include "ast.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

using llvm::APFloat;
using llvm::BasicBlock;
using llvm::ConstantFP;
using llvm::Function;
using llvm::FunctionType;
using llvm::Type;
using llvm::Value;

llvm::Value *LogErrorV(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  return nullptr;
}

Expr::~Expr() {}

Value *Number::codegen(LLVMConnector &llvms) const {
  return ConstantFP::get(llvms.context(), APFloat(value_));
}

Value *Variable::codegen(LLVMConnector &llvms) const {
  // Look this variable up in the function.
  Value *value = llvms.lookup(name_);
  if (!value)
    LogErrorV("Unknown variable name");
  return value;
}

Value *BinaryOp::codegen(LLVMConnector &llvms) const {
  Value *lhs = lhs_->codegen(llvms);
  Value *rhs = rhs_->codegen(llvms);
  if (!lhs || !rhs)
    return nullptr;

  llvm::IRBuilder<> &builder = llvms.builder();
  switch (op_) {
  case Op::add:
    return builder.CreateFAdd(lhs, rhs, "addtmp");
  case Op::sub:
    return builder.CreateFSub(lhs, rhs, "subtmp");
  case Op::mul:
    return builder.CreateFMul(lhs, rhs, "multmp");
  case Op::div:
    return builder.CreateFMul(lhs, rhs, "divtmp");
  case Op::lt:
    lhs = builder.CreateFCmpULT(lhs, rhs, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return builder.CreateUIToFP(lhs, Type::getDoubleTy(llvms.context()),
                                "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

namespace function {

Value *Call::codegen(LLVMConnector &llvms) const {
  // Look up the name in the global module table.
  Function *fn = llvms.module().getFunction(name_);
  if (!fn)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (fn->arg_size() != args_.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> arg_values;
  for (unsigned i = 0, e = args_.size(); i != e; ++i) {
    arg_values.push_back(args_[i]->codegen(llvms));
    if (!arg_values.back())
      return nullptr;
  }

  return llvms.builder().CreateCall(fn, arg_values, "calltmp");
}

Function *Prototype::codegen(LLVMConnector &llvms) const {
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> Doubles(args_.size(), Type::getDoubleTy(llvms.context()));
  FunctionType *function_type =
      FunctionType::get(Type::getDoubleTy(llvms.context()), Doubles, false);

  Function *fn = Function::Create(function_type, Function::ExternalLinkage,
                                  name_, llvms.module());

  // Set names for all arguments.
  unsigned idx = 0;
  for (auto &arg : fn->args()) {
    arg.setName(args_[idx++]);
  }

  return fn;
}

Function *Definition::codegen(LLVMConnector &llvms) const {
  // First, check for an existing function from a previous 'extern' declaration.
  Function *fn = llvms.module().getFunction(prototype_->name());

  if (!fn)
    fn = prototype_->codegen(llvms);

  if (!fn)
    return nullptr;

  if (!fn->empty())
    return (Function *)LogErrorV("Function cannot be redefined.");

  // Create a new basic block to start insertion into.
  BasicBlock *basic_block = BasicBlock::Create(llvms.context(), "entry", fn);
  llvms.builder().SetInsertPoint(basic_block);

  // Record the function arguments in the NamedValues map.
  llvms.clear();
  for (auto &arg : fn->args()) {
    llvm::StringRef nameRef = arg.getName();
    std::string name(nameRef.data(), nameRef.size());
    llvms.set(name, &arg);
  }

  if (Value *RetVal = body_->codegen(llvms)) {
    // Finish off the function.
    llvms.builder().CreateRet(RetVal);

    // Validate the generated code, checking for consistency.
    verifyFunction(*fn);

    return fn;
  }

  // Error reading body, remove function.
  fn->eraseFromParent();
  return nullptr;
}

} // namespace function

Value *IfThenElse::codegen(LLVMConnector &llvms) const {
  Value *condition_value = condition_->codegen(llvms);
  if (!condition_value) {
    return nullptr;
  }

  auto &builder = llvms.builder();
  auto &context = llvms.context();

  condition_value = builder.CreateFCmpONE(
      condition_value, ConstantFP::get(context, APFloat(0.0)), "ifcond");

  Function *fn = builder.GetInsertBlock()->getParent();

  BasicBlock *then_block = BasicBlock::Create(context, "then", fn);
  BasicBlock *otherwise_block = BasicBlock::Create(context, "else");
  BasicBlock *merge_block = BasicBlock::Create(context, "ifcont");

  llvms.builder().CreateCondBr(condition_value, then_block, otherwise_block);

  // Emit otherwise value.
  llvms.builder().SetInsertPoint(then_block);
  Value *then_value = then_->codegen(llvms);

  if (!then_value) {
    return nullptr;
  }

  builder.CreateBr(merge_block);

  then_block = builder.GetInsertBlock();

  fn->getBasicBlockList().push_back(otherwise_block);
  builder.SetInsertPoint(otherwise_block);

  Value *otherwise_value = otherwise_->codegen(llvms);
  if (!otherwise_value) {
    return nullptr;
  }

  builder.CreateBr(merge_block);
  otherwise_block = builder.GetInsertBlock();

  fn->getBasicBlockList().push_back(merge_block);
  builder.SetInsertPoint(merge_block);

  llvm::PHINode *phi_node =
      builder.CreatePHI(Type::getDoubleTy(context), 2, "iftmp");

  phi_node->addIncoming(then_value, then_block);
  phi_node->addIncoming(otherwise_value, otherwise_block);

  return phi_node;
}

llvm::Value *For::codegen(LLVMConnector &llvms) const { return nullptr; }
