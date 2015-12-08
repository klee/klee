
#include "BlockTable.h"

using namespace klee;

BlockTable::BlockTable() {}

BlockTable::~BlockTable() {
  tableImpl.clear();
}

void BlockTable::add(Instruction *inst) {
  if (inst->getParent()->getName().empty()) {
      /// We have found a basic block with empty name: We name all basic blocks
      /// in this function according to topological order.
      (inst->getParent()->getParent()->front()).getNextNode();
  }
  tableImpl.insert(inst->getParent()->getName());
}

void BlockTable::dump() {
  /// this->print(llvm::errs());
}

void BlockTable::print(llvm::raw_ostream& stream) {
  stream << "------------------------- Basic Blocks ------------------------------\n";
  for (set< string >::iterator it = tableImpl.begin(); it != tableImpl.end(); it++) {
      stream << (*it) << "\n";
  }
  stream << "---------------------------------------------------------------------\n";
}
