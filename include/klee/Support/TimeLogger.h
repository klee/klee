//===-- TimeLogger.h ----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TIMELOGGER_H
#define KLEE_TIMELOGGER_H

#include "klee/Statistics/Statistics.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/FileHandling.h"
#include "klee/Support/Timer.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

namespace klee {

class TimeLogger {
private:
  const WallTimer timer;
  std::string filePath;

public:
  explicit TimeLogger(std::string filePath) : filePath(std::move(filePath)) {}
  ~TimeLogger() {}

  void lap(std::string status) {
    if (filePath.empty()) {
      return;
    }
    auto delta = timer.delta().toMicroseconds();
    std::string error;
    auto file = klee_append_output_file(filePath, error);
    if (!file) {
      klee_error("Could not open file %s : %s", filePath.c_str(),
                 error.c_str());
    }
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    *file << now_time << "," << status << "," << delta << "\n";
  }
};
} // namespace klee

#endif /* KLEE_DELTATIMEDUMPER_H */
