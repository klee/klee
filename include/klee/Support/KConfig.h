//===-- KConfig.h -----------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_KCONFIG_H
#define KLEE_KCONFIG_H

#include "llvm/Support/raw_ostream.h"

#include <map>
#include <memory>
#include <string>

namespace klee {
class KConfig {
private:
  KConfig() = default;

  std::map<std::string, std::string> configurations;

public:
  static KConfig &get() {
    static KConfig singleton;
    return singleton;
  }

  void register_option(std::string key, std::string value);

  void manifest(llvm::raw_ostream &file);

  KConfig(const KConfig &) = delete;
  KConfig &operator=(const KConfig &) = delete;
};

} // namespace klee
#endif // KLEE_KCONFIG_H
