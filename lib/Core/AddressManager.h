#ifndef _ADDRESS_MANAGER_H
#define _ADDRESS_MANAGER_H

#include "Memory.h"

#include "klee/Expr/ExprHashMap.h"
#include "klee/Solver/AddressGenerator.h"

#include <cstdint>
#include <map>
#include <unordered_map>

namespace klee {
class MemoryManager;
class Array;

class AddressManager : public AddressGenerator {
  friend MemoryManager;

private:
  MemoryManager *memory;
  ExprHashMap<IDType> bindingsAdressesToObjects;
  uint64_t maxSize;

public:
  AddressManager(MemoryManager *memory, uint64_t maxSize)
      : memory(memory), maxSize(maxSize) {}
  void addAllocation(ref<Expr> address, IDType id);
  void *allocate(ref<Expr> address, uint64_t size) override;
  MemoryObject *allocateMemoryObject(ref<Expr> address, uint64_t size);
  bool isAllocated(ref<Expr>);
};

} // namespace klee

#endif
