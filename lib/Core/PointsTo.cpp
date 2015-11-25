
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

  MemoryCell::MemoryCell() :
      llvm_value(0) {}

  MemoryCell::MemoryCell(Value *cell) :
      llvm_value(cell) {}

  Value *MemoryCell::get_llvm() {
    return llvm_value;
  }

  bool MemoryCell::operator==(MemoryCell rhs) {
    return llvm_value == rhs.get_llvm();
  }

  Location::Location(unsigned long alloc_id) :
      alloc_id(alloc_id) {}

  Location::Location(const MemoryCell& cell) :
      content(cell), alloc_id(0) {}

  Location::~Location() {}

  void Location::set_content(const MemoryCell& cell) {
    content = cell;
  }

  MemoryCell Location::get_content() {
    return content;
  }

  bool Location::operator==(Location rhs) {
    if (alloc_id > 0 && rhs.alloc_id > 0) {
	return alloc_id == rhs.alloc_id;
    } else if (alloc_id == 0 && rhs.alloc_id == 0) {
	return content == rhs.get_content();
    }
    return false;
  }

  PointsToFrame::PointsToFrame(Function *function) :
    function(function) {}

  PointsToFrame::~PointsToFrame()     {
    points_to.clear();
  }

  Function *PointsToFrame::getFunction() {
    return function;
  }

  void PointsToFrame::alloc_local(const MemoryCell& cell, const Location& location) {
    points_to[cell].push_back(location);
  }

  void PointsToFrame::address_of_to_local(const MemoryCell& target, const MemoryCell& source) {
    Location loc(source);
    points_to[target].clear();
    points_to[target].push_back(loc);
  }

  void PointsToFrame::assign_to_local(const MemoryCell& target, const MemoryCell& source) {
    points_to[target].clear();
    points_to[target] = points_to[source];
  }

  void PointsToFrame::load_to_local(const MemoryCell& target, const MemoryCell& address) {
    load_points_to(points_to, target, address);
  }

  void PointsToFrame::store_from_local(const MemoryCell& source, const MemoryCell& address) {
    store_points_to(points_to, source, address);
  }

  bool PointsToFrame::is_main_frame() {
    return function == 0;
  }

  /**/

  PointsToState::PointsToState() : next_alloc_id(1) {
    /// Here we insert a frame with null function indicating "main"
    PointsToFrame new_frame(0);
    points_to_stack.push(new_frame);
  }

  PointsToState::~PointsToState() {
    points_to.clear();
  }

  void PointsToState::alloc_local(Value *cell) {
    MemoryCell mem_cell(cell);
    Location loc(next_alloc_id++);
    points_to_stack.top().alloc_local(mem_cell, loc);
  }

  void PointsToState::alloc_global(Value *cell) {
    MemoryCell mem_cell(cell);
    Location loc(next_alloc_id++);
    points_to[mem_cell].push_back(loc);
  }

  void PointsToState::assign_to_local(Value *target, Value *source) {
    MemoryCell target_cell(target), source_cell(source);
    points_to_stack.top().assign_to_local(target_cell, source_cell);
  }

  void PointsToState::assign_to_global(Value *target, Value *source) {
    MemoryCell target_cell(target), source_cell(source);
    points_to[target_cell].clear();
    points_to[target_cell] = points_to[source_cell];
  }

  void PointsToState::address_of_to_local(Value *target, Value *source) {
    MemoryCell target_cell(target), source_cell(source);
    points_to_stack.top().address_of_to_local(target_cell, source_cell);
  }

  void PointsToState::address_of_to_global(Value *target, Value *source) {
    MemoryCell target_cell(target), source_cell(source);
    Location source_loc(source_cell);
    points_to[target_cell].clear();
    points_to[target_cell].push_back(source_loc);
  }

  void PointsToState::load_to_local(Value *target, Value *address) {
    MemoryCell target_cell(target), address_cell(address);
    points_to_stack.top().load_to_local(target_cell, address_cell);
  }

  void PointsToState::load_to_global(Value *target, Value *address) {
    MemoryCell target_cell(target), address_cell(address);
    load_points_to(points_to, target_cell, address_cell);
  }

  void PointsToState::store_from_local(Value *source, Value *address) {
    MemoryCell source_cell(source), address_cell(address);
    points_to_stack.top().store_from_local(source_cell, address_cell);
  }

  void PointsToState::store_from_global(Value *source, Value *address) {
    MemoryCell source_cell(source), address_cell(address);
    store_points_to(points_to, source_cell, address_cell);
  }

  void PointsToState::push_frame(Function *function) {
    PointsToFrame points_to_frame(function);
    llvm::errs() << "PUSH FRAME " << function->getName() << "\n";
    points_to_stack.push(points_to_frame);
  }

  Function *PointsToState::pop_frame() {
    Function *ret = NULL;
    if (!points_to_stack.empty()) {
	ret = points_to_stack.top().getFunction();
	points_to_stack.pop();
    }
    return ret;
  }

  /**/

  void load_points_to(map<MemoryCell, vector<Location> >& points_to, const MemoryCell& target, const MemoryCell& address) {
    for (vector<Location>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	MemoryCell content = pointed_to_location->get_content();
	if (content != 0) {
	    vector<Location> pointed_to_by_target = points_to[target];
	    vector<Location> pointed_to_by_content = points_to[content];
	    pointed_to_by_target.insert(pointed_to_by_target.end(), pointed_to_by_content.begin(), pointed_to_by_content.end());
	}
    }
  }

  void store_points_to(map<MemoryCell, vector<Location> >& points_to, const MemoryCell& source, const MemoryCell& address) {
    for (vector<Location>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	MemoryCell content = pointed_to_location->get_content();
	if (content != 0) {
	    vector<Location> pointed_to_by_source = points_to[source];
	    vector<Location> pointed_to_by_content = points_to[content];
	    pointed_to_by_content.insert(pointed_to_by_content.end(), pointed_to_by_source.begin(), pointed_to_by_source.end());
	}
    }
  }

}
