#ifndef _ADDRESS_MANAGER_H
#define _ADDRESS_MANAGER_H

#include "Memory.h"

#include "klee/Solver/AddressGenerator.h"

#include <cstdint>
#include <map>
#include <unordered_map>

namespace klee {
class MemoryManager;
class Array;

class AddressManager : public AddressGenerator {
private:
  MemoryManager *memory;
  std::unordered_map<const Array *, IDType> bindingsArraysToObjects;

public:
  AddressManager(MemoryManager *memory) : memory(memory) {}
  void addAllocation(const Array *, IDType);
  void *allocate(const Array *array, uint64_t size) override;
  MemoryObject *allocateMemoryObject(const Array *array, uint64_t size);
};

} // namespace klee

#endif
