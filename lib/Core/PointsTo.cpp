
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

  PointsToFrame::~PointsToFrame()     {
    local_allocations.clear();
    local_address_of.clear();
  }

  Function *PointsToFrame::getFunction() {
    return function;
  }

  void PointsToFrame::add_local(Value *cell, Value *location) {
    local_allocations.push_back(location);
    local_address_of[location].push_back(cell);
  }

  void PointsToFrame::replace_local(Value *cell, vector<Value *> location_set) {

    // Remove cell from every value in map.
    for (map<Value *, vector<Value *> >::iterator it = local_address_of.begin(); it != local_address_of.end(); it++) {
	int index = 0;
	vector<Value *> cell_list = it->second;
	for (vector<Value *>::iterator cell_it = cell_list.begin(); cell_it != cell_list.end(); cell_it++) {
	    if (*(cell_it) == cell) {
		cell_list.erase(cell_list.begin() + index);
		break;
	    }
	    index++;
	}
    }

    // Add cell to every location in location_set
    for (vector<Value *>::iterator it0 = location_set.begin(); it0 != location_set.end(); it0++) {
	local_address_of[*it0].push_back(cell);
    }
  }

  vector<Value *> PointsToFrame::pointing_to(Value *cell) {
    vector<Value *> res;
    for (map<Value *, vector<Value *> >::iterator it = local_address_of.begin(); it != local_address_of.end(); it++) {
	vector<Value *> cell_list = it->second;
	for (vector<Value *>::iterator cell_it = cell_list.begin(); cell_it != cell_list.end(); cell_it++) {
	    if ((*cell_it) == cell) {
		res.push_back(it->first);
		break;
	    }
	}
    }
    return res;
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

    global_allocations.clear();
    global_address_of.clear();
  }

  void PointsToState::add_global(Value *cell, Value *location) {
    global_allocations.push_back(location);
    global_address_of[location].push_back(cell);
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

  void PointsToState::add_local(Value *cell, Value *location) {
    points_to_stack.top()->add_local(cell, location);
  }

  void load(Value *cell, Value *location) {
  }

  void store(Value *cell, Value *location) {
  }

  vector<Value *> PointsToState::pointing_to(Value *cell) {
    vector<Value *> res;
    for (map<Value *, vector<Value *> >::iterator it = global_address_of.begin(); it != global_address_of.end(); it++) {
	vector<Value *> cell_list = it->second;
	for (vector<Value *>::iterator cell_it = cell_list.begin(); cell_it != cell_list.end(); cell_it++) {
	    if ((*cell_it) == cell) {
		res.push_back(it->first);
		break;
	    }
	}
    }
    return res;
  }

}
