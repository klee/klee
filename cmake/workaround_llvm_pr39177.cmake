# Workaround for LLVM PR39177
#  - https://bugs.llvm.org/show_bug.cgi?id=39177
#  - https://github.com/klee/klee/issues/1000
#
# TODO: remove when support for LLVM <= 7 is dropped
#
# Short description of the bug:
# The LLVM pass `-instcombine` optimizes calls to C standard lib functions by,
# e.g. transforming the following call to a call to fwrite():
#   fprintf(stderr, "hello world!\n");
# In uClibc, and thus klee-uclibc, fwrite() is defined as an alias to a function
# fwrite_unlocked(). This translates to a GlobalAlias in LLVM IR. When trying to
# infer function attributes from fwrite(), LLVM tries to cast a GlobalAlias to a
# Function, which results in a null-pointer dereference. When calling KLEE with
# `-optimize`, this leads to a crash of KLEE.
#
# This bug affects LLVM 3.9 - 7.0.0.
#
# As the bug results in a null-pointer dereference when trying to access a
# Function that is only available as GlobalAlias, this workaround introduces a
# pass into KLEE that replaces aliases for certain C standard lib function with
# clones of the corresponding aliasee function.
#
# The bug was fixed in the following commits in LLVM:
#  - https://reviews.llvm.org/rL344454
#  - https://reviews.llvm.org/rL344455
#  - https://reviews.llvm.org/rL344645
# These patches were then applied to the release_70 branch to land in 7.0.1:
#  - https://reviews.llvm.org/rL345921
#
# This CMake file checks whether the method responsible for the null-pointer
# dereference leads to a crash of the program given in this file.
#
# Files that were created/modified for this workaround:
# [NEW FILE] cmake/workaround_llvm_pr39177.cmake (this file)
# [NEW FILE] cmake/workaround_llvm_pr39177.ll (auxiliary file for feature test)
# [NEW FILE] lib/Module/WorkaroundLLVMPR39177.cpp
#
# [MODIFIED] CMakeLists.txt (including this file)
# [MODIFIED] include/klee/Config/config.h.cmin (cmakedefine)
# [MODIFIED] lib/Module/CMakeLists.txt
# [MODIFIED] lib/Module/Optimize.cpp (add pass during optimization)
# [MODIFIED] lib/Module/Passes.h

# Detect whether LLVM version is affected by PR39177
if ((${LLVM_VERSION_MAJOR} GREATER 3 OR (${LLVM_VERSION_MAJOR} EQUAL 3 AND ${LLVM_VERSION_MINOR} EQUAL 9)) # LLVM >= 3.9
   AND (${LLVM_VERSION_MAJOR} LESS 7 OR (${LLVM_VERSION_MAJOR} EQUAL 7 AND ${LLVM_VERSION_MINOR} EQUAL 0 AND ${LLVM_VERSION_PATCH} EQUAL 0))) # LLVM <= 7.0.0
  set(DISABLE_WORKAROUND_LLVM_PR39177_DEFAULT OFF)
else()
  set(DISABLE_WORKAROUND_LLVM_PR39177_DEFAULT ON)
endif()

option(DISABLE_WORKAROUND_LLVM_PR39177 "Disable Workaround for LLVM PR39177 (affecting LLVM 3.9 - 7.0.0)" ${DISABLE_WORKAROUND_LLVM_PR39177_DEFAULT})

if (NOT DISABLE_WORKAROUND_LLVM_PR39177)
  # Detect whether PR39177 leads to crash
  include(CheckCXXSourceRuns)

  cmake_push_check_state()
  klee_get_llvm_libs(LLVM_LIBS asmparser transformutils)
  set(CMAKE_REQUIRED_INCLUDES "${LLVM_INCLUDE_DIRS}")
  set(CMAKE_REQUIRED_LIBRARIES "${LLVM_LIBS}")

  check_cxx_source_runs("
    #include \"llvm/Analysis/TargetLibraryInfo.h\"
    #include \"llvm/AsmParser/Parser.h\"
    #include \"llvm/AsmParser/SlotMapping.h\"
    #include \"llvm/IR/ConstantFolder.h\"
    #include \"llvm/IR/Constants.h\"
    #include \"llvm/IR/DataLayout.h\"
    #include \"llvm/IR/DiagnosticInfo.h\"
    #include \"llvm/IR/Instructions.h\"
    #include \"llvm/IR/IRBuilder.h\"
    #include \"llvm/IR/LLVMContext.h\"
    #include \"llvm/Transforms/Utils/BuildLibCalls.h\"

    #include <signal.h>

    void handler(int, siginfo_t*, void*) {
      // program received SIGSEGV
      exit(1);
    }

    using namespace llvm;

    int main() {
      // capture segfault
      struct sigaction action;
      memset(&action, 0, sizeof(struct sigaction));
      action.sa_flags = SA_SIGINFO;
      action.sa_sigaction = handler;
      sigaction(SIGSEGV, &action, NULL);

      // setup test
      LLVMContext Ctx;
      SMDiagnostic Error;
      SlotMapping Mapping;
      auto M = llvm::parseAssemblyFile(\"${CMAKE_SOURCE_DIR}/cmake/workaround_llvm_pr39177.ll\", Error, Ctx, &Mapping);
      if (!M) {
        Error.print(\"AssemblyString\", llvm::errs());
        return -1;
      }

      auto *F = M->getFunction(\"test\");
      auto *CI = cast<CallInst>(&*std::next(F->begin()->begin()));
      auto &DL = M->getDataLayout();
      Value *Size = ConstantInt::get(DL.getIntPtrType(Ctx), 8);
      ConstantFolder CF;
      IRBuilder<> B(&*F->begin(), CF);
      TargetLibraryInfo TLI = TargetLibraryInfoWrapperPass({\"x86_64\", \"\", \"linux-gnu\"}).getTLI();

      // test if this call produces segfault
      emitFWrite(CI->getArgOperand(1), Size, CI->getArgOperand(0), B, DL, &TLI);

      return 0;
    }"
    LLVM_PR39177_FIXED
  )
  cmake_pop_check_state()

  if (NOT LLVM_PR39177_FIXED)
    message(STATUS "Workaround for LLVM PR39177 (affecting LLVM 3.9 - 7.0.0) enabled")
    set(USE_WORKAROUND_LLVM_PR39177 1) # For config.h
  else()
    message(FATAL_ERROR "DISABLE_WORKAROUND_LLVM_PR39177 is not set to true"
      "but crash resulting from PR39177 could not be detected."
      "You may try to disable the workaround using"
      "-DDISABLE_WORKAROUND_LLVM_PR39177=1 if you believe the issue is patched"
      "in your version of LLVM.")
  endif()
else()
  message(STATUS "Workaround for LLVM PR39177 (affecting LLVM 3.9 - 7.0.0) disabled")
  unset(USE_WORKAROUND_LLVM_PR39177) # For config.h
endif()
