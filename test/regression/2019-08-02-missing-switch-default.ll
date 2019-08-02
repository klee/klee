; REQUIRES: geq-llvm-3.8
; RUN: rm -rf %t.klee-out
; RUN: llvm-as -f %s -o %t.bc
; RUN: %klee --switch-type=internal --output-dir=%t.klee-out %t.bc
; RUN: FileCheck --input-file=%t.klee-out/info %s
; CHECK: KLEE: done: completed paths = 3

target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [5 x i8] c"cond\00", align 1

define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %4 = bitcast i32* %2 to i8*
  call void @klee_make_symbolic(i8* %4, i64 4, i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str, i32 0, i32 0))
  store i32 0, i32* %3, align 4
  %5 = load i32, i32* %2, align 4
  switch i32 %5, label %7 [
    i32 1, label %6
    i32 5, label %7
  ]

; <label>:6:
  store i32 1, i32* %3, align 4
  br label %8

; <label>:7:
  store i32 5, i32* %3, align 4
  br label %8

; <label>:8:
  %9 = load i32, i32* %2, align 4
  %10 = icmp eq i32 %9, 7
  br i1 %10, label %11, label %12

; <label>:11:
  store i32 7, i32* %3, align 4
  br label %12

; <label>:12:
  %13 = load i32, i32* %1, align 4
  ret i32 %13
}

declare void @klee_make_symbolic(i8*, i64, i8*)

attributes #0 = { noinline nounwind optnone uwtable }
