# CMake build system

KLEE has a CMake build system.

## Useful top-level targets

* `check` - Build and run all tests.
* `clean` - Clean the build tree.
* `docs` - Build documentation.
* `edit_cache` - Show cmake/ccmake/cmake-gui interface for changing configure options.
* `help` - Show list of top-level targets.
* `systemtests` - Build and run system tests.
* `unittests` - Build and run unit tests.

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

* `ENABLE_POSIX_RUNTIME` (BOOLEAN) - Enable POSIX runtime.

* `ENABLE_SOLVER_METASMT` (BOOLEAN) - Enable MetaSMT solver support.

* `ENABLE_SOLVER_STP` (BOOLEAN) - Enable STP solver support.

* `ENABLE_SOLVER_Z3` (BOOLEAN) - Enable Z3 solver support.

* `ENABLE_TCMALLOC` (BOOLEAN) - Enable TCMalloc support.

* `ENABLE_UNIT_TESTS` (BOOLEAN) - Enable KLEE unit tests.

* `ENABLE_ZLIB` (BOOLEAN) - Enable zlib support.

* `GTEST_SRC_DIR` (STRING) - Path to Google Test source tree. If it is not
   specified, CMake will try to reuse the version included within the LLVM
   source tree or find a system installation of Google Test.

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

* `LLVM_DIR` (STRING) - Path to the target LLVM install directory

* `metaSMT_DIR` (STRING) - Provides a hint to CMake, where the metaSMT constraint
  solver can be found.  This should be an absolute path to a directory
  containing the file `metaSMTConfig.cmake`.

* `STP_DIR` (STRING) - Provides a hint to CMake, where the STP constraint
  solver can be found.  This should be an absolute path to a directory
  containing the file `STPConfig.cmake`. This file is installed by STP
  but also exists in its build directory. This allows KLEE to link
  against STP in a build directory or an installed copy.

* `WARNINGS_AS_ERRORS` (BOOLEAN) - Treat warnings as errors when building KLEE.
