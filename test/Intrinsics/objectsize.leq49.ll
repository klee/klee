; REQUIRES: lt-llvm-5.0
; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %t.bc
; ModuleID = 'objectsize.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind uwtable {
entry:
  %a = alloca i8*, align 8
  %0 = load i8*, i8** %a, align 8
  %1 = call i64 @llvm.objectsize.i64.p0i8(i8* %0, i1 true)
  %cmp = icmp ne i64 %1, 0
  br i1 %cmp, label %abort.block, label %continue.block

continue.block:
  %2 = load i8*, i8** %a, align 8
  %3 = call i64 @llvm.objectsize.i64.p0i8(i8* %2, i1 false)
  %cmp1 = icmp ne i64 %3, -1
  br i1 %cmp1, label %abort.block, label %exit.block

exit.block:
  ret i32 0

abort.block:
  call void @abort()
  unreachable
}

declare i64 @llvm.objectsize.i64.p0i8(i8*, i1) nounwind readnone

declare void @abort() noreturn nounwind
