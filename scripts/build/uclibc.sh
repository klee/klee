#!/bin/bash -x
# Build the uclibc library
# Make sure we exit if there is a failure
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
source "${DIR}/common-defaults.sh"

if [[ -z "${KLEE_UCLIBC}" ]]; then
  echo "KLEE_UCLIBC must be specified: 0 or branch/tag"
  exit 1
fi

###############################################################################
# Handling LLVM configuration
###############################################################################
if [[ "$TRAVIS_OS_NAME" = "linux"  ||  "$TRAVIS_OS_NAME" = "osx" ]] ; then
  LLVM_CONFIG="${LLVM_BIN}/llvm-config"
  KLEE_CC="${LLVM_BIN}/clang"
  KLEE_CXX="${LLVM_BIN}/clang++"
else
  echo "Unhandled TRAVIS_OS_NAME \"${TRAVIS_OS_NAME}\""
  exit 1
fi

###############################################################################
# klee-uclibc
###############################################################################
if [ "${KLEE_UCLIBC}" != "0" ]; then
  if [[ "$TRAVIS_OS_NAME" = "osx" ]] ; then
    echo "UCLibc is not supported Mac OSX"
    exit 1
  fi
  git_clone_or_update git://github.com/klee/klee-uclibc.git "${BASE}/klee-uclibc-${LLVM_VERSION_SHORT}" "${KLEE_UCLIBC}"
  cd "${BASE}/klee-uclibc-${LLVM_VERSION_SHORT}"
  ./configure --make-llvm-lib --with-cc ${KLEE_CC} --with-llvm-config ${LLVM_CONFIG}
  make
fi
