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
        buf[0] = 'a'; // CHECK-NUM: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
}                     // CHECK-UID: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 8389b1896658d867c9e15267acfe8c32

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --annotations=%annotations --mock-policy=all --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s -check-prefix=CHECK-NUM
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --annotations=%annotations --mock-policy=all --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --use-lazy-initialization=only --analysis-reproduce=%s.sarif %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s -check-prefix=CHECK-UID
