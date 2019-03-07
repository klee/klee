// This test checks for the (in)correct use of --sym-files and --sym-args

// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out

// RUN: not %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime %t.bc --sym-files 1 10 --sym-files 2 10 &> %t1
// RUN: grep "Multiple --sym-files are not allowed" %t1
// RUN: rm -rf %t.klee-out

// RUN: not %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime %t.bc --sym-files 0 10 &> %t2
// RUN: grep "The first argument to --sym-files (number of files) cannot be 0" %t2
// RUN: rm -rf %t.klee-out

// RUN: not %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime %t.bc --sym-files 2 0 &> %t3
// RUN: grep "The second argument to --sym-files (file size) cannot be 0" %t3
// RUN: rm -rf %t.klee-out

// RUN: not %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime %t.bc --sym-args 3 2 10 &> %t4
// RUN: grep "Invalid range to --sym-args" %t4
// RUN: rm -rf %t.klee-out

// RUN: not %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime %t.bc --sym-args 0 0 10 &> %t5
// RUN: grep "Invalid range to --sym-args" %t5
// RUN: rm -rf %t.klee-out

// RUN: not %klee --output-dir=%t.klee-out --exit-on-error --posix-runtime %t.bc --sym-arg 5 --sym-args 0 99 5 --sym-arg 5 &> %t6
// RUN: grep "No more than 100 symbolic arguments allowed." %t6
// RUN: rm -rf %t.klee-out

int main(int argc, char** argv) {
  return 0;
}
