declare void @print_i32(i32)

define i32 @main() {
entry:
	%a = add i32 315904, 128
	%b = trunc i32 %a to i8
	%c0 = icmp eq i8 %b, 128
	%d = zext i8 %b to i32
	%c1 = icmp eq i32 %d, 128
	%e = sext i8 %b to i32
	%c2 = icmp eq i32 %e, -128
	%c0i = zext i1 %c0 to i32
	%c1i = zext i1 %c1 to i32
	%c2i = zext i1 %c2 to i32
	%c0is = shl i32 %c0i, 0
	%c1is = shl i32 %c1i, 1
	%c2is = shl i32 %c2i, 2
	%tmp = add i32 %c0is, %c1is
	%res = add i32 %tmp, %c2is
	%p = icmp eq i32 %res, 7
	br i1 %p, label %exitTrue, label %exitFalse
exitTrue:
	call void @print_i32(i32 1)
	ret i32 0
exitFalse:
	call void @print_i32(i32 0)
	ret i32 0
}
