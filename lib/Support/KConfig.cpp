//===-- KConfig.cpp -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/KConfig.h"

#include <utility>

using namespace klee;

void KConfig::register_option(std::string key, std::string value) {
  configurations.insert({std::move(key), std::move(value)});
}

void KConfig::manifest(llvm::raw_ostream &file) {
  file << "{\n";
  bool first = true;

  for (const auto &[key, value] : configurations) {
    if (first) {
      first = false;
    } else {
      file << ",\n";
    }
    file << "\"" << key << "\""
         << ":"
         << "\"" << value << "\"";
  }
  file << "\n"
       << "}\n";
  file.flush();
}