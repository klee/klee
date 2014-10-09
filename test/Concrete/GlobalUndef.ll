; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

target datalayout =
"e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.anon = type { i8, [3 x i8] }

@z = global %struct.anon { i8 1, [3 x i8] undef }, align 4

define i32 @main() nounwind  {
entry:
  %retval = alloca i32, align 4
  store i32 0, i32* %retval
  ret i32 0
}
