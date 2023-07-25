/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description 可能返回空指针的函数返回值，在解引用之前应该判空
 *
 * @bad TestBad9;
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <new>
#include <memory>

/*
 * @mainScene 开始事件：new，结束事件：指针解引用运算
 * @subScene new一个int数组后，未校验直接解引用
 */
void TestBad9()
{
    // 开始事件，new返回NULL。到指针类型的动态转换可能是有意返回 NULL。
    // 中间事件，alias_transfer - 变量被赋予可能为 null 的指针
    int* p = new (std::nothrow) int[16];
    /* POTENTIAL FLAW: var_deref_op - null 指针解引用运算，new nothrow的返回值没有判空 */
	printf("the current integer is: %d", *p); // CHECK: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
}

// RUN: %clangxx %s -emit-llvm %O0opt -c -g -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-kqueries --use-guided-search=error --location-accuracy --annotations=%annotations --mock-policy=failed --check-out-of-memory --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
// CHECK: KLEE: WARNING: 100.00% NullPointerException False Positive at trace 2