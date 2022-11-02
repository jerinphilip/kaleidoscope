#include "ast.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

using llvm::APFloat;
using llvm::BasicBlock;
using llvm::Constant;
using llvm::ConstantFP;
using llvm::Function;
using llvm::FunctionType;
using llvm::PHINode;
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
    //
    // On the other hand, LLVM specifies that the fcmp instruction always
    // returns an ‘i1’ value (a one bit integer). The problem with this is that
    // Kaleidoscope wants the value to be a 0.0 or 1.0 value. In order to get
    // these semantics, we combine the fcmp instruction with a uitofp
    // instruction. This instruction converts its input integer into a floating
    // point value by treating the input as an unsigned value. In contrast, if
    // we used the sitofp instruction, the Kaleidoscope ‘<’ operator would
    // return 0.0 and -1.0, depending on the input value.

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
  FunctionType *function_type = FunctionType::get(
      Type::getDoubleTy(llvms.context()), Doubles, /*vararg=*/false);

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

    // This function does a variety of consistency checks on the generated code,
    // to determine if our compiler is doing everything right. Using this is
    // important: it can catch a lot of bugs.
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

  PHINode *phi_node = builder.CreatePHI(Type::getDoubleTy(context), 2, "iftmp");

  phi_node->addIncoming(then_value, then_block);
  phi_node->addIncoming(otherwise_value, otherwise_block);

  return phi_node;
}

llvm::Value *For::codegen(LLVMConnector &llvms) const {
  // Emit the start code first, without 'variable' in scope.
  Value *start_value = start_->codegen(llvms);
  if (!start_value)
    return nullptr;

  auto &builder = llvms.builder();
  auto &context = llvms.context();

  // Make the new basic block for the loop header, inserting after current
  // block.
  Function *fn = builder.GetInsertBlock()->getParent();
  BasicBlock *pre_header_block = builder.GetInsertBlock();
  BasicBlock *loop_block = BasicBlock::Create(context, "loop", fn);

  // Insert an explicit fall through from the current block to the LoopBB.
  builder.CreateBr(loop_block);

  // Start insertion in LoopBB.
  builder.SetInsertPoint(loop_block);

  // Start the PHI node with an entry for start_.
  PHINode *variable = builder.CreatePHI(Type::getDoubleTy(context), 2, var_);
  variable->addIncoming(start_value, pre_header_block);

  // Within the loop, the variable is defined equal to the PHI node.  If it
  // shadows an existing variable, we have to restore it, so save it now.
  Value *old_value = llvms.lookup(var_);
  llvms.set(var_, variable);

  // Emit the body of the loop.  This, like any other expr, can change the
  // current BB.  Note that we ignore the value computed by the body, but don't
  // allow an error.
  if (!body_->codegen(llvms))
    return nullptr;

  // Emit the step value.
  Value *step_value = nullptr;
  if (step_) {
    step_value = step_->codegen(llvms);
    if (!step_value)
      return nullptr;
  } else {
    // If not specified, use 1.0.
    step_value = ConstantFP::get(context, APFloat(1.0));
  }

  Value *next_var = builder.CreateFAdd(variable, step_value, "nextvar");

  // Compute the end condition.
  Value *end_condition = end_->codegen(llvms);
  if (!end_condition)
    return nullptr;

  // Convert condition to a bool by comparing non-equal to 0.0.
  end_condition = builder.CreateFCmpONE(
      end_condition, ConstantFP::get(context, APFloat(0.0)), "loopcond");

  // Create the "after loop" block and insert it.
  BasicBlock *loop_end_block = builder.GetInsertBlock();
  BasicBlock *after_block = BasicBlock::Create(context, "afterloop", fn);

  // Insert the conditional branch into the end of LoopEndBB.
  builder.CreateCondBr(end_condition, loop_end_block, after_block);

  // Any new code will be inserted in AfterBB.
  builder.SetInsertPoint(after_block);

  // Add a new entry to the PHI node for the backedge.
  variable->addIncoming(next_var, loop_end_block);

  // Restore the unshadowed variable.
  if (old_value) {
    llvms.set(var_, old_value);
  } else {
    llvms.erase(var_);
  }

  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getDoubleTy(context));
}
