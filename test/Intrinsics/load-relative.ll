; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out %t.bc

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@target = internal constant [17 x i8] c"load-relative-ok\00", align 1
@table = internal constant [2 x i32] [
  i32 trunc (i64 sub (i64 ptrtoint ([17 x i8]* @target to i64), i64 ptrtoint ([2 x i32]* @table to i64)) to i32),
  i32 trunc (i64 sub (i64 ptrtoint ([17 x i8]* @target to i64), i64 ptrtoint ([2 x i32]* @table to i64)) to i32)
], align 4

declare i8* @llvm.load.relative.i64(i8*, i64)
declare void @klee_abort(i8*)

define i32 @main() {
entry:
  %base = bitcast [2 x i32]* @table to i8*
  %ptr = call i8* @llvm.load.relative.i64(i8* %base, i64 4)
  %value = load i8, i8* %ptr, align 1
  %ok = icmp eq i8 %value, 108
  br i1 %ok, label %done, label %abort

abort:
  call void @klee_abort(i8* null)
  ret i32 1

done:
  ret i32 0
}
