/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * @description 可能返回空指针的函数返回值，在解引用之前应该判空
 *
 * @bad NullReturn_BadCase_WhiteBox01;
 *
 * @CWE 476;
 *
 * @secureCoding
 *
 * @csec
 *
 * @tool Coverity:NULL_RETURNS;SecBrella:SecB_NullReturns;
 *
 * @author x00452482  验收失败
 */

#include "NullReturn_WhiteBox.h"

#define IS_USRID_INVALID(usrId) ((usrId) >= MAX_USER_ID)
#define IS_ULSCH_SCHINDEX_INVALID(schIndex) ((schIndex) >= MAX_SCH_INDEX)

GlobalInfoStru *g_ulschUsrInfo;

static inline GlobalInfoStru *GetUserInst(UINT16 usrId, UINT32 schIndex)
{
    if (IS_USRID_INVALID(usrId) || IS_ULSCH_SCHINDEX_INVALID(schIndex)) {
        return NULL_PTR;
    }

    UINT32 index = schIndex + usrId;
    return &g_ulschUsrInfo[index];
}

static inline SchrltStru *GetResult(UINT16 usrId, UINT32 schIndex)
{
    GlobalInfoStru *usrGlbInfo = GetUserInst(usrId, schIndex);
    if (IS_PTR_INVALID(usrGlbInfo)) {
        return NULL_PTR;                                                                                // (1)
    }

    return usrGlbInfo->result;
}

void SendMsg(UINT8 index, UINT16 usrId, SchrltStru *resultInfo)
{
    if ((index + usrId) > MAX_SCHED_USER_NUM_PER_TTI) {
        resultInfo->userType = 1; // CHECK-DAG: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 2
    } else {
        resultInfo->userType = 0; // CHECK-DAG: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
    }
}

void NullReturn_BadCase_WhiteBox01(UINT8 index, SchedHarqStru *harqInfo)
{
    UINT32 schedUserNum = min(harqInfo->schedHarqNum, MAX_SCHED_USER_NUM_PER_TTI);

    for (UINT32 schedUserIndex = 0; schedUserIndex < schedUserNum; schedUserIndex++) {
        UINT16 usrId = harqInfo->schedUsrId[schedUserIndex];
        SchrltStru *resultInfo = GetResult(usrId, harqInfo->schIndex);                                  // (2)
        // POTENTIAL FLAW: 指针未判空，直接下后续流程中进行解引用处理，引发空指针异常
        SendMsg(index, usrId, resultInfo);                                                              // (3)
    }
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --smart-resolve-entry-function --use-guided-search=error --annotations=%annotations --mock-policy=all --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
