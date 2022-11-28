// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t.bc > %t.log
// RUN: %klee-stats --print-columns 'ICov(%),BCov(%)' --table-format=csv %t.klee-out > %t.stats
// RUN: FileCheck -check-prefix=CHECK-STATS -input-file=%t.stats %s
/*
 * Simple regular expression matching.
 *
 * From:
 *   The Practice of Programming
 *   Brian W. Kernighan, Rob Pike
 *
 */

#include "klee/klee.h"

static int matchhere(char *, char *);

static int matchstar(int c, char *re, char *text) {
  do {
    if (matchhere(re, text))
      return 1;
  } while (*text != '\0' && (*text++ == c || c == '.'));
  return 0;
}

static int matchhere(char *re, char *text) {
  if (re[0] == '\0')
    return 0;
  if (re[1] == '*')
    return matchstar(re[0], re + 2, text);
  if (re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if (*text != '\0' && (re[0] == '.' || re[0] == *text))
    return matchhere(re + 1, text + 1);
  return 0;
}

int match(char *re, char *text) {
  if (re[0] == '^')
    return matchhere(re + 1, text);
  do {
    if (matchhere(re, text))
      return 1;
  } while (*text++ != '\0');
  return 0;
}

#define SIZE 6

int main() {
  char re[SIZE];
  klee_make_symbolic(re, sizeof re, "re");
  match(re, "hello");
  return 0;
}
// Check that there 100% coverage
// CHECK-STATS: ICov(%),BCov(%)
// CHECK-STATS: 100.00,100.00