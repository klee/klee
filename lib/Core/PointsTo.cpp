
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

  MemCell::MemCell() :
      llvm_value(0) {}

  MemCell::MemCell(Value *cell) :
      llvm_value(cell) {}

  MemCell::~MemCell() {}

  Value *MemCell::get_llvm() const {
    return llvm_value;
  }

  void MemCell::print(llvm::raw_ostream& stream) const {
    stream << "Cell[";
    if (llvm_value) {
	llvm_value->print(stream);
    } else {
	stream << "NULL";
    }
    stream << "]";
  }

  bool operator==(const MemCell& lhs, const MemCell &rhs) {
    /// FIXME Attempt to retrieve llvm_value from lhs segfaults
    return lhs.llvm_value == rhs.llvm_value;
  }

  bool operator<(const MemCell& lhs, const MemCell& rhs) {
    return !(lhs == rhs);
  }

  Location::Location(unsigned long alloc_id) :
      alloc_id(alloc_id) {}

  Location::Location(const MemCell& cell) :
      content(cell), alloc_id(0) {}

  Location::~Location() {}

  void Location::set_content(const MemCell& cell) {
    content = cell;
  }

  MemCell Location::get_content() {
    return content;
  }

  void Location::print(llvm::raw_ostream& stream) const {
    stream << "Location[";
    content.print(stream);
    stream << "]";
  }

  bool operator==(Location& lhs, Location& rhs) {
    if (lhs.alloc_id > 0 && rhs.alloc_id > 0) {
	return lhs.alloc_id == rhs.alloc_id;
    } else if (lhs.alloc_id == 0 && rhs.alloc_id == 0) {
	return lhs.content == rhs.get_content();
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

  void PointsToFrame::alloc_local(const MemCell& cell, const Location& location) {
    points_to[cell].push_back(location);
  }

  void PointsToFrame::address_of_to_local(const MemCell& target, const MemCell& source) {
    Location loc(source);
    points_to[target].clear();
    points_to[target].push_back(loc);
  }

  void PointsToFrame::assign_to_local(const MemCell& target, const MemCell& source) {
    points_to[target].clear();
    points_to[target] = points_to[source];
  }

  void PointsToFrame::load_to_local(const MemCell& target, const MemCell& address) {
    load_points_to(points_to, target, address);
  }

  void PointsToFrame::store_from_local(const MemCell& source, const MemCell& address) {
    store_points_to(points_to, source, address);
  }

  bool PointsToFrame::is_main_frame() {
    return function == 0;
  }

  void PointsToFrame::print(llvm::raw_ostream& stream) const {
    unsigned i, j;
    stream << "Frame[" << function->getName() << ":";
    i = 0;
    for (map< MemCell, vector<Location> >::const_iterator it0 = points_to.begin();
	it0 != points_to.end(); it0++) {
      it0->first.print(stream);
      stream << "->[";
      j = 0;
      for (vector<Location>::const_iterator it1 = it0->second.begin(); it1 != it0->second.end(); it1++) {
	it1->print(stream);
	if (j++ < it0->second.size() - 1) {
	  stream << ",";
	}
      }
      stream << "]";
      if (i++ < points_to.size() - 1) {
	  stream << ",";
      }
    }
    stream << "]";
  }

  /**/

  PointsToState::PointsToState() : next_alloc_id(1) {
    /// Here we insert a frame with null function indicating "main"
    PointsToFrame new_frame(0);
    points_to_stack.push_back(new_frame);
  }

  PointsToState::~PointsToState() {
    points_to.clear();
  }

  void PointsToState::alloc_local(Value *cell) {
    MemCell mem_cell(cell);
    Location loc(next_alloc_id++);
    points_to_stack.back().alloc_local(mem_cell, loc);
  }

  void PointsToState::alloc_global(Value *cell) {
    MemCell mem_cell(cell);
    Location loc(next_alloc_id++);
    points_to[mem_cell].push_back(loc);
  }

  void PointsToState::assign_to_local(Value *target, Value *source) {
    MemCell target_cell(target), source_cell(source);
    points_to_stack. back().assign_to_local(target_cell, source_cell);
  }

  void PointsToState::assign_to_global(Value *target, Value *source) {
    MemCell target_cell(target), source_cell(source);
    points_to[target_cell].clear();
    points_to[target_cell] = points_to[source_cell];
  }

  void PointsToState::address_of_to_local(Value *target, Value *source) {
    MemCell target_cell(target), source_cell(source);
    points_to_stack.back().address_of_to_local(target_cell, source_cell);
  }

  void PointsToState::address_of_to_global(Value *target, Value *source) {
    MemCell target_cell(target), source_cell(source);
    Location source_loc(source_cell);
    points_to[target_cell].clear();
    points_to[target_cell].push_back(source_loc);
  }

  void PointsToState::load_to_local(Value *target, Value *address) {
    MemCell target_cell(target), address_cell(address);
    points_to_stack.back().load_to_local(target_cell, address_cell);
  }

  void PointsToState::load_to_global(Value *target, Value *address) {
    MemCell target_cell(target), address_cell(address);
    load_points_to(points_to, target_cell, address_cell);
  }

  void PointsToState::store_from_local(Value *source, Value *address) {
    MemCell source_cell(source), address_cell(address);
    points_to_stack.back().store_from_local(source_cell, address_cell);
  }

  void PointsToState::store_from_global(Value *source, Value *address) {
    MemCell source_cell(source), address_cell(address);
    store_points_to(points_to, source_cell, address_cell);
  }

  void PointsToState::push_frame(Function *function) {
    PointsToFrame points_to_frame(function);
    llvm::errs() << "PUSH FRAME " << function->getName() << "\n";
    points_to_stack.push_back(points_to_frame);
  }

  Function *PointsToState::pop_frame() {
    Function *ret = NULL;
    if (!points_to_stack.empty()) {
	ret = points_to_stack.back().getFunction();
	points_to_stack.pop_back();
    }
    return ret;
  }

  void PointsToState::print(llvm::raw_ostream& stream) const {
    unsigned i = 0;
    stream << "Globals[";
    for (map< MemCell, vector<Location> >::const_iterator it0 = points_to.begin();
	it0 != points_to.end(); it0++) {
      it0->first.print(stream);
      stream << "->[";
      unsigned j = 0;
      for (vector<Location>::const_iterator it1 = it0->second.begin(); it1 != it0->second.end(); it1++) {
	it1->print(stream);
	if (j++ < it0->second.size() - 1) {
	  stream << ",";
	}
      }
      stream << "]";
      if (i++ < points_to.size() - 1) {
	stream << ",";
      }
    }
    stream << "]\n";
    stream << "Stack[\n";
    i = 0;
    for (vector<PointsToFrame>::const_iterator it0 = points_to_stack.begin(); it0 != points_to_stack.end(); it0++) {
      it0->print(stream);
      if (i++ < points_to_stack.size() - 1) {
	stream << ",";
      }
    }
    stream << "]\n";
  }

  /**/

  void load_points_to(map<MemCell, vector<Location> >& points_to, const MemCell& target, const MemCell& address) {
    /// FIXME This segfaults here: vector<Location> loc_vector = points_to[address];
    for (vector<Location>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	MemCell content = pointed_to_location->get_content();
	vector<Location> pointed_to_by_target = points_to[target];
	vector<Location> pointed_to_by_content = points_to[content];
	pointed_to_by_target.insert(pointed_to_by_target.end(), pointed_to_by_content.begin(), pointed_to_by_content.end());
    }
  }

  void store_points_to(map<MemCell, vector<Location> >& points_to, const MemCell& source, const MemCell& address) {
    for (vector<Location>::iterator pointed_to_location = points_to[address].begin();
	pointed_to_location != points_to[address].end();
	pointed_to_location++) {
	MemCell content = pointed_to_location->get_content();
	vector<Location> pointed_to_by_source = points_to[source];
	vector<Location> pointed_to_by_content = points_to[content];
	pointed_to_by_content.insert(pointed_to_by_content.end(), pointed_to_by_source.begin(), pointed_to_by_source.end());
    }
  }

}
