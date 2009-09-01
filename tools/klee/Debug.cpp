#include <klee/Expr.h>
#include <iostream>

void kdb_printExpr(klee::Expr *e) {
  std::cerr << "expr: " << e << " -- ";
  if (e) {
    std::cerr << *e;
  } else {
    std::cerr << "(null)";
  }
  std::cerr << "\n";
}
