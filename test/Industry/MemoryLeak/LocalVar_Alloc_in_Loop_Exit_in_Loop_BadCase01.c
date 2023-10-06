/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description RESOURCE_LEAK 查找程序没有尽快释放系统资源的情况
 *
 * @good
 *
 * @bad ResourceLeak_bad01
 *
 * @tool Coverity:RESOURCE_LEAK;SecBrella:SecB_MemoryLeak;
 * 
 *@CWE 404
 *
 * @author r00369861
 *
 */

#include <stdio.h>
#include <stdlib.h>

#define MEM_SIZE 10

// @scene 循环中本地分配的内存，在条件分支中提前返回，返回前没有释放内存
int ResourceLeak_bad01(int c)
{
    /* POTENTIAL FLAW: "ppParamId" and "ppParamId[0] - ppParamId[i-1]" is leaked */
    char **ppParamId = (char **)malloc(MEM_SIZE * sizeof(char *));
    if (ppParamId == NULL) {
        return -1;
    }

    int i = 0;
    for (; i < MEM_SIZE; i++) {
        ppParamId[i] = (char *)malloc(sizeof(char));
        if (ppParamId[i] == NULL) {
            /* leak here: "ppParamId" and "ppParamId[0] - ppParamId[i-1]" is leaked */
            return -1;
        }
    }

    /* do something */
    int j = 0;
    for (; j < MEM_SIZE; j++) {
        free(ppParamId[j]);
    }

    free(ppParamId);
    return 0;
}

void call_func(int num)
{
    ResourceLeak_bad01(num);
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-kqueries --max-cycles-before-stuck=0 --use-guided-search=error --check-out-of-memory --mock-external-calls --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s

// CHECK: KLEE: WARNING: 100.00% Reachable Reachable at trace 1
