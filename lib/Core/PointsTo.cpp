
#include "PointsTo.h"
#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif
#include "llvm/Support/raw_ostream.h"


using namespace klee;

namespace klee {

  Location::Location(llvm::Value *allocationSite) :
      constantDependency(0), allocationSite(allocationSite) {}

  Location::~Location() { dependencies.clear(); }

  bool Location::unInitialized() {
    return (dependencies.size() == 0 && constantDependency == 0);
  }

  bool Location::dependsOnConstant() {
    return (dependencies.size() == 0 && constantDependency != 0);
  }

  void Location::initializeWithNonConstant(std::vector<Location *> _dependencies) {
    assert(_dependencies.size() != 0);
    constantDependency = 0;
    dependencies.clear();
    dependencies.insert(dependencies.begin(),
                        _dependencies.begin(), _dependencies.end());
  }

  void Location::initializeWithConstant(llvm::Constant *constant) {
    dependencies.clear();
    constantDependency = constant;
  }

  std::vector<Location *> Location::getDependencies() {
    return dependencies;
  }

  void Location::addDependency(Location *extraDependency) {
    if (dependsOnConstant() || unInitialized()) {
	if (extraDependency->dependsOnConstant()) {
	    initializeWithConstant(extraDependency->constantDependency);
	} else {
	    initializeWithNonConstant(extraDependency->dependencies);
	}
	return;
    }

    dependencies.insert(dependencies.begin(),
                        extraDependency->dependencies.begin(),
                        extraDependency->dependencies.end());
  }

  MemCell::MemCell(llvm::Value *_llvm_value) :
      llvm_value(_llvm_value) {}

  MemCell::~MemCell() {}

  llvm::Value *MemCell::get_llvm() const {
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

  _Location::_Location(unsigned long alloc_id) :
      content(0), alloc_id(alloc_id) {}

  _Location::_Location(llvm::Value *value, unsigned long alloc_id) :
      content(value), alloc_id(alloc_id) {}

  _Location::~_Location() {}

  void _Location::set_content(llvm::Value *value) {
    content = value;
  }

  llvm::Value *_Location::get_content() {
    return content;
  }

  void _Location::print(llvm::raw_ostream& stream) const {
    stream << "Location[";
    if (content != 0) {
	content->print(stream);
    } else {
	stream << "NULL";
    }
    stream << "]";
  }

  bool _Location::operator==(const _Location& rhs) const {
    if (alloc_id > 0 && rhs.alloc_id > 0) {
	return alloc_id == rhs.alloc_id;
    } else if (alloc_id == 0 && rhs.alloc_id == 0) {
	return content == rhs.content;
    }
    return false;
  }

  PointsToFrame::PointsToFrame(llvm::Function *function) :
    function(function) {}

  PointsToFrame::~PointsToFrame()     {
    points_to.clear();
  }

  llvm::Function *PointsToFrame::getFunction() {
    return function;
  }

  void PointsToFrame::alloc_local(llvm::Value *llvm_value, unsigned alloc_id) {
    MemCell mem_cell(llvm_value);
    points_to[mem_cell] = new _Location(llvm_value, alloc_id);
  }

  void PointsToFrame::address_of_to_local(llvm::Value *target, llvm::Value *source) {
    MemCell mem_cell(target);
    points_to[mem_cell] = new _Location(source, 0);
  }

  void PointsToFrame::assign_to_local(const MemCell& target, const MemCell& source) {
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
    unsigned i;
    stream << "Frame[" << (function == 0? "MAIN" : function->getName()) << ":";
//    vector<MemCell> v;
//    for (map< MemCell, Location >::const_iterator it0 = points_to.begin();
//	it0 != points_to.end(); it0++) {
//	v.push_back(it0->first);
//    }
    i = 0;
    for (std::map< MemCell, _Location *>::const_iterator it0 = points_to.begin();
	it0 != points_to.end(); it0++) {
      it0->first.print(stream);
      stream << "->";
      it0->second->print(stream);
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

  void PointsToState::alloc_local(llvm::Value *cell) {
    points_to_stack.back().alloc_local(cell, next_alloc_id++);
  }

  void PointsToState::alloc_global(llvm::Value *cell) {
    MemCell mem_cell(cell);
    points_to[mem_cell] = new _Location(cell, next_alloc_id++);
  }

  void PointsToState::assign_to_local(llvm::Value *target, llvm::Value *source) {
    MemCell target_cell(target), source_cell(source);
    points_to_stack.back().assign_to_local(target_cell, source_cell);
  }

  void PointsToState::assign_to_global(llvm::Value *target, llvm::Value *source) {
    MemCell target_cell(target), source_cell(source);
    points_to[target_cell] = points_to[source_cell];
  }

  void PointsToState::address_of_to_local(llvm::Value *target, llvm::Value *source) {
    points_to_stack.back().address_of_to_local(target, source);
  }

  void PointsToState::address_of_to_global(llvm::Value *target, llvm::Value *source) {
    MemCell target_cell(target);
    points_to[target_cell] = new _Location(source, 0);
  }

  void PointsToState::load_to_local(llvm::Value *target, llvm::Value *address) {
    MemCell target_cell(target), address_cell(address);
    points_to_stack.back().load_to_local(target_cell, address_cell);
  }

  void PointsToState::load_to_global(llvm::Value *target, llvm::Value *address) {
    MemCell target_cell(target), address_cell(address);
    load_points_to(points_to, target_cell, address_cell);
  }

  void PointsToState::store_from_local(llvm::Value *source, llvm::Value *address) {
    MemCell source_cell(source), address_cell(address);
    points_to_stack.back().store_from_local(source_cell, address_cell);
  }

  void PointsToState::store_from_global(llvm::Value *source, llvm::Value *address) {
    MemCell source_cell(source), address_cell(address);
    store_points_to(points_to, source_cell, address_cell);
  }

  void PointsToState::push_frame(llvm::Function *function) {
    PointsToFrame points_to_frame(function);
    llvm::errs() << "PUSH FRAME " << function->getName() << "\n";
    points_to_stack.push_back(points_to_frame);
  }

  llvm::Function *PointsToState::pop_frame() {
    llvm::Function *ret = NULL;
    if (!points_to_stack.empty()) {
	ret = points_to_stack.back().getFunction();
	points_to_stack.pop_back();
    }
    return ret;
  }

  void PointsToState::print(llvm::raw_ostream& stream) const {
    unsigned i = 0;
    stream << "Globals[";
    for (std::map< MemCell, _Location*>::const_iterator it0 = points_to.begin();
	it0 != points_to.end(); it0++) {
      it0->first.print(stream);
      stream << "->";
      it0->second->print(stream);
      if (i++ < points_to.size() - 1) {
	stream << ",";
      }
    }
    stream << "]\n";
    stream << "Stack[\n";
    i = 0;
    for (std::vector<PointsToFrame>::const_iterator it0 = points_to_stack.begin(); it0 != points_to_stack.end(); it0++) {
      it0->print(stream);
      stream << "\n";
    }
    stream << "]\n";
  }

  /**/

  void load_points_to(std::map<MemCell, _Location *>& points_to, const MemCell& target, const MemCell& address) {
    MemCell content(points_to[address]->get_content());
    points_to[target] = points_to[content];
  }

  void store_points_to(std::map<MemCell, _Location *>& points_to, const MemCell& source, const MemCell& address) {
    llvm::errs() << __FUNCTION__ << "\n";
    llvm::errs() << "SOURCE\n";
    source.dump();
    llvm::errs() << "ADDRESS\n";
    address.dump();


    if (points_to.find(address) == points_to.end() ||
	points_to.find(source) == points_to.end())
      return;

    llvm::errs() << "ADDRESS FOUND\n";
    MemCell content(points_to[address]->get_content());
    if (points_to.find(content) != points_to.end()) {
	points_to[content] = points_to[source];
    }
  }

}
