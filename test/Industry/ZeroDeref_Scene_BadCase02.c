/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description FORWARD_NULL, 如果字符串或者指针作为函数参数，为了防止空指针引用错误，在引用前必须确保该参数不为NULL，
 * 				如果上层调用者已经保证了该参数不可能为NULL，在调用本函数时，在函数开始处可以加ASSERT进行校验。
 *
 * @bad TestBad9;
 *
 * @CWE 476;
 *
 * @secureCoding
 *
 * @csec
 *
 * @tool Coverity:FORWARD_NULL;SecBrella:SecB_ForwardNull;
 *
 * @author xwx356597; m00489032
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "securec.h"

#define BUFFERSIZE 100

char pDest[BUFFERSIZE] = {0};

/*
 * @mainScene 针对NULL常量的检查，需要开启coverity扫描参数FORWARD_NULL:deref_zero_errors:true
 * @subScene 危险函数中直接使用NULL常量
 */
void TestBad9()
{
    char *p = NULL;
    /* POTENTIAL FLAW: var_deref_model - 在该函数中对可能为null的指针进行解引用 */
    memcpy(pDest, p, BUFFERSIZE); // CHECK-DAG: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --check-out-of-memory --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
