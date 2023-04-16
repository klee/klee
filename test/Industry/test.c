#include <stdio.h>

char *gDest;

char *HelpTestBad8(int len)
{
#if 0
        if (len > 0) {
                return gDest;
        }
#endif
        return NULL;
}

void TestBad8(int len)
{
        // char *buf = HelpTestBad8(len);
        char *buf = (char *)malloc(10);
        buf[0] = 'a'; // CHECK: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
}

// REQUIRES: z3
// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s