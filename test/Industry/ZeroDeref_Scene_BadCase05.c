/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description FORWARD_NULL, 如果字符串或者指针作为函数参数，为了防止空指针引用错误，在引用前必须确保该参数不为NULL，
 * 				如果上层调用者已经保证了该参数不可能为NULL，在调用本函数时，在函数开始处可以加ASSERT进行校验。
 *
 * @bad TestBad18;
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
 * @mainScene 开始事件：入参传入NULL
 * @subScene 函数内在不同的条件下对该参数解引用
 */
struct STU *HelpBadTest2();

void HelpBadTest1(struct STU *stu)
{
	struct STU *localStu = HelpBadTest2();
	if (localStu == NULL) {
		localStu = stu;
		// sink: deref
		localStu->id;
	}
	// sink: deref
	localStu->id;
	if (localStu == NULL) {
		return;
	}
	unsigned int id = localStu->id;
}

void TestBad18(struct STU *stu)
{
	// POTENTIAL FLAW: transfer NULL
	HelpBadTest1(NULL);
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --check-out-of-memory --annotations=%annotations --mock-policy=all --libc=klee --external-calls=all --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
// CHECK: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 2
// CHECK: KLEE: WARNING: 100.00% NullPointerException False Positive at trace 1
