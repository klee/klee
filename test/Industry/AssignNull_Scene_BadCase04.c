/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description FORWARD_NULL, 如果字符串或者指针作为函数参数，为了防止空指针引用错误，在引用前必须确保该参数不为NULL，
 * 				如果上层调用者已经保证了该参数不可能为NULL，在调用本函数时，在函数开始处可以加ASSERT进行校验。
 *
 * @bad TestBad7;
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

void *HelpTestBad7(void *dev, unsigned int count)
{
    if (count > 0) {
        dev = malloc(count);
        if (dev == NULL) {
            return NULL;
        }
        memset_s((char *)dev, count, 0, count);
    }
    return dev; // 这里必须返回入参，如果直接返回NULL，Coverity检查不出来
}

/*
 * @mainScene 开始事件：变量赋值NULL，结束事件：指针解引用运算
 * @subScene 通过函数出参可能返回NULL，解引用使用数组访问方式
 */
int TestBad7(char *arg, unsigned int count)
{
    // 开始事件，assign_zero - 变量被赋予 NULL 值
    char **dev = NULL;
    // 中间事件，identity_transfer - 函数的返回值为 NULL，因为函数的其中一个参数可能为 null 并且未经修改即返回。
    dev = HelpTestBad7(dev, count);
    /* POTENTIAL FLAW: 结束事件，var_deref_op - null 指针解引用运算，使用数组访问方式 */
    dev[0] = arg;  // 当前SecBrella漏报 // CHECK: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
    return 1;
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --check-out-of-memory --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --smart-resolve-entry-function --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
