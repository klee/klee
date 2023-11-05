// RUN: %clang  -Wno-everything %s -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --track-coverage=branches --delete-dead-loops=false --cex-cache-validity-cores --optimize=true --emit-all-errors --only-output-states-covering-new=true --dump-states-on-halt=true --search=dfs --search=random-path %t1.bc
// RUN: %klee-stats --print-columns 'ICov(%),BCov(%)' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-BRANCH -input-file=%t.stats %s

// Branch coverage 100%, and instruction coverage may vary:
// CHECK-BRANCH: ICov(%),BCov(%)
// CHECK-BRANCH-NEXT: {{([1-9][0-9]\.[0-9][0-9])}},100.00

// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --track-coverage=blocks --delete-dead-loops=false --cex-cache-validity-cores --optimize=true --emit-all-errors --only-output-states-covering-new=true --dump-states-on-halt=true --search=dfs --search=random-path %t1.bc
// RUN: %klee-stats --print-columns 'ICov(%),BCov(%)' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-BLOCK -input-file=%t.stats %s

// Branch coverage 100%, and instruction 100%:
// CHECK-BLOCK: ICov(%),BCov(%)
// CHECK-BLOCK-NEXT: 100.00,100.00


#include "klee-test-comp.c"

/*
 * Date: 30/09/2015
 * Created by: 
 *   Ton Chanh Le (chanhle@comp.nus.edu.sg) and
 *   Duc Muoi Tran (muoitranduc@gmail.com)
 */

extern int __VERIFIER_nondet_int();

typedef struct node {
  int val;
  struct node* next;
} node_t;

// Create a new linked list with length n when n >= 0
// or non-terminating when n < 0 
node_t* new_ll(int n)
{
  if (n == 0)
    return 0;
  node_t* head = malloc(sizeof(node_t));
  head->val = n;
  head->next = new_ll(n-1);
  return head;
}

int main ()
{
  int n = __VERIFIER_nondet_int();
  if (n < 0) {
      return 0;
  }
  node_t* head = new_ll(n);
  return 0;
}
