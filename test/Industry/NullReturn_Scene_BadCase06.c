/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description 可能返回空指针的函数返回值，在解引用之前应该判空
 *
 * @bad TestBad6;
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

char *HelpTestBad6(char *dev, unsigned int count)
{
    if (count > 0) {
        int ret = memset_s(dev, count, 0, count);
        if (ret != 0) {
            return dev;
        }
    }
    // return_param: Returning parameter "dev"
    return dev;
}

/*
 * @scene malloc函数调用结果未判空，作为入参传入函数调用未作修改直接返回，通过数组下表访问解引用
 */
void TestBad6(unsigned int count)
{
    // returned_null: "malloc" returns "NULL" (checked 1 out of 7 times)
    // var_assigned: Assigning: "buf" = "NULL" return value from "malloc"
    char *buf = (char *)malloc(BUFFERSIZE);
    // identity_transfer: Passing "buf" as argument 1 to function "HelpTestBad7", which returns that argument
    buf = HelpTestBad6(buf, count);
    /* POTENTIAL FLAW: dereference: Dereferencing "buf", which is known to be "NULL" */
    buf[0] = 'a';
    free(buf);
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --annotations=%annotations --mock-policy=all --check-out-of-memory --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s

// CHECK-DAG: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
// CHECK-DAG: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 2
