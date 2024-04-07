#!/bin/bash
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/home/klee/.local/bin:$PATH"
pwd
echo $PATH
echo $(which klee)

set -e
set -u

cd /tmp/klee_build110stp_z3
make

cd ~/
cd /tmp/klee_src/mytests/deadlockmark

clang++ -emit-llvm -O0 -c -g deadlock.cpp
klee deadlock.bc

cd ~/
cd /tmp/klee_src/mytests/racecondmark

clang++ -emit-llvm -O0 -c -g racecond.cpp
klee racecond.bc

cd ~/
cd /tmp/klee_src/mytests/undefined_behaviour_mark

clang++ -emit-llvm -O0 -c -g undefined_behaviour.cpp
klee undefined_behaviour.bc
