//===-- FileHandling.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Support/FileHandling.h"

#include "klee/Config/Version.h"
#include "klee/Config/config.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/Support/FileSystem.h"

#ifdef HAVE_ZLIB_H
#include "klee/Support/CompressionStream.h"
#endif

namespace klee {

std::unique_ptr<llvm::raw_fd_ostream>
klee_open_output_file(const std::string &path, std::string &error) {
  error.clear();
  std::error_code ec;

  auto f = std::make_unique<llvm::raw_fd_ostream>(path.c_str(), ec,
                                                  llvm::sys::fs::OF_None);
  if (ec)
    error = ec.message();
  if (!error.empty()) {
    f.reset(nullptr);
  }
  return f;
}

#ifdef HAVE_ZLIB_H
std::unique_ptr<llvm::raw_ostream>
klee_open_compressed_output_file(const std::string &path, std::string &error) {
  error.clear();
  auto f = std::make_unique<compressed_fd_ostream>(path, error);
  if (!error.empty()) {
    f.reset(nullptr);
  }
  return f;
}
#endif
}
