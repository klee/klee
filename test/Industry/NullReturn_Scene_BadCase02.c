/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description 可能返回空指针的函数返回值，在解引用之前应该判空
 *
 * @bad TestBad2;
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

/*
 * @scene localtime函数调用结果未判空，访问成员解引用
 */
void TestBad2()
{
    time_t rawtime;

    time(&rawtime);
    // returned_null: "localtime" returns "NULL" (checked 0 out of 1 times)
    // var_assigned: Assigning: "info" = "NULL" return value from "localtime"
    struct tm *info = localtime(&rawtime);
    /* POTENTIAL FLAW: dereference: Dereferencing "info", which is known to be "NULL" */
    printf("The second is %d", info->tm_sec); // CHECK: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --location-accuracy --annotations=%annotations --mock-policy=all  --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
