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
#include <memory>
#include <string>
namespace klee {
/// Tries to open a file given by path for writing and returns a handle to it if
/// successful.
///
/// @param path to the file
/// @param error contains an error message if file access was not successful
/// @return Returns file handle or nullptr if it could not be opened
std::unique_ptr<llvm::raw_fd_ostream> klee_open_output_file(std::string &path,
                                                            std::string &error);
}

#endif
