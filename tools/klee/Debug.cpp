#include <klee/Expr.h>
#include <iostream>

void kdb_printExpr(klee::Expr *e) {
  llvm::cerr << "expr: " << e << " -- ";
  if (e) {
    llvm::cerr << *e;
  } else {
    llvm::cerr << "(null)";
  }
  llvm::cerr << "\n";
}
