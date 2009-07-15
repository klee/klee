#include "SMTParser.h"

#include "klee/ExprBuilder.h"

#include <iostream>

using namespace std;
using namespace klee;

int main(int argc, char** argv) {
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " <smt-filename>\n";
    return 1;
  }
  
  enum BuilderKinds {
    DefaultBuilder,
    ConstantFoldingBuilder,
    SimplifyingBuilder
  } BuilderKind = SimplifyingBuilder;

  ExprBuilder *Builder = 0;
  switch (BuilderKind) {
  case DefaultBuilder:
    Builder = createDefaultExprBuilder();
    break;
  case ConstantFoldingBuilder:
    Builder = createDefaultExprBuilder();
    Builder = createConstantFoldingExprBuilder(Builder);
    break;
  case SimplifyingBuilder:
    Builder = createDefaultExprBuilder();
    Builder = createConstantFoldingExprBuilder(Builder);
    Builder = createSimplifyingExprBuilder(Builder);
    break;
  }
  
  klee::expr::SMTParser smtParser(argv[1], Builder);
  smtParser.Parse();
  int result = smtParser.Solve();
  
  cout << (result ? "UNSAT":"SAT") << "\n"; 
}
