#!/usr/bin/env bash

# ===-- replay.sh --------------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  TIMEOUT="timeout"
else
  TIMEOUT="gtimeout"
fi
find $1 -name "*.ktest" -type f -exec bash -c 'KLEE_RUN_TEST_ERRORS_NON_FATAL=STOP KTEST_FILE=$1 $3 1 $2' bash {} $2 $TIMEOUT \;
exit 0
