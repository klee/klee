// RUN: %clang -Wno-everything %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --optimize-aggressive=false --track-coverage=all --max-cycles=2 --optimize=true --emit-all-errors --delete-dead-loops=false --use-forked-solver=false -max-memory=6008 --cex-cache-validity-cores --only-output-states-covering-new=true --dump-states-on-halt=all --use-sym-size-alloc=true --symbolic-allocation-threshold=8192 %t1.bc 2>&1

// RUN: rm -f %t*.gcda %t*.gcno %t*.gcov
// RUN: %cc -DGCOV %s %libkleeruntest -Wl,-rpath %libkleeruntestdir -o %t_runner --coverage
// RUN: %replay %t.klee-out %t_runner
// RUN: gcov -b %t_runner-%basename_t > %t.cov.log

// RUN: FileCheck --input-file=%t.cov.log --check-prefix=CHECK-COV %s

// Branch coverage is greater 80%:
// CHECK-COV: Lines executed:9{{([0-9]\.[0-9][0-9])}}% of 24
// CHECK-COV-NEXT: Branches executed:100.00% of 16
// CHECK-COV-NEXT: Taken at least once:{{([8-9][0-9]\.[0-9][0-9])}}% of 16

#include "klee-test-comp.c"

extern void exit(int);
extern void abort(void);
#ifdef GCOV
extern void __gcov_dump(void);
#endif

void dump() {
#ifdef GCOV
  __gcov_dump();
  exit(0);
#endif
}

extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { dump(); __assert_fail("0", "matrix-2-2.c", 3, "reach_error"); }

void __VERIFIER_assert(int cond) {
  if (!(cond)) {
    ERROR: {reach_error();abort();}
  }
  return;
}
extern unsigned int __VERIFIER_nondet_uint();
extern int __VERIFIER_nondet_int();

int main()
{
  unsigned int N_LIN=__VERIFIER_nondet_uint();
  unsigned int N_COL=__VERIFIER_nondet_uint();
  if (N_LIN >= 4000000000 / sizeof(int) || N_COL >= 4000000000 / sizeof(int)) {
    return 0;
  }
  unsigned int j,k;
  int matriz[N_COL][N_LIN], maior;
  
  maior = __VERIFIER_nondet_int();
  for(j=0;j<N_COL;j++)
    for(k=0;k<N_LIN;k++)
    {       
       matriz[j][k] = __VERIFIER_nondet_int();
       
       if(matriz[j][k]>maior)
          maior = matriz[j][k];                          
    }                       
    
  for(j=0;j<N_COL;j++)
    for(k=0;k<N_LIN;k++)
      __VERIFIER_assert(matriz[j][k]<maior);    
}

