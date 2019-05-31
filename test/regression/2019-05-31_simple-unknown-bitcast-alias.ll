; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -output-source -output-dir=%t.klee-out %t.bc

; bitcast of @foo2, which is not known yet
@foo3 = alias i16 (...), bitcast (i32 (...)* @foo2 to i16 (...)*)
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
  call i16 (...) @foo3()
  ret i32 0
}
