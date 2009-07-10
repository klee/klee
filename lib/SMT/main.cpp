#include "SMTParser.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv) {
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " <smt-filename>\n";
    return 1;
  }
  
  klee::expr::SMTParser smtParser(argv[1]);

  smtParser.Init();
  
  cout << smtParser.query << "\n"; 
}
