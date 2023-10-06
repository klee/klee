/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * @description FORWARD_NULL, 如果字符串或者指针作为函数参数，为了防止空指针引用错误，在引用前必须确保该参数不为NULL，
 * 				如果上层调用者已经保证了该参数不可能为NULL，在调用本函数时，在函数开始处可以加ASSERT进行校验。
 *
 * @bad TestBad12;
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

#define BUFFERSIZE 100

/*
 * @mainScene 开始事件：指针解引用之前判空，结束事件：在函数中对可能为null的指针进行解引用
 * @subScene 指针来自动态分配，在为空路径下作为实参传入对其解引用的函数调用
 */
void TestBad12(int cond1, int cond2)
{
    // returned_null: "malloc" returns "NULL" (checked 1 out of 7 times)
    // var_assigned: Assigning: "buf" = "NULL" return value from "malloc"
    char *buf1 = (char *)malloc(BUFFERSIZE);
	char *buf2 = (char *)malloc(BUFFERSIZE);
	if (buf1 == NULL) {
		unsigned int i = 0;
		
		if(cond1 > 0 ){
			i = 1;
		}
		if(cond2 < 0){
			i = 2;
		}
		if((cond1+cond2) > 0){
			if(buf2 == NULL) {
				/* POTENTIAL FLAW: var_deref_model - 在该函数中对可能为null的指针进行解引用 */
				strcpy(buf2, "0"); // CHECK-DAG: KLEE: WARNING: 100.00% NullPointerException True Positive at trace 1
				free(buf2);
			}
			return;			
		}
		free(buf1);
	}
}

// RUN: %clang %s -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --use-guided-search=error --mock-external-calls --check-out-of-memory --libc=klee --skip-not-symbolic-objects --skip-not-lazy-initialized --use-lazy-initialization=only --analysis-reproduce=%s.json %t1.bc
// RUN: FileCheck -input-file=%t.klee-out/warnings.txt %s
