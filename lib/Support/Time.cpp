//===-- Time.cpp ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Config/Version.h"
#include "klee/Internal/System/Time.h"

#include "llvm/Support/Timer.h"
#include "llvm/Support/Process.h"

using namespace llvm;
using namespace klee;

double util::getUserTime() {
  auto time_rec = TimeRecord::getCurrentTime();
  return time_rec.getUserTime();
}

double util::getWallTime() {
  auto time_rec = TimeRecord::getCurrentTime();
  return time_rec.getWallTime();
}

uint64_t util::getWallTimeMicros() {
  std::time_t st = std::time(NULL);
  auto millies = static_cast<std::chrono::milliseconds>(st).count();
  return static_cast<uint64_t>(millies);
}
