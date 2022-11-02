#pragma once
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <map>
#include <memory>

class LLVMConnector {

public:
  LLVMConnector(const std::string name)
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

  void erase(const std::string &name) { named_values_.erase(name); }

  void set(const std::string &name, llvm::Value *value) {
    named_values_[name] = value;
  }

  void clear() { named_values_.clear(); }

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
  std::map<std::string, llvm::Value *> named_values_;
};
