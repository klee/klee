; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

define i32 @main() {
	%amt = add i8 2, 5
	%a = shl i8 1, 5
	%b = lshr i8 %a, 5
	%c = shl i8 %b, %amt
	%d = lshr i8 %c, %amt
	%e = shl i8 %d, 7
	%f = ashr i8 %e, 7
	%g = shl i8 %f, %amt
	%h = ashr i8 %g, %amt
	%test = icmp eq i8 %h, -1
	br i1 %test, label %exitTrue, label %exitFalse
exitTrue:
	call void @print_i32(i32 1)
	ret i32 0
exitFalse:
	call void @print_i32(i32 0)
	ret i32 0
}
