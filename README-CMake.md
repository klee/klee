# CMake build system

KLEE now has a CMake build system which is intended to replace
its autoconf/Makefile based build system.

## Useful top level targets

* `check` - Build and run all tests.
* `clean` - Invoke CMake's built-in target to clean the build tree.  Note this
  won't invoke the `clean_*` targets. It is advised that the `clean_all` target
  is used instead.
* `clean_all` - Run all clean targets.
* `clean_doxygen` - Clean doxygen build tree.
* `clean_runtime` - Clean the runtime build tree.
* `docs` - Build documentation
* `edit_cache` - Show cmake/ccmake/cmake-gui interface for chaning configure options.
* `help` - Show list of top level targets
* `systemtests` - Run system tests
* `unittests` - Build and run unittests

## Useful CMake variables

These can be set by passing `-DVAR_NAME=VALUE` to the CMake invocation.

e.g.

```
cmake -DCMAKE_BUILD_TYPE=Release /path/to/klee/src
```
* `LLVMCC` (STRING) - Path to the LLVM C compiler (e.g. Clang).

* `LLVMCXX` (STRING) - Path to the LLVM C++ compiler (e.g. Clang++).

* `CMAKE_BUILD_TYPE` (STRING) - Build type for KLEE. Can be
  `Debug`, `Release`, `RelWithDebInfo` or `MinSizeRel`.

* `DOWNLOAD_LLVM_TESTING_TOOLS` (BOOLEAN) - Force downloading
   of LLVM testing tool sources.

* `ENABLE_DOCS` (BOOLEAN) - Enable building documentation.

* `ENABLE_DOXYGEN` (BOOLEAN) - Enable building doxygen documentation.

* `ENABLE_SYSTEM_TESTS` (BOOLEAN) - Enable KLEE system tests.

* `ENABLE_KLEE_ASSERTS` (BOOLEAN) - Enable assertions when building KLEE.

* `ENABLE_KLEE_EH_CXX` (BOOLEAN) - Enable support for C++ Exceptions.

* `ENABLE_KLEE_LIBCXX` (BOOLEAN) - Enable libc++ for klee.

* `ENABLE_KLEE_UCLIBC` (BOOLEAN) - Enable support for klee-uclibc.

* `ENABLE_POSIX_RUNTIME` (BOOLEAN) - Enable POSIX runtime.

* `ENABLE_SOLVER_METASMT` (BOOLEAN) - Enable MetaSMT solver support.

* `ENABLE_SOLVER_STP` (BOOLEAN) - Enable STP solver support.

* `ENABLE_SOLVER_Z3` (BOOLEAN) - Enable Z3 solver support.

* `ENABLE_TCMALLOC` (BOOLEAN) - Enable TCMalloc support.

* `ENABLE_UNIT_TESTS` (BOOLEAN) - Enable KLEE unit tests.

* `ENABLE_ZLIB` (BOOLEAN) - Enable zlib support.

* `GTEST_SRC_DIR` (STRING) - Path to Google Test source tree. If it is not
   specified and `USE_CMAKE_FIND_PACKAGE_LLVM` is used, CMake will try to reuse
   the version included within the LLVM source tree.

* `GTEST_INCLUDE_DIR` (STRING) - Path to Google Test include directory,
   if it is not under `GTEST_SRC_DIR`.

* `KLEE_ENABLE_TIMESTAMP` (BOOLEAN) - Enable timestamps in KLEE sources.

* `KLEE_LIBCXX_DIR` (STRING) - Path to directory containing libc++ shared object (bitcode).

* `KLEE_LIBCXX_INCLUDE_DIR` (STRING) - Path to libc++ include directory.

* `KLEE_LIBCXXABI_SRC_DIR` (STRING) - Path to libc++abi source directory.

* `KLEE_UCLIBC_PATH` (STRING) - Path to klee-uclibc root directory.

* `KLEE_RUNTIME_BUILD_TYPE` (STRING) - Build type for KLEE's runtimes.
   Can be `Release`, `Release+Asserts`, `Debug` or `Debug+Asserts`.

* `LIT_TOOL` (STRING) - Path to lit testing tool.

* `LIT_ARGS` (STRING) - Semi-colon separated list of lit options.

* `LLVM_CONFIG_BINARY` (STRING) - Path to `llvm-config` binary. This is
   only relevant if `USE_CMAKE_FIND_PACKAGE_LLVM` is `FALSE`. This is used
   to detect the LLVM version and find libraries.

* `LLVM_DIR` (STRING) - Path to `LLVMConfig.cmake`. This is only relevant if
   `USE_CMAKE_FIND_PACKAGE_LLVM` is `TRUE`. This can be used to tell CMake where
   it can find LLVM outside of standard directories.

* `metaSMT_DIR` (STRING) - Provides a hint to CMake, where the metaSMT constraint
  solver can be found.  This should be an absolute path to a directory
  containing the file `metaSMTConfig.cmake`.

* `STP_DIR` (STRING) - Provides a hint to CMake, where the STP constraint
  solver can be found.  This should be an absolute path to a directory
  containing the file `STPConfig.cmake`. This file is installed by STP
  but also exists in its build directory. This allows KLEE to link
  against STP in a build directory or an installed copy.

* `USE_CMAKE_FIND_PACKAGE_LLVM` (BOOLEAN) - Use `find_package(LLVM CONFIG)`
   to find LLVM (instead of using `llvm-config` with `LLVM_CONFIG_BINARY`).

* `WARNINGS_AS_ERRORS` (BOOLEAN) - Treat warnings as errors when building KLEE.
