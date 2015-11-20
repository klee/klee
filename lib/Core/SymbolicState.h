
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

  class SymbolicFrame {
    Function *function;
    vector<Value *> local_ptr;

  public:
    SymbolicFrame(Function *function);

    ~SymbolicFrame();

    Function *getFunction();

    void add_local(Value *ptr) {
      local_ptr.push_back(ptr);
    }
  };

  class SymbolicState {
    stack< SymbolicFrame *, vector<SymbolicFrame *> > stack_frame;
  public:
    SymbolicState();

    ~SymbolicState();

    void push_frame(Function *callee);

    Function *pop_frame();

    void add_local(Value *ptr);
  };
}

#endif
