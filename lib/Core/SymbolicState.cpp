
#include "SymbolicState.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif

using namespace klee;
using namespace llvm;

namespace klee {

  SymbolicFrame::SymbolicFrame(Function *function) :
      function(function) {}

  SymbolicFrame::~SymbolicFrame() {
    local_ptr.clear();
  }

  Function *SymbolicFrame::getFunction() {
    return function;
  }

  SymbolicState::SymbolicState() {}

  SymbolicState::~SymbolicState() {
    /// Delete all frames
    while (!stack_frame.empty()) {
	SymbolicFrame *frame = stack_frame.top();
	stack_frame.pop();
	delete frame;
    }
  }

  void SymbolicState::push_frame(Function *function) {
    stack_frame.push(new SymbolicFrame(function));
  }

  Function *SymbolicState::pop_frame() {
    Function *ret = NULL;
    if (!stack_frame.empty()) {
	ret = stack_frame.top()->getFunction();
    }
    return ret;
  }

  void SymbolicState::add_local(Value *ptr) {
    stack_frame.top()->add_local(ptr);
  }
}
