; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

define i32 @main() {
entry:
	%a = add i32 0, 1
	br label %exit
exit:
	call void @print_i32(i32 %a)
	ret i32 0
}
