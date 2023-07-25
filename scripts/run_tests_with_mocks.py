#!/usr/bin/env python3

# This script builds executable file using initial bitcode file and artifacts produced by KLEE.
# To run the script provide all the arguments you want to pass to clang for building executable.
#
# NOTE: First argument is path to compiler
# NOTE: Pre-last argument should be a path to KLEE output directory which contains redefinitions.txt and externals.ll.
# NOTE: Last argument is path to initial bitcode.
#
# Example: python3 run_tests_with_mocks.py clang -I ~/klee/include/ -L ~/klee/build/lib/ -lkleeRuntest klee-last a.bc
#
# You can read more about replaying test cases here: http://klee.github.io/tutorials/testing-function/

import subprocess as sp
import sys
import os

klee_out_dir = sys.argv[-2]
bitcode = sys.argv[-1]

filename = os.path.splitext(bitcode)[0]
object_file = f'{filename}.o'
sp.run(f'llc {bitcode} -filetype=obj -o {object_file}', shell=True)
sp.run(f'llvm-objcopy {object_file} --redefine-syms {klee_out_dir}/redefinitions.txt', shell=True)

externals = f'{klee_out_dir}/externals.o'
sp.run(f'llc {klee_out_dir}/externals.ll -filetype=obj -o {externals}', shell=True)

compiler = sys.argv[1]
user_args = ' '.join(sys.argv[2:len(sys.argv) - 2])
command = f'{compiler} {externals} {object_file} {user_args}'
print(command)
sp.run(command, shell=True)
