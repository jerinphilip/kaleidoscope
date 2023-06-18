#pragma once
#include <map>
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

class DebugInfo {
 public:
  DebugInfo(const std::string &name, llvm::Module &module);
  llvm::DICompileUnit *compile_unit();
  llvm::DIType *type();
  llvm::DIBuilder &debug_info_builder();

 private:
  llvm::DICompileUnit *compile_unit_;
  llvm::DIType *type_;
  llvm::DIBuilder debug_info_builder_;
};

class CodegenContext {
 public:
  explicit CodegenContext(const std::string &name);

  llvm::LLVMContext &context();
  llvm::Module &module();
  llvm::IRBuilder<> &builder();

  // Used to handle instructions for named values.

  /// create_entry_block_alloca - Create an alloca instruction in the entry
  /// block of the function.  This is used for mutable variables etc.
  llvm::AllocaInst *create_entry_block_alloca(llvm::Function *fn,
                                              const std::string &variable);

  void set(const std::string &name, llvm::AllocaInst *value);
  llvm::AllocaInst *lookup(const std::string &name);
  void erase(const std::string &name);
  void clear();

  llvm::DIBuilder &debug_info_builder();

 private:
  /// Global context for LLVM book-keeping.
  llvm::LLVMContext context_;

  /// Entity housing created LLVM entities (say an LLVM::Value) or something.
  /// Externally we only supply pointers to objects owned by the Module.
  /// In some sense, contains the code we build using C++ constructs to be
  /// generated as LLVM IR later.
  llvm::Module module_;

  /// Convenience class to build LLVM IR objects by means of composition.
  llvm::IRBuilder<> builder_;

  // std::map<std::string, llvm::Value *> named_values_;
  std::map<std::string, llvm::AllocaInst *> named_values_;

  DebugInfo debug_info_;
};
