//===-- FileHandling.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "klee/Internal/Support/FileHandling.h"
#include "klee/Config/Version.h"
#include "klee/Config/config.h"
#include "klee/Internal/Support/ErrorHandling.h"

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
#include "llvm/Support/FileSystem.h"
#endif

namespace klee {

llvm::raw_fd_ostream *klee_open_output_file(std::string &path,
                                            std::string &error) {
  llvm::raw_fd_ostream *f;
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
  std::error_code ec;
  f = new llvm::raw_fd_ostream(path.c_str(), ec, llvm::sys::fs::F_None);
  if (ec)
    error = ec.message();
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
  f = new llvm::raw_fd_ostream(path.c_str(), error, llvm::sys::fs::F_None);
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 4)
  f = new llvm::raw_fd_ostream(path.c_str(), error, llvm::sys::fs::F_Binary);
#else
  f = new llvm::raw_fd_ostream(path.c_str(), error,
                               llvm::raw_fd_ostream::F_Binary);
#endif
  if (!error.empty()) {
    if (f)
      delete f;
    f = NULL;
  }
  return f;
}
}
