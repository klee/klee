declare void @print_i32(i32)

define i8 @sum(i8 %a, i8 %b) {
	%c = add i8 %a, %b
	ret i8 %c
}

define i32 @main() {
	invoke i8 @sum(i8 1, i8 2) 
		to label %continue
		unwind label %error
continue:
	call void @print_i32(i32 1)
	ret i32 0
error:
	call void @print_i32(i32 0)
	ret i32 0
}
