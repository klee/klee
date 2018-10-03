//===-- FileHandling.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_FILEHANDLING_H
#define KLEE_FILEHANDLING_H

#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <string>

namespace klee {
std::unique_ptr<llvm::raw_fd_ostream>
klee_open_output_file(const std::string &path, std::string &error);

#ifdef HAVE_ZLIB_H
std::unique_ptr<llvm::raw_ostream>
klee_open_compressed_output_file(const std::string &path, std::string &error);
#endif
} // namespace klee

#endif /* KLEE_FILEHANDLING_H */
