; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

define i32 @main() {
	%ten = select i1 true, i32 10, i32 0
	%five = select i1 false, i32 0, i32 5
	%check = add i32 %ten, %five
	%test = icmp eq i32 %check, 15
	br i1 %test, label %exitTrue, label %exitFalse
exitTrue:
	call void @print_i32(i32 1)
	ret i32 0
exitFalse:
	call void @print_i32(i32 0)
	ret i32 0
}
