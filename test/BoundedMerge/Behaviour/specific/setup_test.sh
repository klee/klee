#!/bin/sh

llvmgcc=$1
source_file=$2
tmp_file=$3
add_flags=$4

bc_file=$tmp_file.bc
klee_out=$tmp_file.klee-out

$llvmgcc -emit-llvm -g -c -o $bc_file $source_file
rm -rf $klee_out
klee --output-dir=$klee_out --use-bounded-merge --debug-log-bounded-merge $add_flags $bc_file 2>&1
