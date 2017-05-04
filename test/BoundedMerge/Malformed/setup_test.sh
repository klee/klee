#!/bin/sh

llvmgcc=$1
source_file=$2
tmp_file=$3
add_flags=$4

$llvmgcc -emit-llvm -g -c -o $tmp_file.bc $source_file
rm -rf $tmp_file.klee-out
klee --output-dir=$tmp_file.klee-out --use-bounded-merge --debug-log-bounded-merge $add_flags $tmp_file.bc 2>&1
