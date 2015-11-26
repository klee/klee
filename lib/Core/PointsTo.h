
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

  class MemoryCell {
    Value *llvm_value;
  public:
    MemoryCell();

    MemoryCell(Value *cell);

    ~MemoryCell();

    Value *get_llvm();

    bool operator==(const MemoryCell& other);

    friend bool operator<(const MemoryCell& lhs, const MemoryCell& rhs);

  };

  class Location {
    MemoryCell content;
    unsigned long alloc_id;
  public:
    Location(unsigned long alloc_id);

    Location(const MemoryCell& content);

    ~Location();

    void set_content(const MemoryCell& content);

    MemoryCell get_content();

    bool operator==(Location& rhs);
  };

  class PointsToFrame {
    Function *function;
    map< MemoryCell, vector<Location> > points_to;

  public:
    PointsToFrame(Function *function);

    ~PointsToFrame();

    Function *getFunction();

    void alloc_local(const MemoryCell& cell, const Location& location);

    void address_of_to_local(const MemoryCell& target, const MemoryCell& source);

    void assign_to_local(const MemoryCell& target, const MemoryCell& source);

    void load_to_local(const MemoryCell& target, const MemoryCell& address);

    void store_from_local(const MemoryCell& source, const MemoryCell& address);

    bool is_main_frame();

  };

  class PointsToState {
    stack< PointsToFrame, vector<PointsToFrame> > points_to_stack;
    map< MemoryCell, vector<Location> > points_to;
    unsigned long next_alloc_id;

  public:
    PointsToState();

    ~PointsToState();

    void push_frame(Function *callee);

    Function *pop_frame();

    void alloc_local(Value *cell);

    void alloc_global(Value *cell);

    void address_of_to_local(Value *target, Value *source);

    void address_of_to_global(Value *target, Value *source);

    void assign_to_local(Value *target, Value *source);

    void assign_to_global(Value *target, Value *source);

    void load_to_local(Value *target, Value *address);

    void load_to_global(Value *target, Value *address);

    void store_from_local(Value *source, Value *address);

    void store_from_global(Value *source, Value *address);

  };


  /// Function declarations
  void store_points_to(map<MemoryCell, vector<Location> >& points_to, const MemoryCell& source, const MemoryCell& address);

  void load_points_to(map<MemoryCell, vector<Location> >& points_to, const MemoryCell& target, const MemoryCell& address);

}

#endif
