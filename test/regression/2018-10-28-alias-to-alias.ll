; RUN: rm -rf %t.klee-out
; RUN: llvm-as -f %s -o - | %klee --output-dir=%t.klee-out
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; @foo is not known yet
@foo2 = alias i32 (...), i32 (...)* @foo
@foo = alias i32 (...), bitcast (i32 ()* @__foo to i32 (...)*)

define i32 @__foo() {
entry:
  ret i32 42
}

define i32 @main() {
entry:
  call i32 (...) @foo()
  call i32 (...) @foo2()
  ret i32 0
}
