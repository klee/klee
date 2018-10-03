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

#ifdef HAVE_ZLIB_H
#include "klee/Internal/Support/CompressionStream.h"
#endif

namespace klee {

std::unique_ptr<llvm::raw_fd_ostream>
klee_open_output_file(const std::string &path, std::string &error) {
  error = "";
  std::unique_ptr<llvm::raw_fd_ostream> f;
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 6)
  std::error_code ec;
  f = std::unique_ptr<llvm::raw_fd_ostream>(new llvm::raw_fd_ostream(path.c_str(), ec, llvm::sys::fs::F_None)); // FIXME C++14
  if (ec)
    error = ec.message();
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
  f = std::unique_ptr<llvm::raw_fd_ostream>(new llvm::raw_fd_ostream(path.c_str(), error, llvm::sys::fs::F_None)); // FIXME C++14
#else
  f = std::unique_ptr<llvm::raw_fd_ostream>(new llvm::raw_fd_ostream(path.c_str(), error, llvm::sys::fs::F_Binary)); // FIXME C++14
#endif
  if (!error.empty()) {
    f.reset(nullptr);
  }
  return f;
}

#ifdef HAVE_ZLIB_H
std::unique_ptr<llvm::raw_ostream>
klee_open_compressed_output_file(const std::string &path, std::string &error) {
  error = "";
  std::unique_ptr<llvm::raw_ostream> f(new compressed_fd_ostream(path, error));
  if (!error.empty()) {
    f.reset(nullptr);
  }
  return f;
}
#endif
}
