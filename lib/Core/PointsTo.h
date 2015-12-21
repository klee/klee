
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

namespace klee {

  class FlowsTo {
    llvm::Value *source;
    llvm::Value *target;

  public:
    FlowsTo(llvm::Value *source, llvm::Value *target);

    ~FlowsTo();

    llvm::Value *getSouce() {
      return source;
    }

    llvm::Value *getTarget() {
      return target;
    }
  };

  class Loc {

  };

  class MemCell {
    llvm::Value *llvm_value;

  public:
    MemCell(llvm::Value *_llvm_value);

    ~MemCell();

    llvm::Value *get_llvm() const;

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
    llvm::Value *content;
    unsigned long alloc_id;

  public:
    Location(unsigned long alloc_id);

    Location(llvm::Value *content, unsigned long alloc_id);

    ~Location();

    void set_content(llvm::Value *content);

    llvm::Value *get_content();

    bool operator==(const Location& rhs) const;

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }
  };

  class PointsToFrame {
    llvm::Function *function;
    std::map< MemCell, Location *> points_to;

  public:
    PointsToFrame(llvm::Function *function);

    ~PointsToFrame();

    llvm::Function *getFunction();

    void alloc_local(llvm::Value *llvm_value, unsigned alloc_id);

    void address_of_to_local(llvm::Value *target, llvm::Value *source);

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
    std::vector<PointsToFrame> points_to_stack;
    std::map< MemCell, Location *> points_to;
    unsigned long next_alloc_id;

  public:
    PointsToState();

    ~PointsToState();

    void push_frame(llvm::Function *callee);

    llvm::Function *pop_frame();

    void alloc_local(llvm::Value *cell);

    void alloc_global(llvm::Value *cell);

    void address_of_to_local(llvm::Value *target, llvm::Value *source);

    void address_of_to_global(llvm::Value *target, llvm::Value *source);

    void assign_to_local(llvm::Value *target, llvm::Value *source);

    void assign_to_global(llvm::Value *target, llvm::Value *source);

    void load_to_local(llvm::Value *target, llvm::Value *address);

    void load_to_global(llvm::Value *target, llvm::Value *address);

    void store_from_local(llvm::Value *source, llvm::Value *address);

    void store_from_global(llvm::Value *source, llvm::Value *address);

    void print(llvm::raw_ostream& stream) const;

    void dump() const {
      this->print(llvm::errs());
      llvm::errs() << "\n";
    }
  };


  /// Function declarations
  void store_points_to(std::map<MemCell, Location *>& points_to, const MemCell& source, const MemCell& address);

  void load_points_to(std::map<MemCell, Location *>& points_to, const MemCell& target, const MemCell& address);

}

#endif
