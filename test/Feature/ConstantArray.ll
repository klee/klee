; RUN: llvm-as %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc 2>&1 | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [2 x i8] c"0\00", align 1
@.str2 = private unnamed_addr constant [2 x i8] c"1\00", align 1

%struct.dirent = type { i32, i32, i16, i8 }
declare void @klee_print_expr(i8*, ...)

define i32 @main() {
entry:
	%a = alloca %struct.dirent
	%tmp1 = getelementptr %struct.dirent, %struct.dirent* %a, i32 0
	%tmp2 = bitcast %struct.dirent* %tmp1 to <2 x i32>*
	; Initialize with constant vector
	store <2 x i32> <i32 42, i32 4096>, <2 x i32>* %tmp2
	br label %exit
exit:
	; Print first initialized element
	%idx = getelementptr %struct.dirent, %struct.dirent* %a, i32 0, i32 0
	%val = load i32, i32* %idx
	call void(i8*, ...) @klee_print_expr(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0), i32 %val)
	; CHECK: 0:42

	; Print second initialized element
	%idx2 = getelementptr %struct.dirent, %struct.dirent* %a, i32 0, i32 1
	%val2 = load i32, i32* %idx2
	call void(i8*, ...) @klee_print_expr(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str2, i32 0, i32 0), i32 %val2)
	; CHECK: 1:4096
	
	; Initialize with constant array
	%array = alloca [2 x i32];
	store [2 x i32][i32 7, i32 9], [2 x i32]* %array

	; Print first initialized element
	%idx3 = getelementptr [2 x i32], [2 x i32]* %array, i32 0, i32 0
	%val3 = load i32, i32* %idx3
	call void(i8*, ...) @klee_print_expr(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0), i32 %val3)
	; CHECK: 0:7

	; Print second initialized element
	%idx4 = getelementptr [2 x i32], [2 x i32]* %array, i32 0, i32 1
	%val4 = load i32, i32* %idx4
	call void(i8*, ...) @klee_print_expr(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str2, i32 0, i32 0), i32 %val4)
	; CHECK: 1:9
	
	ret i32 0
}
