; REQUIRES: geq-llvm-12.0
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %s
define dso_local i32 @main() local_unnamed_addr {
  smax:
  %0 = call i32 @llvm.smax.i32(i32 -10, i32 10)
  %cmp = icmp ne i32 %0, 10
  br i1 %cmp, label %abort.block, label %smin

  smin:
  %1 = call i32 @llvm.smin.i32(i32 -10, i32 10)
  %cmp1 = icmp ne i32 %1, -10
  br i1 %cmp1, label %abort.block, label %umax

  umax:
  %2 = call i32 @llvm.umax.i32(i32 10, i32 20)
  %cmp2 = icmp ne i32 %2, 20
  br i1 %cmp2, label %abort.block, label %umin

  umin:
  %3 = call i32 @llvm.umin.i32(i32 10, i32 5)
  %cmp3 = icmp ne i32 %3, 5
  br i1 %cmp3, label %abort.block, label %exit.block

  exit.block:
    ret i32 0

  abort.block:
    call void @abort()
    unreachable
}
declare void @abort() noreturn nounwind
declare i32 @llvm.smax.i32(i32, i32)
declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.umax.i32(i32, i32)
declare i32 @llvm.umin.i32(i32, i32)
