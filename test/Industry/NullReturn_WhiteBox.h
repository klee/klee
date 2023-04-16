/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: NULL_RETURNS规则公共头文件
 * Author: 徐西建 x00452482
 * Create: 2022-1-17
 */

#ifndef NULLRETURN_BADCASE_WHITEBOX_H
#define NULLRETURN_BADCASE_WHITEBOX_H

#include <stdio.h>
#include "base_type.h"

#define MAX_SCHED_USER_NUM_PER_TTI 64
#define MAX_USER_ID 32
#define MAX_SCH_INDEX 32
#define NULL_PTR NULL
#define min(a,b) ((a) < (b) ? (a) : (b))
#define IS_PTR_INVALID(x) ((x) == NULL_PTR)

typedef struct {
    UINT8 schedHarqNum;
    UINT16 schedUsrId[MAX_SCHED_USER_NUM_PER_TTI];
    UINT8 schIndex;
} SchedHarqStru;

typedef struct {
    int userType;
} SchrltStru;

typedef struct {
    SchrltStru *result;
} GlobalInfoStru;

typedef struct {
    UINT8 usrNum;
    SchrltStru usrList[MAX_USER_ID];
} PeriodSchUsrListStru;

#ifdef __cpluscplus
extern "C" {
#endif

#ifdef __cpluscplus
}
#endif
#endif