/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * 
 * @description 资源释放后，对应的变量应该立即赋予新值，防止后续又被重新引用
 * 
 * @good 
 * 
 * @bad DoubleFreeBad02
 * 
 * @tool Coverity:USE_AFTER_FREE;SecBrella:SecB_UseAfterFree
 *
 * @cwe 416
 * 
 * @secureCoding 《华为C语言编程规范V5.1》 3.15-G.MEM.05 不要访问已释放的内存
 * 
 * @author xwx356597;x00407107
 * 
 */

#include <stdio.h>

//@scene 指针释放后未置空，有条件地再再次释放导致双重释放
void DoubleFreeBad02(int flag)
{
    /* POTENTIAL FLAW: 指针释放后未置空导致双重释放 */
    char *p = (char *)malloc(100);
    if(p == NULL)
        return;
    
    /* do something */
    
    /* 第一次释放 */
    free(p);
    
    if(flag == 0)
    {
        /* double free */
        free(p); // CHECK: KLEE: WARNING: 100.00% DoubleFree True Positive at trace 1
    }
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
