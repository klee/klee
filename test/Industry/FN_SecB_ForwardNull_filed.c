/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * @description 空指针解引用 验收失败
 */
#include <stdio.h>
#include <stdlib.h>
#include "securec.h"
typedef unsigned int UINT32;
#define MAX_SHIFT_COUNT 64
#define OK 1
#define ERROR 0

typedef struct {
    UINT32 offset;
} StructInfo;

typedef struct {
    UINT32 dataLen;
    char *dataBuff;
} DataInfo;

typedef struct {
    UINT32 offset;
    UINT32 dataLen;
    char *dataBuff;
} SendDataInfo;

typedef struct {
    StructInfo *info;
} DataInfoStruct;

void WB_BadCase_Field(UINT32 inputNum1, UINT32 inputNum2)
{
  DataInfo dataBuffInfo = {0};
  dataBuffInfo.dataBuff = NULL;
  *dataBuffInfo.dataBuff = 'c';
}

void WB_BadCase_field2(DataInfo *data)
{
  data->dataBuff = NULL;
  *data->dataBuff = 'c'; // CHECK: KLEE: WARNING: 100.00% NullPointerException False Negative at: {{.*}}test/Industry/FN_SecB_ForwardNull_filed.c:42 19

  char *ptr = NULL;
  *ptr = 'c'; // CHECK: KLEE: WARNING: 100.00% NullPointerException False Positive at trace 1
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s