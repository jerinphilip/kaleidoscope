#include <cstdio>

#include "kaleidoscope/codegen_context.h"
#include "kaleidoscope/lexer.h"
#include "kaleidoscope/parser.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

void repl(const std::string &source, CodegenContext &codegen_context) {
  Lexer lexer(source);
  Parser parser;
  Atom symbol = lexer.read();
  while (symbol != Atom::kEof) {
    switch (symbol) {
      case Atom::kEof: {
        return;
      } break;

      case Atom::kKeywordDef: {
        // Handle definition
        DefinitionPtr def = parser.definition(lexer);
        if (def) {
          if (auto *ir = def->codegen(codegen_context)) {
            // ir->print(llvm::errs());
          }
        } else {
          lexer.read();
        }
      } break;

      case Atom::kKeywordExtern: {
        // Handle extern
        PrototypePtr expr = Parser::extern_(lexer);
        if (expr) {
          if (auto *ir = expr->codegen(codegen_context)) {
            // ir->print(llvm::errs());
          }
        } else {
          lexer.read();
        }
      } break;

      case Atom::kComment: {
      } break;

      case Atom::kUnknown: {
      } break;

      case Atom::kSemicolon: {
        lexer.read();
      } break;

      default: {
        DefinitionPtr expr = parser.top(lexer);
        if (expr) {
          if (auto *ir = expr->codegen(codegen_context)) {
            // ir->print(llvm::errs());

            // Remove anonymous expression
            ir->eraseFromParent();
          }
        } else {
          lexer.read();
        }
      } break;
    }

    symbol = lexer.read();
  }
}

int main(int argc, char **argv) {
  (void)argc;
  CodegenContext codegen_context("kaleidoscope");

  std::string source(argv[1]);

  repl(source, codegen_context);

  llvm::Module &module = codegen_context.module();
  module.print(llvm::errs(), nullptr);

  // Initialize the target registry etc.
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  std::string target_triple = llvm::sys::getDefaultTargetTriple();
  module.setTargetTriple(target_triple);

  std::string error;
  const auto *target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!target) {
    llvm::errs() << error;
    return 1;
  }

  std::string cpu = "generic";
  std::string features;

  llvm::TargetOptions target_options;
  llvm::Optional<llvm::Reloc::Model> relocation_model;
  llvm::TargetMachine *target_machine = target->createTargetMachine(
      target_triple, cpu, features, target_options, relocation_model);

  llvm::DataLayout data_layout = target_machine->createDataLayout();
  module.setDataLayout(data_layout);

  std::string filename = "output.o";
  std::error_code error_code;
  llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);

  if (error_code) {
    llvm::errs() << "Could not open file: " << error_code.message();
    return 1;
  }

  llvm::legacy::PassManager pass;
  llvm::CodeGenFileType filetype = llvm::CGFT_ObjectFile;

  // Promote allocas to registers.
  pass.add(llvm::createPromoteMemoryToRegisterPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  pass.add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  pass.add(llvm::createReassociatePass());
  // Eliminate Common SubExpressions.
  pass.add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  pass.add(llvm::createCFGSimplificationPass());

  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, filetype)) {
    llvm::errs() << "target_machine can't emit a file of this type";
    return 1;
  }

  pass.run(module);
  dest.flush();

  return 0;
}
