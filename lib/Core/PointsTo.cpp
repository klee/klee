
#include "PointsTo.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif

using namespace klee;
using namespace llvm;

namespace klee {

  PointsToFrame::PointsToFrame(Function *function) :
      function(function) {}

  PointsToFrame::~PointsToFrame() {
    local_ptr.clear();
  }

  Function *PointsToFrame::getFunction() {
    return function;
  }

  PointsToState::PointsToState() {}

  PointsToState::~PointsToState() {
    /// Delete all frames
    while (!stack_frame.empty()) {
	PointsToFrame *frame = stack_frame.top();
	stack_frame.pop();
	delete frame;
    }
  }

  void PointsToState::push_frame(Function *function) {
    stack_frame.push(new PointsToFrame(function));
  }

  Function *PointsToState::pop_frame() {
    Function *ret = NULL;
    if (!stack_frame.empty()) {
	ret = stack_frame.top()->getFunction();
	stack_frame.pop();
    }
    return ret;
  }

  void PointsToState::add_local(Value *ptr) {
    stack_frame.top()->add_local(ptr);
  }
}
