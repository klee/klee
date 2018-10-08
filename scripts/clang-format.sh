#!/bin/bash
# clang-formats the current git directory
# Returns 1 if changes were detected and prints those changes, otherwise return 0
#
# Required applications are colordiff, clang-format-${LLVM_VERSION} clang-${LLVM_VERSION}
set -e
LLVM_VERSION=6.0

git diff -U0 ${COMMIT_RANGE} |clang-format-diff-${LLVM_VERSION} -p1 > clang_format.patch

if [ -s clang_format.patch ]; then
   rm clang_format.patch

   git diff -U0 ${COMMIT_RANGE} |clang-format-diff-${LLVM_VERSION} -p1 | colordiff
   exit 1
fi

rm clang_format.patch
exit 0
