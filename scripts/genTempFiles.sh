#!/usr/bin/env bash

# ===-- genTempFiles.sh ---------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

if [ -z "$1" ] ; then
	echo "No directory given"
	exit 1
fi

mkdir -p $1
for i in `seq 1 100`; do
	mkdir -p $1/dir_foo_$i
	touch $1/tmp_foo_$i
done    
