#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
run_tests() 
{
  build_dir="$1"
  KLEE_SRC="$(cd "${DIR}"/../../ && pwd)"

  # TODO change to pinpoint specific directory
  cd "${build_dir}"

  # Remove klee from PATH
  export PATH="/home/klee/klee_build/bin:$PATH"
  if which klee; then
    return 1 # should not happen
  fi

clang -c -emit-llvm -O0 test/unittestsmark/No_Errors_Build_Test_Code.cpp
klee test/unittestsmark/No_Errors_Build_Test_Code.bc
clang -c -emit-llvm -O0 test/unittestsmark/racecond.cpp
klee test/unittestsmark/racecond.bc
clang -c -emit-llvm -O0 test/unittestsmark/undefined_behaviour.cpp
klee test/unittestsmark/undefined_behaviour.bc
clang -c -emit-llvm -O0 test/unittestsmark/deadlock.cpp
klee test/unittestsmark/deadlock.bc
echo "why"
}