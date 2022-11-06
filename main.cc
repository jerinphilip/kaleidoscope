
#include "kaleidoscope/lexer.h"
#include "kaleidoscope/llvm_connector.h"
#include "kaleidoscope/parser.h"
#include <cstdio>

void main_loop(Lexer &lexer, Parser &parser, LLVMConnector &llvms) {
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

  fprintf(stderr, "> ");
  main_loop(lexer, parser, llvms);
  llvms.module().print(llvm::errs(), nullptr);

  return 0;
}
