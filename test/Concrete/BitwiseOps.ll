declare void @print_i32(i32)

define i32 @main() {
	%a = or i32 12345678, 87654321
	%b = and i32 %a, 87654321
	%check = xor i32 %b, 87654321
	%test = icmp eq i32 %check, 0
	br i1 %test, label %exitTrue, label %exitFalse
exitTrue:
	call void @print_i32(i32 1)
	ret i32 0
exitFalse:
	call void @print_i32(i32 0)
	ret i32 0
}
