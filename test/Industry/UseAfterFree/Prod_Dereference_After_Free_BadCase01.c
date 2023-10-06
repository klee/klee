/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
 * 
 * @description 资源释放后，对应的变量应该立即赋予新值，防止后续又被重新引用
 * 
 * @good
 * 
 * @bad UseAfterFree
 * 
 * @tool Coverity:USE_AFTER_FREE;SecBrella:SecB_UseAfterFree
 *
 * @cwe 416
 * 
 * @secureCoding 《华为C语言编程规范V5.1》 3.15-G.MEM.05 不要访问已释放的内存
 * 
 * @author x00407107
 * 
 */
#include <stdlib.h>

#define ARR_SIZE 10
// @scene 指针释放后直接解引用 
void UseAfterFree()
{
    /* POTENTIAL FLAW: 指针释放后未置空导致双重释放 */
    int *msg = (int *)malloc(ARR_SIZE * sizeof(int));
    if (msg == NULL) {
        return;
    }
    free(msg);
    /* uaf */
    *msg = 1; // CHECK: KLEE: WARNING: 100.00% UseAfterFree True Positive at trace 1
    return;
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
