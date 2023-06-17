#include "codegen_context.h"

DebugInfo::DebugInfo(const std::string &name, llvm::Module &module)
    : debug_info_builder_(module) {
  compile_unit_ = debug_info_builder_.createCompileUnit(
      llvm::dwarf::DW_LANG_C, debug_info_builder_.createFile(name, "."), "kali",
      false, "", 0);
  type_ = debug_info_builder_.createBasicType("double", 64,
                                              llvm::dwarf::DW_ATE_float);
}

llvm::DIType *DebugInfo::type() { return type_; }

CodegenContext::CodegenContext(const std::string &name)
    : module_(name, context_), builder_(context_) {}

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

llvm::AllocaInst *CodegenContext::create_entry_block_alloca(
    llvm::Function *fn, const std::string &variable) {
  llvm::IRBuilder<> temp_builder(&fn->getEntryBlock(),
                                 fn->getEntryBlock().begin());
  return temp_builder.CreateAlloca(llvm::Type::getDoubleTy(context_), nullptr,
                                   variable);
}
