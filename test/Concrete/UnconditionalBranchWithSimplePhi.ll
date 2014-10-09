; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

define i32 @main() {
entry:
	%a = add i32 0, 1
	br label %exit
unused:
	%b = add i32 1, 2
	br label %exit
exit:
	%c = phi i32 [%a, %entry], [%b, %unused]
	call void @print_i32(i32 %c)
	ret i32 0
}
