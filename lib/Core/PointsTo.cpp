
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

  Location::Location(Value *location) :
    llvm_value(location), content(0) {}

  Location::Location(Value *location, MemoryCell *content) :
    llvm_value(location), content(content) {}

  Location::~Location() {}

  void Location::set_content(MemoryCell *cell) {
    content = cell;
  }

  Value *Location::get_llvm() {
    return llvm_value;
  }

  PointsToFrame::PointsToFrame(Function *function) :
      function(function) {}

  PointsToFrame::~PointsToFrame()     {
    points_to.clear();
  }

  Function *PointsToFrame::getFunction() {
    return function;
  }

  void PointsToFrame::alloc_local(MemoryCell *cell, Location *location) {
    points_to[cell].push_back(location);
  }

  void PointsToFrame::address_of_to_local(MemoryCell *target, MemoryCell *source) {
    points_to[target].clear();
    points_to[target].push_back(new Location(source));
  }

  void PointsToFrame::assign_to_local(MemoryCell *target, MemoryCell *source) {
    points_to[target].clear();
    points_to[target] = points_to[source];
  }

  void PointsToFrame::load_to_local(MemoryCell *target, MemoryCell *address) {
    for (vector<Location *>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	Cell *content = (*pointed_to_location)->get_content();
	if (content != 0) {
	    vector<Location *> pointed_to_by_target = points_to[target];
	    vector<Location *> pointed_to_by_content = points_to[content];
	    pointed_to_by_target.insert(pointed_to_by_target.end(), pointed_to_by_content.begin(), pointed_to_by_content.end());
	}
    }
  }

  void PointsToFrame::store_from_local(MemoryCell *source, MemoryCell *address) {
    for (vector<Location *>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	Cell *content = (*pointed_to_location)->get_content();
	if (content != 0) {
	    vector<Location *> pointed_to_by_source = points_to[source];
	    vector<Location *> pointed_to_by_content = points_to[content];
	    pointed_to_by_content.insert(pointed_to_by_content.end(), pointed_to_by_source.begin(), pointed_to_by_source.end());
	}
    }
  }

  /**/

  PointsToState::PointsToState() {}

  PointsToState::~PointsToState() {
    /// Delete all frames
    while (!points_to_stack.empty()) {
	PointsToFrame *frame = points_to_stack.top();
	points_to_stack.pop();
	delete frame;
    }
    points_to.clear();
  }

  void PointsToState::alloc_local(MemoryCell *cell, Location *location) {
    points_to_stack.top()->alloc_local(cell, location);
  }

  void PointsToState::alloc_global(MemoryCell *cell, Location *location) {
    points_to[cell].push_back(location);
  }

  void PointsToState::assign_to_local(MemoryCell *target, MemoryCell *source) {
    points_to_stack.top()->assign_to_local(target, source);
  }

  void PointsToState::assign_to_global(MemoryCell *target, MemoryCell *source) {
    points_to[target].clear();
    points_to[target] = points_to[source];
  }

  void PointsToState::load_to_local(MemoryCell *target, MemoryCell *address) {
    points_to_stack.top()->load_to_local(target, address);
  }

  void PointsToState::load_to_global(MemoryCell *target, MemoryCell *address) {
    for (vector<Location *>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	Cell *content = (*pointed_to_location)->get_content();
	if (content != 0) {
	    vector<Location *> pointed_to_by_target = points_to[target];
	    vector<Location *> pointed_to_by_content = points_to[content];
	    pointed_to_by_target.insert(pointed_to_by_target.end(), pointed_to_by_content.begin(), pointed_to_by_content.end());
	}
    }
  }

  void PointsToState::store_from_local(MemoryCell *source, MemoryCell *address) {
    points_to_stack.top()->store_from_local(source, address);
  }

  void PointsToState::store_from_global(MemoryCell *source, MemoryCell *address) {
    for (vector<Location *>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	Cell *content = (*pointed_to_location)->get_content();
	if (content != 0) {
	    vector<Location *> pointed_to_by_source = points_to[source];
	    vector<Location *> pointed_to_by_content = points_to[content];
	    pointed_to_by_content.insert(pointed_to_by_content.end(), pointed_to_by_source.begin(), pointed_to_by_source.end());
	}
    }
  }

  void PointsToState::push_frame(Function *function) {
    points_to_stack.push(new PointsToFrame(function));
  }

  Function *PointsToState::pop_frame() {
    Function *ret = NULL;
    if (!points_to_stack.empty()) {
	ret = points_to_stack.top()->getFunction();
	points_to_stack.pop();
    }
    return ret;
  }

}
