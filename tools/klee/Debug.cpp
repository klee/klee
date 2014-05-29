#include <klee/Expr.h>

void kdb_printExpr(klee::Expr *e) {
  llvm::errs() << "expr: " << e << " -- ";
  if (e) {
    llvm::errs() << *e;
  } else {
    llvm::errs() << "(null)";
  }
  llvm::errs() << "\n";
}
