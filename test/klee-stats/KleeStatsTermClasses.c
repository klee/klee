// RUN: %clang %s -fsanitize=unsigned-integer-overflow -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --max-stack-frames=15 --switch-type=simple --search=dfs --use-merge --ubsan-runtime --write-no-tests --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: %klee-stats --print-columns 'TermExit,TermEarly,TermSolverErr,TermProgrErr,TermUserErr,TermExecErr,TermEarlyAlgo,TermEarlyUser' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s


#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Winfinite-recursion"

// do not #include "klee/klee.h" to misuse API

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

const char *ro_str = "hi";
int global[3];


void max_depth(int a) {
  max_depth(a + 1); // Early (MaxDepth)
}

unsigned mul(unsigned a, unsigned b) {
  uint32_t r = a * b;
  return r;
}

void merge() {
  int cond;
  int a = 42;
  klee_make_symbolic(&cond, sizeof(cond), "merge_cond");

  klee_open_merge();
  if (cond == 8) a++;
  else a--;
  klee_close_merge(); // EarlyAlgorithm (Merge)
  // (normal) Exit
}

void bogus_external();


int main(void) {
  int cond;
  klee_make_symbolic(&cond, sizeof(cond), "cond");

  switch(cond) {
    case __LINE__:
      max_depth(17);
    case __LINE__:
      assert(0); // ProgErr (Assert)
      break;
    case __LINE__:
      abort(); // ProgErr (Abort)
    case __LINE__:
      free(global); // ProgErr (Free)
    case __LINE__: {
      unsigned a = UINT_MAX, b = UINT_MAX, c;
      c = mul(a, b); // ProgErr (Overflow)
    }
    case __LINE__:
      free((void*)0xdeadbeef); // ProgErr (Ptr)
      break;
    case __LINE__: {
      char *p = (char*)ro_str;
      p[0] = '!'; // ProgErr (ReadOnly)
    }
    case __LINE__:
      klee_report_error(__FILE__, __LINE__, "Report Error", "report.err"); // ProgErr (Report)
    case __LINE__:
      klee_make_symbolic(1); // UserErr
    case __LINE__:
      bogus_external(); // ExecErr (External)
    case __LINE__:
      merge();
      break;
    case __LINE__:
      klee_silent_exit(0); // EarlyUser
    default:
      return 0; // (normal) Exit
  }
}

// Check termination classes
// CHECK-STATS: TermExit,TermEarly,TermSolverErr,TermProgrErr,TermUserErr,TermExecErr,TermEarlyAlgo,TermEarlyUser
// CHECK-STATS: 2,1,0,7,1,1,1,1
