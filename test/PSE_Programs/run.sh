rm -rf klee* *.bc *.dot *.out
clang++ -I $HOME/klee/klee_src/include -c -emit-llvm -g -O1 -Xclang -disable-O0-optnone sample_example.cpp

klee sample_example.bc
ktest-tool klee-last/test000001.ktest

gcc -I $HOME/klee/klee_src/include/ -L $HOME/klee/klee_build/lib/ sample_example.cpp -lkleeRuntest
KTEST_FILE=klee-last/test000001.ktest ./a.out

echo $?