; REQUIRES: geq-llvm-17.0
; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --exit-on-error --optimize=false %t.bc

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @llvm.ptrmask.p0.i64(ptr, i64)
declare void @abort() noreturn nounwind

define i32 @main() {
entry:
  %ptr = inttoptr i64 4097 to ptr
  %masked = call ptr @llvm.ptrmask.p0.i64(ptr %ptr, i64 -2)
  %addr = ptrtoint ptr %masked to i64
  %ok = icmp eq i64 %addr, 4096
  br i1 %ok, label %exit, label %abort.block

exit:
  ret i32 0

abort.block:
  call void @abort()
  unreachable
}
