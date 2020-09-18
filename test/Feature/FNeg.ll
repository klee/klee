; REQUIRES: geq-llvm-8.0
; RUN: %llvmas %s -o %t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error -output-dir=%t.klee-out -optimize=false %t.bc

define i32 @main() {
  %1 = fneg double 2.000000e-01
  %2 = fcmp oeq double %1, -2.000000e-01
  br i1 %2, label %success, label %fail

success:
  ret i32 0

fail:
  call void @abort()
  unreachable
}

declare void @abort() noreturn nounwind
