#include "expr/Lexer.h"
#include "expr/Parser.h"

#include "klee/Expr.h"
#include "klee/Solver.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/util/ExprVisitor.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Streams.h"
#include "llvm/System/Signals.h"

#include <iomanip>
#include <sstream>

using namespace llvm;
using namespace klee;
using namespace klee::expr;

namespace {
  llvm::cl::opt<std::string>
  InputFile(llvm::cl::desc("<input query log>"), llvm::cl::Positional,
            llvm::cl::init("-"));

  enum ToolActions {
    PrintTokens,
    PrintAST
  };

  static llvm::cl::opt<ToolActions> 
  ToolAction(llvm::cl::desc("Tool actions:"),
             llvm::cl::init(PrintTokens),
             llvm::cl::values(
             clEnumValN(PrintTokens, "print-tokens",
                        "Print tokens from the input file."),
             clEnumValN(PrintAST, "print-ast",
                        "Print parsed AST nodes from the input file."),
             clEnumValEnd));
}

static std::string escapedString(const char *start, unsigned length) {
  std::stringstream s;
  for (unsigned i=0; i<length; ++i) {
    char c = start[i];
    if (isprint(c)) {
      s << c;
    } else if (c == '\n') {
      s << "\\n";
    } else {
      s << "\\x" << hexdigit(((unsigned char) c >> 4) & 0xF) << hexdigit((unsigned char) c & 0xF);
    }
  }
  return s.str();
}

static void PrintInputTokens(const MemoryBuffer *MB) {
  Lexer L(MB);
  Token T;
  do {
    L.Lex(T);
    llvm::cout << "(Token \"" << T.getKindName() << "\" "
               << "\"" << escapedString(T.start, T.length) << "\" "
               << T.length << " "
               << T.line << " " << T.column << ")\n";
  } while (T.kind != Token::EndOfFile);
}

static bool PrintInputAST(const char *Filename,
                          const MemoryBuffer *MB) {
  Parser *P = Parser::Create(Filename, MB);
  P->SetMaxErrors(20);
  while (Decl *D = P->ParseTopLevelDecl()) {
    if (!P->GetNumErrors())
      D->dump();
    delete D;
  }

  bool success = true;
  if (unsigned N = P->GetNumErrors()) {
    llvm::cerr << Filename << ": parse failure: "
               << N << " errors.\n";
    success = false;
  }
  delete P;

  return success;
}

int main(int argc, char **argv) {
  bool success = true;

  llvm::sys::PrintStackTraceOnErrorSignal();
  llvm::cl::ParseCommandLineOptions(argc, argv);

  std::string ErrorStr;
  MemoryBuffer *MB = MemoryBuffer::getFileOrSTDIN(InputFile.c_str(), &ErrorStr);
  if (!MB) {
    llvm::cerr << argv[0] << ": ERROR: " << ErrorStr << "\n";
    return 1;
  }

  switch (ToolAction) {
  case PrintTokens:
    PrintInputTokens(MB);
    break;
  case PrintAST:
    success = PrintInputAST(InputFile=="-" ? "<stdin>" : InputFile.c_str(), MB);
    break;
  default:
    llvm::cerr << argv[0] << ": ERROR: Unknown program action!\n";
  }

  delete MB;

  llvm::llvm_shutdown();
  return success ? 0 : 1;
}
