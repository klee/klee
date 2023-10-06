/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * @description 空指针解引用 验收失败
 *
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


void GetDataBuffInfo(DataInfo *dataBuffInfo, UINT32 inputNum)
{
    if (inputNum != MAX_SHIFT_COUNT) {
        dataBuffInfo->dataBuff = (char *)malloc(MAX_SHIFT_COUNT);
        if (dataBuffInfo->dataBuff == NULL) {
            return;
        }
        dataBuffInfo->dataLen = MAX_SHIFT_COUNT;
    }
}


void PrintBuf(SendDataInfo *dataBuffInfo)
{
    for (UINT32 i = 0; i < dataBuffInfo->dataLen; i++) {
        printf("buf data: %c", *(dataBuffInfo->dataBuff + i));
    }
}

void SaveBufInfo(DataInfo *dataBuffInfo)
{
    SendDataInfo *sendData = NULL;
    sendData = (SendDataInfo *)malloc(sizeof(SendDataInfo));
    if (sendData == NULL) {
        return;
    }

    sendData->offset = MAX_SHIFT_COUNT;
    sendData->dataBuff = dataBuffInfo->dataBuff;
    sendData->dataLen = dataBuffInfo->dataLen;
    PrintBuf(sendData);
    free(sendData);
}

void NotifyBuff(DataInfo *dataBuffInfo, UINT32 inputNum2)
{
    switch (inputNum2) {
        case MAX_SHIFT_COUNT:
            SaveBufInfo(dataBuffInfo);
            break;
        default:
            break;
    }

    if (dataBuffInfo->dataBuff != NULL) {
        free(dataBuffInfo->dataBuff);
    }
}
#if 0

DataInfo g_secInfo = { 0 };
UINT32 GetSecInfo(DataInfo *secInfo)
{
    if (secInfo == NULL) {
        return ERROR;
    }
    (void)memcpy_s(secInfo, sizeof(DataInfo), &g_secInfo, sizeof(DataInfo));
    return OK;
}


// FN: DTS2022061984858, https://portal.edevops.huawei.com/cloudreq-obp/project/gdf7ba016c8b34e3c80a19f8796b330f9/productSpace/bugList/2028184310/addbug/2029643728
void WB_BadCase01(DataInfoStruct *input, UINT32 flag)
{
    StructInfo *tempInfo = NULL;
    (void)memset_s(tempInfo, sizeof(StructInfo), 0, sizeof(StructInfo));
    if ((input == NULL) || (flag == 1)) {
        goto EXIT;
    }
    tempInfo = input->info;

EXIT:
    if (tempInfo->offset > 100) {
      return;
    }
    (void)memset_s(tempInfo, sizeof(StructInfo), 0, sizeof(StructInfo));
}
#endif

// FN: DTS2022061984853, https://portal.edevops.huawei.com/cloudreq-obp/project/gdf7ba016c8b34e3c80a19f8796b330f9/productSpace/bugList/2028184310/addbug/2029643723
void WB_BadCase02(UINT32 inputNum1, UINT32 inputNum2)
{
    DataInfo dataBuffInfo = {0};

    GetDataBuffInfo(&dataBuffInfo, inputNum1);
    NotifyBuff(&dataBuffInfo, inputNum2);
}

#if 0
// FN: DTS2022061984852, https://portal.edevops.huawei.com/cloudreq-obp/project/gdf7ba016c8b34e3c80a19f8796b330f9/productSpace/bugList/2028184310/addbug/2029643722
void WB_BadCase03(UINT32 *output, UINT32 *input, UINT32 flag)
{
    DataInfo secInfo = {0};
    char *data = NULL;
    UINT32 ret = GetSecInfo(&secInfo);
    if (ret == ERROR) {
        return;
    }
    data = secInfo.dataBuff;
    printf("data: %c", *data);
}

void badbad(char *ptr)
{
  ptr = NULL;
  *ptr = 'a';
}
#endif

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --check-out-of-memory --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
// CHECK: KLEE: WARNING: No targets found in error-guided mode