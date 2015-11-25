
#include "PointsTo.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif
#include "llvm/Support/raw_ostream.h"


using namespace klee;
using namespace llvm;

namespace klee {

  MemoryCell::MemoryCell(Value *cell) :
      llvm_value(cell) {}

  Value *MemoryCell::get_llvm() {
    return llvm_value;
  }

  Location::Location() :
    content(0) {}

  Location::Location(MemoryCell *cell) :
    content(cell) {}

  Location::~Location() {}

  void Location::set_content(MemoryCell *cell) {
    content = cell;
  }

  MemoryCell *Location::get_content() {
    return content;
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
    load_points_to(points_to, target, address);
  }

  void PointsToFrame::store_from_local(MemoryCell *source, MemoryCell *address) {
    store_points_to(points_to, source, address);
  }

  bool PointsToFrame::is_main_frame() {
    return function == 0;
  }

  /**/

  PointsToState::PointsToState() {
    /// Here we insert a frame with null function indicating "main"
    points_to_stack.push(new PointsToFrame(0));
  }

  PointsToState::~PointsToState() {
    /// Delete all frames
    while (!points_to_stack.empty()) {
	PointsToFrame *frame = points_to_stack.top();
	points_to_stack.pop();
	delete frame;
    }
    points_to.clear();
  }

  void PointsToState::alloc_local(Value *cell) {
    points_to_stack.top()->alloc_local(new MemoryCell(cell), new Location());
  }

  void PointsToState::alloc_global(Value *cell) {
    points_to[new MemoryCell(cell)].push_back(new Location());
  }

  void PointsToState::assign_to_local(Value *target, Value *source) {
    points_to_stack.top()->assign_to_local(new MemoryCell(target), new MemoryCell(source));
  }

  void PointsToState::assign_to_global(Value *target, Value *source) {
    MemoryCell *target_cell = new MemoryCell(target);
    points_to[target_cell].clear();
    points_to[target_cell] = points_to[new MemoryCell(source)];
  }

  void PointsToState::address_of_to_local(Value *target, Value *source) {
    points_to_stack.top()->address_of_to_local(new MemoryCell(target), new MemoryCell(source));
  }

  void PointsToState::address_of_to_global(Value *target, Value *source) {
    MemoryCell *target_cell = new MemoryCell(target);
    points_to[target_cell].clear();
    points_to[target_cell].push_back(new Location(new MemoryCell(source)));
  }

  void PointsToState::load_to_local(Value *target, Value *address) {
    points_to_stack.top()->load_to_local(new MemoryCell(target), new MemoryCell(address));
  }

  void PointsToState::load_to_global(Value *target, Value *address) {
    load_points_to(points_to, new MemoryCell(target), new MemoryCell(address));
  }

  void PointsToState::store_from_local(Value *source, Value *address) {
    points_to_stack.top()->store_from_local(new MemoryCell(source), new MemoryCell(address));
  }

  void PointsToState::store_from_global(Value *source, Value *address) {
    store_points_to(points_to, new MemoryCell(source), new MemoryCell(address));
  }

  void PointsToState::push_frame(Function *function) {
    llvm::errs() << "PUSH FRAME " << function->getName() << "\n";
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

  /**/

  bool operator==(MemoryCell *lhs, MemoryCell *rhs) {
    return lhs->get_llvm() == rhs->get_llvm();
  }

  void load_points_to(map<MemoryCell *, vector<Location *> > points_to, MemoryCell *target, MemoryCell *address) {
    for (vector<Location *>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	MemoryCell *content = (*pointed_to_location)->get_content();
	if (content != 0) {
	    vector<Location *> pointed_to_by_target = points_to[target];
	    vector<Location *> pointed_to_by_content = points_to[content];
	    pointed_to_by_target.insert(pointed_to_by_target.end(), pointed_to_by_content.begin(), pointed_to_by_content.end());
	}
    }
  }

  void store_points_to(map<MemoryCell *, vector<Location *> > points_to, MemoryCell *source, MemoryCell *address) {
    for (vector<Location *>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	MemoryCell *content = (*pointed_to_location)->get_content();
	if (content != 0) {
	    vector<Location *> pointed_to_by_source = points_to[source];
	    vector<Location *> pointed_to_by_content = points_to[content];
	    pointed_to_by_content.insert(pointed_to_by_content.end(), pointed_to_by_source.begin(), pointed_to_by_source.end());
	}
    }
  }

}
