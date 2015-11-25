
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

    Value *get_llvm();
  };

  class Location {
    MemoryCell *content;
  public:
    Location();

    Location(MemoryCell *content);

    ~Location();

    void set_content(MemoryCell *content);

    MemoryCell *get_content();
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

    bool is_main_frame();

  };

  class PointsToState {
    stack< PointsToFrame *, vector<PointsToFrame *> > points_to_stack;
    map< MemoryCell *, vector<Location *> > points_to;

  public:
    PointsToState();

    ~PointsToState();

    void push_frame(Function *callee);

    Function *pop_frame();

    void alloc_local(Value *cell);

    void alloc_global(Value *cell);

    void address_of_to_local(Value *target, Value *source);

    void address_of_to_global(Value *target, Value *source);

    void assign_to_local(Value *target, Value *source);

    void assign_to_global(Value *target, Value *source);

    void load_to_local(Value *target, Value *address);

    void load_to_global(Value *target, Value *address);

    void store_from_local(Value *source, Value *address);

    void store_from_global(Value *source, Value *address);

  };


  /// Function declarations
  bool operator==(MemoryCell *lhs, MemoryCell *rhs);

  void store_points_to(map<MemoryCell *, vector<Location *> > points_to, MemoryCell *source, MemoryCell *address);

  void load_points_to(map<MemoryCell *, vector<Location *> > points_to, MemoryCell *target, MemoryCell *address);

}

#endif
