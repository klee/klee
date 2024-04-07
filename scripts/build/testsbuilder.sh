#!/bin/bash

set -e
set -u

ls -la

cd mytests/deadlockmark

clang++ -emit-llvm -O0 -c -g deadlock.cpp
klee deadlock.bc

cd ..
cd mytests/racecondmark

clang++ -emit-llvm -O0 -c -g racecond.cpp
klee deadlock.bc

cd ..
cd mytests/undefined_behaviour_mark

clang++ -emit-llvm -O0 -c -g undefined_behaviour.cpp
klee deadlock.bc