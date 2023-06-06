#ifndef _ADDRESS_GENERATOR_H
#define _ADDRESS_GENERATOR_H

#include "klee/ADT/Ref.h"

#include <cstdint>

namespace klee {
class Expr;

class AddressGenerator {
public:
  virtual void *allocate(ref<Expr>, uint64_t size) = 0;
  virtual ~AddressGenerator() = default;
};
}; // namespace klee

#endif
