; REQUIRES: geq-llvm-3.9
; RUN: rm -rf %t.klee-out
; RUN: llvm-as -f %s -o - | %klee -optimize -output-dir=%t.klee-out | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@.str = private unnamed_addr constant [13 x i8] c"replacement\21\00", align 1
@stdout = external global %struct._IO_FILE*, align 8
@.str.1 = private unnamed_addr constant [9 x i8] c"original\00", align 1

@fwrite = alias i64 (i8*, i64, i64, %struct._IO_FILE*), i64 (i8*, i64, i64, %struct._IO_FILE*)* @__fwrite_alias

define i64 @__fwrite_alias(i8*, i64, i64, %struct._IO_FILE*) {
  %5 = call i32 @puts(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @.str, i32 0, i32 0))
  %6 = sext i32 %5 to i64
  ret i64 %6
}

define i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %2 = load %struct._IO_FILE*, %struct._IO_FILE** @stdout, align 8

; InstCombine should replace fprintf() with fwrite(), so in this example "replacement!" should be printed instead of "original"
; CHECK: replacement!
  %3 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %2, i8* getelementptr inbounds ([9 x i8], [9 x i8]* @.str.1, i32 0, i32 0))
  ret i32 0
}

declare i32 @fprintf(%struct._IO_FILE*, i8*, ...)
declare i32 @puts(i8*)
