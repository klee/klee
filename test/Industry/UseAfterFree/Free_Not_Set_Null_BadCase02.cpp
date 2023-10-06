/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * 
 * @description 指针释放后未置空
 *
 * @good 
 *
 * @bad test_bad_02
 *
 * @tool Coverity:USE_AFTER_FREE;SecBrella:SecB_UseAfterFree;
 *
 * @cwe 416
 * 
 * @secureCoding 《华为C语言编程规范V5.1》 3.15-G.MEM.05 不要访问已释放的内存
 * 
 * @author xwx356597;x00407107
 *
 */

#include <stdlib.h>
#include <stdio.h>

class STU{
public:
    STU();
    ~STU();

private:
    char *msg;
};

typedef struct {
    int a;
    char *pname;
}Data;

//全局变量 分支结束前要重新赋值
char *MSG = (char*)malloc(1000);

void VOS_Free(void* p){}

void func(void* p){}

// @scene 析构函数中释放内存后继续使用指针变量
STU::~STU()
{
    char *tmp;
    delete msg;
    /* uaf */
    tmp = msg;
}

// @scene 指针指向的内存释放之后,直接继续使用这个指针变量
void test_bad_02(int a)
{
    int num = 0;
    char *msg;
    char *tmp;
    /*POTENTIAL FLAW:msg is reuse after free*/
    msg = (char*)malloc(10);
    if (msg = NULL)
        return;
    if (a < 0)
    {
        free(msg);
        /*POTENTIAL FLAW:msg is not set NULL after free*/
        tmp = msg; //error
        return;  //分支结束
    }
    num = 10;
}

int main()
{
    return 0;
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s

// CHECK: KLEE: WARNING: 100.00% UseAfterFree False Positive at trace 1
