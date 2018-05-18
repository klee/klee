#!/bin/bash
set -e
###########################################################################
# Install KLEE dependencies
###########################################################################
DIR="$(cd "$(dirname "$0")" && pwd)"

if [[ "${BASE}x" == "x" ]]; then
  echo "\${BASE} not set"
  exit 1
fi

# Install LLVM and the LLVM bitcode compiler we require to build KLEE
"${DIR}/llvm.sh"

# Install allocators
"${DIR}/tcmalloc.sh"

# Get SMT solvers
"${DIR}/solvers.sh"

# Get needed utlities/libraries for testing KLEE
"${DIR}/testing-utils.sh"

# Install uclibc
"${DIR}/uclibc.sh"
