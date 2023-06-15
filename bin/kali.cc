#include "kaleidoscope/lexer.h"
#include "kaleidoscope/llvm_connector.h"
#include "kaleidoscope/parser.h"
#include <cstdio>

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

void main_loop(Lexer &lexer, Parser &parser, LLVMConnector &llvms) {
  fprintf(stderr, "> ");
  Atom symbol = lexer.read();
  while (symbol != Atom::eof) {

    switch (symbol) {
    case Atom::eof: {
      fprintf(stderr, "Parsed EOF\n");
      return;
    } break;

    case Atom::keyword_def: {
      // Handle definition
      // fprintf(stderr, "Attempting to parse def ....\n");
      DefinitionPtr def = parser.definition(lexer);
      if (def) {
        // fprintf(stderr, "Parsed def\n");
        if (auto ir = def->codegen(llvms)) {
          ir->print(llvm::errs());
          fprintf(stderr, "\n");
        }
      } else {
        // fprintf(stderr, "Failed to parse...\n");
        lexer.read();
      }
    } break;

    case Atom::keyword_extern: {
      // Handle extern
      PrototypePtr expr = parser.extern_(lexer);
      if (expr) {
        // fprintf(stderr, "Parsed extern\n");
        if (auto ir = expr->codegen(llvms)) {
          ir->print(llvm::errs());
          fprintf(stderr, "\n");
        }
      } else {
        lexer.read();
      }
    } break;

    case Atom::comment: {
      // fprintf(stderr, "Parsed comment\n");
    } break;

    case Atom::unknown: {
      // fprintf(stderr, "Parsed unknown\n");
    } break;

    case Atom::semicolon: {
      lexer.read();
    } break;

    default: {
      DefinitionPtr expr = parser.top(lexer);
      if (expr) {
        // fprintf(stderr, "Parsed top-level expression till %c\n",
        //         lexer.current());
        if (auto ir = expr->codegen(llvms)) {
          ir->print(llvm::errs());
          fprintf(stderr, "\n");

          // Remove anonymous expression
          ir->eraseFromParent();
        }
      } else {
        lexer.read();
      }
    } break;
    }

    fprintf(stderr, "> ");
    symbol = lexer.read();
  }
}

int main() {
  Lexer lexer;
  Parser parser;
  LLVMConnector llvms("kaleidoscope");
  auto &module = llvms.module();

  main_loop(lexer, parser, llvms);
  module.print(llvm::errs(), nullptr);

  // Initialize the target registry etc.
  using namespace llvm;
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();

  auto target_triple = sys::getDefaultTargetTriple();
  module.setTargetTriple(target_triple);

  std::string Error;
  auto target = TargetRegistry::lookupTarget(target_triple, Error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!target) {
    errs() << Error;
    return 1;
  }

  auto cpu = "generic";
  auto features = "";

  TargetOptions target_options;
  auto relocation_model = Optional<Reloc::Model>();
  auto target_machine = target->createTargetMachine(
      target_triple, cpu, features, target_options, relocation_model);

  auto data_layout = target_machine->createDataLayout();
  module.setDataLayout(data_layout);

  auto filename = "output.o";
  std::error_code error_code;
  raw_fd_ostream dest(filename, error_code, sys::fs::OF_None);

  if (error_code) {
    errs() << "Could not open file: " << error_code.message();
    return 1;
  }

  legacy::PassManager pass;
  auto filetype = CGFT_ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, filetype)) {
    errs() << "target_machine can't emit a file of this type";
    return 1;
  }

  pass.run(module);
  dest.flush();

  outs() << "Wrote " << filename << "\n";

  return 0;
}
