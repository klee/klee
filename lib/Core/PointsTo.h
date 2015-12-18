
#ifndef KLEE_SYMBOLICSTATE_H
#define KLEE_SYMBOLICSTATE_H

#include "klee/Config/Version.h"

#if LLVM_VERSION_CODE > LLVM_VERSION(3, 2)
#include "llvm/IR/Function.h"
#else
#include "llvm/Function.h"
#endif
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <stack>

using namespace std;
using namespace llvm;

namespace llvm {
  class Function;
  class Value;
}

namespace klee {

  class MemCell {
    Value *llvm_value;

  public:
    MemCell(Value *_llvm_value);

    ~MemCell();

    Value *get_llvm() const;

    bool operator ==(const MemCell& rhs) const {
      return llvm_value == rhs.llvm_value;
    }

    bool operator <(const MemCell& rhs) const {
      return llvm_value < rhs.llvm_value;
    }

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class Location {
    Value *content;
    unsigned long alloc_id;

  public:
    Location(unsigned long alloc_id);

    Location(Value *content, unsigned long alloc_id);

    ~Location();

    void set_content(Value *content);

    Value *get_content();

    bool operator==(const Location& rhs) const;

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class PointsToFrame {
    Function *function;
    map< MemCell, Location *> points_to;

  public:
    PointsToFrame(Function *function);

    ~PointsToFrame();

    Function *getFunction();

    void alloc_local(Value *llvm_value, unsigned alloc_id);

    void address_of_to_local(Value *target, Value *source);

    void assign_to_local(const MemCell& target, const MemCell& source);

    void load_to_local(const MemCell& target, const MemCell& address);

    void store_from_local(const MemCell& source, const MemCell& address);

    bool is_main_frame();

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class PointsToState {
    vector<PointsToFrame> points_to_stack;
    map< MemCell, Location *> points_to;
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

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }
  };


  /// Function declarations
  void store_points_to(map<MemCell, Location *>& points_to, const MemCell& source, const MemCell& address);

  void load_points_to(map<MemCell, Location *>& points_to, const MemCell& target, const MemCell& address);

}

#endif
