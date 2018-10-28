// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=:oneint %t.bc 2>&1 | FileCheck --check-prefix=CHECK-EMPTY --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=oneint: %t.bc 2>&1 | FileCheck --check-prefix=CHECK-EMPTY --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=oneint:oneint %t.bc 2>&1 | FileCheck --check-prefix=CHECK-SAME --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=oneint:oneshort %t.bc 2>&1 | FileCheck --check-prefix=CHECK-TYPE-MISMATCH --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=oneint:twoints %t.bc 2>&1 | FileCheck --check-prefix=CHECK-TYPE-MISMATCH --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=twoints:twoshorts2int %t.bc 2>&1 | FileCheck --check-prefix=CHECK-TYPE-MISMATCH --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=oneint:unknownfunction %t.bc 2>&1 | FileCheck --check-prefix=CHECK-UNKNOWN --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=twoints:twointsmul %t.bc 2>&1 | FileCheck --check-prefix=CHECK-SUCCESS %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=xxx.*:somethingwithtwoints %t.bc 2>&1 | FileCheck --check-prefix=CHECK-REGEX-NO-MATCH --check-prefix=CHECK-ERROR %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=twoints.*:somethingwithtwoints %t.bc 2>&1 | FileCheck --check-prefix=CHECK-REGEX-PREFIX %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=.*twoints.*:twoints %t.bc 2>&1 | FileCheck --check-prefix=CHECK-REGEX-INFIX %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=two.*:twoints %t.bc 2>&1 | FileCheck --check-prefix=CHECK-REGEX-TYPE-MISMATCH %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=somethingwith.*:twoints -function-alias=twoints:twointsmul %t.bc 2>&1 | FileCheck --check-prefix=CHECK-MULTIPLE %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -output-module -function-alias=oneshort:shortminusone %t.bc 2>&1 | FileCheck --check-prefix=CHECK-BITCAST-ALIAS %s

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// CHECK-EMPTY: KLEE: ERROR: function-alias: {{name or pattern|replacement}} cannot be empty

// CHECK-SAME: KLEE: ERROR: function-alias: @oneint cannot replace itself

// CHECK-TYPE-MISMATCH: KLEE: WARNING: function-alias: @{{[a-z0-9]+}} could not be replaced with @{{[a-z0-9]+}}

// CHECK-SUCCESS: KLEE: function-alias: replaced @twoints with @twointsmul

// CHECK-REGEX-NO-MATCH: KLEE: ERROR: function-alias: no (replacable) match for 'xxx.*' found

// CHECK-REGEX-PREFIX: KLEE: function-alias: replaced @twoints (matching 'twoints.*') with @somethingwithtwoints
// CHECK-REGEX-PREFIX: KLEE: function-alias: replaced @twointsmul (matching 'twoints.*') with @somethingwithtwoints

// CHECK-REGEX-INFIX: KLEE: WARNING: function-alias: do not replace @twoints (matching '.*twoints.*') with @twoints: cannot replace itself
// CHECK-REGEX-INFIX: KLEE: function-alias: replaced @twointsmul (matching '.*twoints.*') with @twoints
// CHECK-REGEX-INFIX: KLEE: function-alias: replaced @somethingwithtwoints (matching '.*twoints.*') with @twoints

// CHECK-REGEX-TYPE-MISMATCH: KLEE: WARNING: function-alias: @twoshorts2int could not be replaced with @twoints: parameter types differ
// CHECK-REGEX-TYPE-MISMATCH: KLEE: function-alias: replaced @twointsmul (matching 'two.*') with @twoints

// CHECK-MULTIPLE: KLEE: function-alias: replaced @somethingwithtwoints (matching 'somethingwith.*') with @twoints
// CHECK-MULTIPLE: KLEE: function-alias: replaced @twoints with @twointsmul

// CHECK-BITCAST-ALIAS: function-alias: replaced @oneshort with @shortminusone

int __attribute__ ((noinline)) oneint(int a) { return a; }
short __attribute__ ((noinline)) oneshort(short a) { return a + 1; }
int __attribute__ ((noinline)) minusone(int a) { return a - 1; }
int __attribute__ ((noinline)) twoints(int a, int b) { return a + b; }
int __attribute__ ((noinline)) twoshorts2int(short a, short b) { return a + b + 2; }
int __attribute__ ((noinline)) twointsmul(int a, int b) { return a * b + 5; }
int __attribute__ ((noinline)) somethingwithtwoints(int a, int b) { return 2 * a + 3 * b; }

#ifdef __APPLE__
// FIXME: __attribute__((weak_import, alias("minusone"))) does not work on Mac OS X 10.12.6
short shortminusone(short a) { return minusone(a); }
#else
extern short shortminusone(short) __attribute__((alias("minusone")));
#endif

int main() {
  short a = 2;
  short b = 3;
  int result = 0;
  result += shortminusone(a);
  result += oneint(a);
  result += oneshort(a);
  result += twoints(a, b);
  result += twoshorts2int(a, b);
  result += twointsmul(a, b);
  result += somethingwithtwoints(a, b);
  // CHECK-ERROR-NOT: result =
  // CHECK-SUCCESS: result = 48
  // CHECK-REGEX-PREFIX: result = 52
  // CHECK-REGEX-INFIX: result = 28
  // CHECK-REGEX-TYPE-MISMATCH: result = 36
  // CHECK-MULTIPLE: result = 46
  // CHECK-BITCAST-ALIAS: 40
  printf("result = %d\n", result);
}
