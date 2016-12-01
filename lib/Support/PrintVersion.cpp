//===-- PrintVersion.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Internal/Support/PrintVersion.h"
#include "klee/Config/config.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include "klee/Config/CompileTimeInfo.h"

void klee::printVersion()
{
  llvm::outs() << PACKAGE_STRING " (" PACKAGE_URL ")\n";
#ifdef KLEE_ENABLE_TIMESTAMP
  llvm::outs() << "  Built " __DATE__ " (" __TIME__ ")\n";
#endif
  llvm::outs() << "  Build mode: " << KLEE_BUILD_MODE "\n";
  llvm::outs() << "  Build revision: ";
#ifdef KLEE_BUILD_REVISION
  llvm::outs() << KLEE_BUILD_REVISION "\n";
#else
  llvm::outs() << "unknown\n";
#endif
  // Show LLVM version information
  llvm::outs() << "\n";
  llvm::cl::PrintVersionMessage();
}
