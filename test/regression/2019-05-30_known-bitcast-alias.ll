; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error-type=Abort -output-source -output-dir=%t.klee-out %t.bc

@x = global i32 0, align 4

define i32 @__foo() {
entry:
  %0 = load i32, i32* @x, align 4
  %inc = add nsw i32 %0, 1
  store i32 %inc, i32* @x, align 4
  ret i32 1
}

@foo = alias i32 (...), bitcast (i32 ()* @__foo to i32 (...)*)
@foo2 = alias i32 (...), i32 (...)* @foo
@foo3 = alias i16 (...), bitcast (i32 (...)* @foo2 to i16 (...)*)

define i32 @main() {
entry:
  %call1 = call i32 (...) @foo()
  %call2 = call i32 (...) @foo2()
  %call3 = call i16 (...) @foo3()
  %load = load i32, i32* @x, align 4
  %inc1 = add nsw i32 %load, %call1
  %inc2 = add nsw i32 %inc1, %call2
  %zext = zext i16 %call3 to i32
  %inc3 = add nsw i32 %inc2, %zext
  %cmp = icmp ne i32 %inc3, 6
  br i1 %cmp, label %if.then, label %if.end

if.then:
  call void @klee_abort()
  unreachable

if.end:
  ret i32 0
}

declare void @klee_abort() #1

attributes #1 = { noreturn }
