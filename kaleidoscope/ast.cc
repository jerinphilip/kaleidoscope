#include "ast.h"

#include "codegen_context.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

using llvm::AllocaInst;
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

Expr::Expr(SourceLocation source_location)
    : source_location_(std::move(source_location)) {}

Expr::~Expr() = default;

Number::Number(double value, SourceLocation source_location)
    : value_(value), Expr(std::move(source_location)) {}

Variable::Variable(std::string name, SourceLocation source_location)
    : Expr(std::move(source_location)), name_(std::move(name)) {}

VarIn::VarIn(std::vector<Assignment> assignments, ExprPtr body,
             SourceLocation source_location)
    : Expr(std::move(source_location)),
      assignments_(std::move(assignments)),
      body_(std::move(body)) {}

BinaryOp::BinaryOp(Op op, ExprPtr lhs, ExprPtr rhs,
                   SourceLocation source_location)
    : Expr(std::move(source_location)),
      op_(op),
      lhs_(std::move(lhs)),
      rhs_(std::move(rhs)) {}

IfThenElse::IfThenElse(ExprPtr condition, ExprPtr then, ExprPtr otherwise,
                       SourceLocation source_location)
    : Expr(std::move(source_location)),
      condition_(std::move(condition)),
      then_(std::move(then)),
      otherwise_(std::move(otherwise)) {}

For::For(std::string var, ExprPtr start, ExprPtr end, ExprPtr step,
         ExprPtr body, SourceLocation source_location)
    : Expr(std::move(source_location)),
      var_(std::move(var)),
      start_(std::move(start)),
      end_(std::move(end)),
      step_(std::move(step)),
      body_(std::move(body)) {}

namespace function {

Prototype::Prototype(std::string name, Args args)
    : name_(std::move(name)), args_(std::move(args)) {}

Definition::Definition(PrototypePtr prototype, ExprPtr body)
    : prototype_(std::move(prototype)), body_(std::move(body)) {}

Call::Call(std::string name, ArgExprs args, SourceLocation source_location)
    : Expr(std::move(source_location)),
      name_(std::move(name)),
      args_(std::move(args)) {}

}  // namespace function

Value *Number::codegen(CodegenContext &codegen_context) const {
  return ConstantFP::get(codegen_context.context(), APFloat(value_));
}

Value *Variable::codegen(CodegenContext &codegen_context) const {
  // Look this variable up in the function.
  AllocaInst *alloca_inst = codegen_context.lookup(name_);
  if (!alloca_inst) LogErrorV("Unknown variable name");

  // Load the value
  auto &builder = codegen_context.builder();
  return builder.CreateLoad(alloca_inst->getAllocatedType(), alloca_inst,
                            name_.c_str());
}

Value *VarIn::codegen(CodegenContext &codegen_context) const {
  // Look this variable up in the function.
  std::vector<AllocaInst *> old_bindings;

  auto &builder = codegen_context.builder();

  Function *fn = builder.GetInsertBlock()->getParent();

  // Register all variables - emit initializer
  for (const auto &assignment : assignments_) {
    const std::string &name = assignment.first;
    Expr *init = (assignment.second).get();

    Value *init_value;
    if (init) {
      init_value = init->codegen(codegen_context);
      if (!init_value) {
        return nullptr;
      }
    } else {
      init_value = ConstantFP::get(codegen_context.context(), APFloat(0.0));
    }

    AllocaInst *alloca = codegen_context.create_entry_block_alloca(fn, name);
    builder.CreateStore(init_value, alloca);

    old_bindings.push_back(codegen_context.lookup(name));
    codegen_context.set(name, alloca);
  }

  Value *body_value = body_->codegen(codegen_context);
  if (!body_value) {
    return nullptr;
  }

  for (size_t i = 0; i < assignments_.size(); i++) {
    const std::string &name = assignments_[i].first;
    codegen_context.set(name, old_bindings[i]);
  }

  return body_value;
}

Value *BinaryOp::codegen(CodegenContext &codegen_context) const {
  Value *lhs = lhs_->codegen(codegen_context);
  Value *rhs = rhs_->codegen(codegen_context);
  if (!lhs || !rhs) return nullptr;

  llvm::IRBuilder<> &builder = codegen_context.builder();
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
      // returns an ‘i1’ value (a one bit integer). The problem with this is
      // that Kaleidoscope wants the value to be a 0.0 or 1.0 value. In order to
      // get these semantics, we combine the fcmp instruction with a uitofp
      // instruction. This instruction converts its input integer into a
      // floating point value by treating the input as an unsigned value. In
      // contrast, if we used the sitofp instruction, the Kaleidoscope ‘<’
      // operator would return 0.0 and -1.0, depending on the input value.

      return builder.CreateUIToFP(
          lhs, Type::getDoubleTy(codegen_context.context()), "booltmp");
    default:
      return LogErrorV("invalid binary operator");
  }
}

namespace function {

Value *Call::codegen(CodegenContext &codegen_context) const {
  // Look up the name in the global module table.
  Function *fn = codegen_context.module().getFunction(name_);
  if (!fn) return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (fn->arg_size() != args_.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> arg_values;
  for (const auto &arg : args_) {
    arg_values.push_back(arg->codegen(codegen_context));
    if (!arg_values.back()) return nullptr;
  }

  return codegen_context.builder().CreateCall(fn, arg_values, "calltmp");
}

Function *Prototype::codegen(CodegenContext &codegen_context) const {
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> doubles(args_.size(),
                              Type::getDoubleTy(codegen_context.context()));
  FunctionType *function_type =
      FunctionType::get(Type::getDoubleTy(codegen_context.context()), doubles,
                        /*isVarArg=*/false);

  Function *fn = Function::Create(function_type, Function::ExternalLinkage,
                                  name_, codegen_context.module());

  // Set names for all arguments.
  unsigned idx = 0;
  for (auto &arg : fn->args()) {
    arg.setName(args_[idx++]);
  }

  return fn;
}

Function *Definition::codegen(CodegenContext &codegen_context) const {
  // First, check for an existing function from a previous 'extern' declaration.
  Function *fn = codegen_context.module().getFunction(prototype_->name());

  if (!fn) fn = prototype_->codegen(codegen_context);

  if (!fn) return nullptr;

  if (!fn->empty())
    return static_cast<Function *>(LogErrorV("Function cannot be redefined."));

  // Create a new basic block to start insertion into.
  BasicBlock *basic_block =
      BasicBlock::Create(codegen_context.context(), "entry", fn);

  auto &builder = codegen_context.builder();
  builder.SetInsertPoint(basic_block);

  // Record the function arguments in the NamedValues map.
  codegen_context.clear();
  for (auto &arg : fn->args()) {
    llvm::StringRef name_ref = arg.getName();
    std::string name(name_ref.data(), name_ref.size());

    AllocaInst *alloca = codegen_context.create_entry_block_alloca(fn, name);
    builder.CreateStore(&arg, alloca);
    codegen_context.set(name, alloca);
  }

  if (Value *ret_val = body_->codegen(codegen_context)) {
    // Finish off the function.
    builder.CreateRet(ret_val);

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

}  // namespace function

Value *IfThenElse::codegen(CodegenContext &codegen_context) const {
  Value *condition_value = condition_->codegen(codegen_context);
  if (!condition_value) {
    return nullptr;
  }

  auto &builder = codegen_context.builder();
  auto &context = codegen_context.context();

  condition_value = builder.CreateFCmpONE(
      condition_value, ConstantFP::get(context, APFloat(0.0)), "ifcond");

  Function *fn = builder.GetInsertBlock()->getParent();

  BasicBlock *then_block = BasicBlock::Create(context, "then", fn);
  BasicBlock *otherwise_block = BasicBlock::Create(context, "else");
  BasicBlock *merge_block = BasicBlock::Create(context, "ifcont");

  codegen_context.builder().CreateCondBr(condition_value, then_block,
                                         otherwise_block);

  // Emit otherwise value.
  codegen_context.builder().SetInsertPoint(then_block);
  Value *then_value = then_->codegen(codegen_context);

  if (!then_value) {
    return nullptr;
  }

  builder.CreateBr(merge_block);

  then_block = builder.GetInsertBlock();

  fn->getBasicBlockList().push_back(otherwise_block);
  builder.SetInsertPoint(otherwise_block);

  Value *otherwise_value = otherwise_->codegen(codegen_context);
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

llvm::Value *For::codegen(CodegenContext &codegen_context) const {
  auto &builder = codegen_context.builder();
  Function *fn = builder.GetInsertBlock()->getParent();

  // Emit the start code first, without 'variable' in scope.
  AllocaInst *alloca = codegen_context.create_entry_block_alloca(fn, var_);

  Value *start_value = start_->codegen(codegen_context);
  if (!start_value) return nullptr;

  auto &context = codegen_context.context();

  // Make the new basic block for the loop header, inserting after current
  // block.
  builder.CreateStore(start_value, alloca);

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
  AllocaInst *old_value = codegen_context.lookup(var_);
  codegen_context.set(var_, alloca);

  // Emit the body of the loop.  This, like any other expr, can change the
  // current BB.  Note that we ignore the value computed by the body, but don't
  // allow an error.
  if (!body_->codegen(codegen_context)) return nullptr;

  // Emit the step value.
  Value *step_value = nullptr;
  if (step_) {
    step_value = step_->codegen(codegen_context);
    if (!step_value) return nullptr;
  } else {
    // If not specified, use 1.0.
    step_value = ConstantFP::get(context, APFloat(1.0));
  }

  Value *current_var =
      builder.CreateLoad(alloca->getAllocatedType(), alloca, var_.c_str());
  Value *next_var = builder.CreateFAdd(variable, step_value, "nextvar");

  builder.CreateStore(next_var, alloca);

  // Compute the end condition.
  Value *end_condition = end_->codegen(codegen_context);
  if (!end_condition) return nullptr;

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
    codegen_context.set(var_, old_value);
  } else {
    codegen_context.erase(var_);
  }

  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getDoubleTy(context));
}
