
#ifndef KLEE_SYMBOLICSTATE_H
#define KLEE_SYMBOLICSTATE_H

#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif

#include <vector>
#include <stack>

using namespace std;
using namespace llvm;

namespace llvm {
  class Function;
  class Value;
}

namespace klee {

  class MemoryCell {
    Value *llvm_value;
  public:
    MemoryCell(Value *cell) : llvm_value(cell) {}
    ~MemoryCell();
    Value *get_llvm() {
      return llvm_value;
    }
  };

  class Location {
    Value *llvm_value;
    MemoryCell *content;
  public:
    Location(Value *location);

    Location(Value *location, MemoryCell *content);

    Location(MemoryCell *content);

    ~Location();

    void set_content(MemoryCell *content);

    MemoryCell *get_content() {
      return content;
    }

    Value *get_llvm();
  };

  class PointsToFrame {
    Function *function;
    map<MemoryCell *, vector<Location *> > points_to;

  public:
    PointsToFrame(Function *function);

    ~PointsToFrame();

    Function *getFunction();

    void alloc_local(MemoryCell *cell, Location *location);

    void address_of_to_local(MemoryCell *target, MemoryCell *source);

    void assign_to_local(MemoryCell *target, MemoryCell *source);

    void load_to_local(MemoryCell *target, MemoryCell *address);

    void store_from_local(MemoryCell *source, MemoryCell *address);

  };

  class PointsToState {
    stack< PointsToFrame *, vector<PointsToFrame *> > points_to_stack;
    map<MemoryCell *, vector<Location *> > points_to;

  public:
    PointsToState();

    ~PointsToState();

    void push_frame(Function *callee);

    Function *pop_frame();

    void alloc_local(MemoryCell *cell, Location *location);

    void alloc_global(MemoryCell *cell, Location *location);

    void assign_to_local(MemoryCell *target, MemoryCell *source);

    void assign_to_global(MemoryCell *target, MemoryCell *source);

    void load_to_local(MemoryCell *target, MemoryCell *address);

    void load_to_global(MemoryCell *target, MemoryCell *address);

    void store_from_local(MemoryCell *source, MemoryCell *address);

    void store_from_global(MemoryCell *source, MemoryCell *address);

  };
}

#endif
