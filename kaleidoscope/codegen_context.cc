#include "codegen_context.h"

DebugInfo::DebugInfo(const std::string &name, llvm::Module &module)
    : debug_info_builder_(module) {
  compile_unit_ = debug_info_builder_.createCompileUnit(
      llvm::dwarf::DW_LANG_C, debug_info_builder_.createFile(name, "."), "kali",
      false, "", 0);
  type_ = debug_info_builder_.createBasicType("double", 64,
                                              llvm::dwarf::DW_ATE_float);
}

void DebugInfo::emit_location(const Expr *expr, llvm::IRBuilder<> &builder) {
  if (!expr) {
    return builder.SetCurrentDebugLocation(llvm::DebugLoc());
  }

  llvm::DIScope *scope = nullptr;
  if (lexical_blocks_.empty()) {
    scope = compile_unit_;
  } else {
    scope = lexical_blocks_.back();
  }

  const SourceLocation &location = expr->location();
  builder.SetCurrentDebugLocation(llvm::DILocation::get(
      scope->getContext(), location.line, location.column, scope));
}

llvm::DIType *DebugInfo::type() { return type_; }
llvm::DIBuilder &DebugInfo::debug_info_builder() { return debug_info_builder_; }

CodegenContext::CodegenContext(const std::string &name)
    : module_(name, context_), builder_(context_), debug_info_(name, module_) {}

llvm::LLVMContext &CodegenContext::context() { return context_; }
llvm::Module &CodegenContext::module() { return module_; };
llvm::IRBuilder<> &CodegenContext::builder() { return builder_; }

llvm::AllocaInst *CodegenContext::lookup(const std::string &name) {
  auto query = named_values_.find(name);
  if (query == named_values_.end()) {
    return nullptr;
  }
  return query->second;
}

void CodegenContext::set(const std::string &name, llvm::AllocaInst *value) {
  named_values_[name] = value;
}

void CodegenContext::erase(const std::string &name) {
  named_values_.erase(name);
}

void CodegenContext::clear() { named_values_.clear(); }

llvm::AllocaInst *CodegenContext::create_entry_block_alloca(
    llvm::Function *fn, const std::string &variable) {
  llvm::IRBuilder<> temp_builder(&fn->getEntryBlock(),
                                 fn->getEntryBlock().begin());
  return temp_builder.CreateAlloca(llvm::Type::getDoubleTy(context_), nullptr,
                                   variable);
}

llvm::DIBuilder &CodegenContext::debug_info_builder() {
  return debug_info_.debug_info_builder();
}

void CodegenContext::emit_location(const Expr *expr) {
  debug_info_.emit_location(expr, builder_);
}

void DebugInfo::push_subprogram(const std::string &name,
                                const function::Definition *definition,
                                llvm::Function *fn) {
  // Create a subprogram DIE for this function.
  llvm::DIFile *unit = debug_info_builder_.createFile(
      compile_unit_->getFilename(), compile_unit_->getDirectory());
  llvm::DIScope *scope = unit;
  const SourceLocation &location = definition->location();

  const function::Prototype *prototype = definition->prototype();
  llvm::DISubprogram *subprogram = debug_info_builder_.createFunction(
      scope, name, llvm::StringRef(), unit, location.line,
      create_function_type(prototype->args().size()), location.line,
      llvm::DINode::FlagPrototyped, llvm::DISubprogram::SPFlagDefinition);
  fn->setSubprogram(subprogram);
  lexical_blocks_.push_back(subprogram);
}

void DebugInfo::pop_subprogram() { lexical_blocks_.pop_back(); }

llvm::DISubroutineType *DebugInfo::create_function_type(size_t args) {
  llvm::SmallVector<llvm::Metadata *, 8> type_signature;

  // Add the result type.
  type_signature.push_back(type_);

  for (size_t i = 0; i < args; ++i) {
    type_signature.push_back(type_);
  }
  auto type_array = debug_info_builder_.getOrCreateTypeArray(type_signature);
  return debug_info_builder_.createSubroutineType(type_array);
}

DebugInfo &CodegenContext::debug_info() { return debug_info_; }
