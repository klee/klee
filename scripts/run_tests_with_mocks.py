#!/usr/bin/env python3

# This script builds executable file using initial bitcode file and artifacts produced by KLEE.
# To run the script provide all the arguments you want to pass to clang for building executable.
#
# NOTE: Pre-last argument should be a path to KLEE output directory which contains redefinitions.txt and externals.ll.
# NOTE: Last argument is path to initial bitcode.
#
# Example: python3 run_tests_with_mocks.py -I ~/klee/include/ -L ~/klee/build/lib/ -lkleeRuntest klee-last a.bc
#
# You can read more about replaying test cases here: http://klee.github.io/tutorials/testing-function/

import subprocess as sp
import sys
import os

klee_out_dir = sys.argv[-2]
bitcode = sys.argv[-1]

filename = os.path.splitext(bitcode)[0]
object_file = f'{filename}.o'
sp.run(f'clang -c {bitcode} -o {object_file}', shell=True)
sp.run(f'llvm-objcopy {object_file} --redefine-syms {klee_out_dir}/redefinitions.txt', shell=True)
clang_args = ' '.join(sys.argv[1:len(sys.argv) - 2])
print(f'clang {clang_args} {klee_out_dir}/externals.ll {object_file}')
sp.run(f'clang {clang_args} {klee_out_dir}/externals.ll {object_file}', shell=True)
