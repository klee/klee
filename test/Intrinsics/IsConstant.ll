; REQUIRES: geq-llvm-8.0
; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %t.bc

declare i1 @llvm.is.constant.i32(i32)

declare i32 @klee_is_symbolic(i32)
declare void @klee_abort(i8*)

define i1 @check1() {
  %r = call i1 @llvm.is.constant.i32(i32 123)

  ret i1 %r
}

define i1 @check2() {
  %x = call i32 @klee_is_symbolic(i32 0)

  %r = call i1 @llvm.is.constant.i32(i32 %x)

  %notR = xor i1 %r, 1

  ret i1 %notR
}

define i32 @main() {
  %r1 = call i1 @check1()
  %r2 = call i1 @check2()

  br i1 %r1, label %next, label %exitFalse
next:
  br i1 %r2, label %exitTrue, label %exitFalse
exitFalse:
  call void @klee_abort(i8* null)
  ret i32 1
exitTrue:
  ret i32 0
}
