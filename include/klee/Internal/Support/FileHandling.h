//===-- FileHandling.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef __KLEE_FILE_HANDLING_H__
#define __KLEE_FILE_HANDLING_H__
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace klee {
llvm::raw_fd_ostream *klee_open_output_file(std::string &path,
                                            std::string &error);
}

#endif
