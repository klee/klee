//===-- PrintVersion.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/PrintVersion.h"
#include "klee/Config/CompileTimeInfo.h"
#include "klee/Config/Version.h"
#include "klee/Config/config.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

void klee::printVersion(llvm::raw_ostream &OS) {
  OS << PACKAGE_STRING " (" PACKAGE_URL ")\n";
#ifdef KLEE_ENABLE_TIMESTAMP
  OS << "  Built " __DATE__ " (" __TIME__ ")\n";
#endif
  OS << "  Build mode: " << KLEE_BUILD_MODE "\n";
  OS << "  Build revision: ";
#ifdef KLEE_BUILD_REVISION
  OS << KLEE_BUILD_REVISION "\n";
#else
  OS << "unknown\n";
#endif
  // Show LLVM version information
  OS << "\n";
  llvm::cl::PrintVersionMessage();
}
