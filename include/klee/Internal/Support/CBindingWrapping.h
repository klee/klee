//===-- CBindingWrapping.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CBINDING_WRAPPING_H
#define KLEE_CBINDING_WRAPPING_H

#define KLEE_DEFINE_C_WRAPPERS(type, wrapper)                                  \
  inline type *unwrap(wrapper Wrapped) {                                       \
    return reinterpret_cast<type *>(Wrapped);                                  \
  }                                                                            \
                                                                               \
  inline wrapper wrap(const type *ToWrap) {                                    \
    return reinterpret_cast<wrapper>(const_cast<type *>(ToWrap));              \
  }

#endif
