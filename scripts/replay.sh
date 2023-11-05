#!/usr/bin/env bash

# ===-- replay.sh --------------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

for f in `find $1 -name "*.ktest" -type f`; do
  KLEE_RUN_TEST_ERRORS_NON_FATAL=STOP KTEST_FILE=$f timeout 1 $2
done
