/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description 可能返回空指针的函数返回值，在解引用之前应该判空
 *
 * @bad TestBad3;
 *
 * @CWE 476;
 *
 * @secureCoding
 *
 * @csec
 *
 * @tool Coverity:NULL_RETURNS;SecBrella:SecB_NullReturns;
 *
 * @author m00489032
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "securec.h"

#define BUFFERSIZE 100

/*
 * @scene malloc函数调用结果未判空，通过指针别名在sprintf_s中解引用
 */
void TestBad3()
{
    // returned_null: "malloc" returns "NULL" (checked 1 out of 7 times)
    // var_assigned: Assigning: "buf" = "NULL" return value from "malloc"
    char *buf = (char *)malloc(BUFFERSIZE);
    // alias: Assigning: "bufNew" = "buf". Both pointer are now "NULL"
    char *bufNew = buf;
    /* POTENTIAL FLAW: dereference: Dereferencing a pointer that might be "NULL" "buf" when calling "strcpy" */
    strcpy(bufNew, "0"); // CHECK-DAG: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
    free(buf);
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --annotations=%annotations --mock-policy=all --check-out-of-memory --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
