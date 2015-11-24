
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

  class PointsToFrame {
    Function *function;
    vector<Value *> local_allocations;
    map<Value *, vector<Value *> > local_address_of;

  public:
    PointsToFrame(Function *function);

    ~PointsToFrame();

    Function *getFunction();

    void add_local(Value *cell, Value *location);

    void replace_local(Value *cell, vector<Value *> location_set);

    vector<Value *> pointing_to(Value *cell);
  };

  class PointsToState {
    stack< PointsToFrame *, vector<PointsToFrame *> > points_to_stack;
    vector<Value *> global_allocations;
    map<Value *, vector<Value *> > global_address_of;

    void add_global(Value *cell, Value *location);

  public:
    PointsToState();

    ~PointsToState();

    void push_frame(Function *callee);

    Function *pop_frame();

    void add_global(Value *cell, Value *location);

    void add_local(Value *cell, Value *location);

    void load(Value *cell, Value *location);

    void store(Value *cell, Value *location);

    vector<Value *> pointing_to(Value *cell);
  };
}

#endif
