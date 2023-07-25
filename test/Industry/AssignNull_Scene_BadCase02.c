/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description FORWARD_NULL, 如果字符串或者指针作为函数参数，为了防止空指针引用错误，在引用前必须确保该参数不为NULL，
 * 				如果上层调用者已经保证了该参数不可能为NULL，在调用本函数时，在函数开始处可以加ASSERT进行校验。
 *
 * @bad TestBad5;
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
//#include "securec.h"

struct TEACHER {
    unsigned int id;
    char *name;
};

struct STU {
    unsigned int id;
    char *name;
    struct TEACHER *teacher;
};

/*
 * @mainScene 开始事件：变量赋值NULL，结束事件：指针解引用运算
 * @subScene 指针是结构体类型，直接赋值为{0}，之后访问成员变量
 */
void TestBad5(struct STU *pdev, const char *buf, unsigned int count)
{
    struct TEACHER *tea;
    // 开始事件，assign_zero - 结构体变量被赋予 {0} 值
    struct STU *stu = 0;
    /* POTENTIAL FLAW: 结束事件，var_deref_op - null 指针解引用运算，访问成员变量 */
    tea = stu->teacher; // CHECK: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
    unsigned int teacherID = tea->id;
    printf("teacher id is %ud", teacherID);
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --location-accuracy --annotations=%annotations --mock-policy=all --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
// CHECK: KLEE: WARNING: 100.00% NullPointerException False Positive at trace 2
