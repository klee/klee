; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

%struct.anon = type { i8, [3 x i8] }

@z = global %struct.anon { i8 1, [3 x i8] undef }, align 4

define i32 @main() nounwind  {
entry:
  %retval = alloca i32, align 4
  store i32 0, i32* %retval
  ret i32 0
}
