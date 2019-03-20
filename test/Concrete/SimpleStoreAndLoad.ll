; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

define i32 @main() {
entry:
	%a = alloca i32, i32 4
	%tmp1 = getelementptr i32, i32* %a, i32 0
	store i32 0, i32* %tmp1
	%tmp2 = load i32, i32* %tmp1
	%tmp3 = icmp eq i32 %tmp2, 0
	br i1 %tmp3, label %exitTrue, label %exitFalse
exitTrue:
	call void @print_i32(i32 1)
	ret i32 0
exitFalse:
	call void @print_i32(i32 0)
	ret i32 0
}
