declare void @print_i32(i32)

define i32 @sum(i32 %a, i32 %b) {
	%c = sub i32 %a, %b
	ret i32 %c
}

define i32 @main() {
	%a = call i32 @sum(i32 54, i32 2)
	call void @print_i32(i32 %a)
	ret i32 0
}
