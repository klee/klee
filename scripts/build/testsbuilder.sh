#!/bin/bash
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/home/klee/.local/bin:$PATH"
ls -la /tmp
pwd
echo $PATH
echo $(which klee)

set -e
set -u

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
