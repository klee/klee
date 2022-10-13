//===-- tagged_logger.h -----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KDALLOC_UTIL_TAGGED_LOGGER_H
#define KDALLOC_UTIL_TAGGED_LOGGER_H

#include "define.h"

#include <utility>

#if KDALLOC_TRACE >= 1
#include <iostream>
#endif

namespace klee::kdalloc {
template <typename C> class TaggedLogger {
  template <typename O> inline void traceLineImpl(O &out) const noexcept {
    out << "\n";
  }

  template <typename O, typename T, typename... V>
  inline void traceLineImpl(O &out, T &&head, V &&...tail) const noexcept {
    out << head;
    traceLineImpl(out, std::forward<V>(tail)...);
  }

protected:
  template <typename... V> inline void traceLine(V &&...args) const noexcept {
#if KDALLOC_TRACE >= 1
    traceLineImpl(static_cast<C const *>(this)->logTag(std::cout),
                  std::forward<T>(args));
#endif
  }
};
} // namespace klee::kdalloc

#endif
