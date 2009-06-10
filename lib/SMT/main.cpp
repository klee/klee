#include "parser_temp.h"
#include "parser.h"

#include <iostream>

using namespace std;
using namespace klee;
using namespace klee::expr;


int main(int argc, char** argv) {
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " <smt-filename>\n";
    return 1;
  }

  CVC3::Parser* parser = new CVC3::Parser(false, argv[1]);
  while (!parser->done()) {
    ExprHandle e = parser->next();
    if (!e.isNull())
      cout << "e: " << e << "\n";
  }
}
