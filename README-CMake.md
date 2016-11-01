# CMake build system

KLEE now has a CMake build system which is intended to replace
its autoconf/Makefile based build system.

## Useful top level targets

* `check` - Build and run all tests.
* `clean` - Clean the build tree. Note this won't clean the runtime build.
* `docs` - Build documentation
* `edit_cache` - Show cmake/ccmake/cmake-gui interface for chaning configure options.
* `help` - Show list of top level targets
* `integrationtests` - Run integration tests
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

* `ENABLE_INTEGRATION_TESTS` (BOOLEAN) - Enable KLEE integration tests.

* `ENABLE_KLEE_ASSERTS` (BOOLEAN) - Enable assertions when building KLEE.

* `ENABLE_KLEE_UCLIBC` (BOOLEAN) - Enable support for klee-uclibc.

* `ENABLE_POSIX_RUNTIME` (BOOLEAN) - Enable POSIX runtime.

* `ENABLE_SOLVER_METASMT` (BOOLEAN) - Enable MetaSMT solver support.

* `ENABLE_SOLVER_STP` (BOOLEAN) - Enable STP solver support.

* `ENABLE_SOLVER_Z3` (BOOLEAN) - Enable Z3 solver support.

* `ENABLE_TCMALLOC` (BOOLEAN) - Enable TCMalloc support.

* `ENABLE_TESTS` (BOOLEAN) - Enable testing.

* `ENABLE_UNIT_TESTS` (BOOLEAN) - Enable KLEE unit tests.

* `GTEST_SRC_DIR` (STRING) - Path to GTest source tree.

* `KLEE_ENABLE_TIMESTAMP` (BOOLEAN) - Enable timestamps in KLEE sources.

* `KLEE_UCLIBC_PATH` (STRING) - Path to klee-uclibc root directory.

* `KLEE_RUNTIME_BUILD_TYPE` (STRING) - Build type for KLEE's runtimes.
   Can be `Release`, `Release+Asserts`, `Debug` or `Debug+Asserts`.

* `LIT_TOOL` (STRING) - Path to lit testing tool.

* `LIT_ARGS` (STRING) - Semi-colon separated list of lit options.

* `LLVM_CONFIG_BINARY` (STRING) - Path to `llvm-config` binary. This is
   only relevant if `USE_CMAKE_FIND_PACKAGE_LLVM` is `FALSE`. This is used
   to detect the LLVM version and find libraries.

* `MAKE_BINARY` (STRING) - Path to `make` binary used to build KLEE's runtime.

* `USE_CMAKE_FIND_PACKAGE_LLVM` (BOOLEAN) - Use `find_package(LLVM CONFIG)`
   to find LLVM.

* `USE_CXX11` (BOOLEAN) - Use C++11.

* `WARNINGS_AS_ERRORS` (BOOLEAN) - Treat warnings as errors when building KLEE.
