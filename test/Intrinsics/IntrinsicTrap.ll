; RUN: llvm-as %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc
; RUN: FileCheck %s --input-file=%t.klee-out/assembly.ll

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-f128:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind {
entry:
  %a = add i32 1, 2
  %b = add i32 %a, 3
  %c = icmp ne i32 %b, 6
  br i1 %c, label %btrue, label %bfalse

btrue:
  ; CHECK-NOT: call void @llvm.trap()
  ; CHECK: call void @abort()
  call void @llvm.trap() noreturn nounwind
  unreachable

bfalse:
  br label %return

return:
  ret i32 0
}

; CHECK-NOT: call void @llvm.trap()
; CHECK: declare void @abort()
declare void @llvm.trap() noreturn nounwind
