; RUN: rm -rf %t.klee-out
; RUN: llvm-as -f %s -o - | %klee --output-dir=%t.klee-out
; RUN: not test -f %t.klee-out/test0001.abort.err

declare void @klee_abort()

define i32 @foo(i8 signext %val) {
        %tmp = zext i8 %val to i32
	ret i32 %tmp
}

define i32 @main() {
	%res = call i32 bitcast (i32 (i8)* @foo to i32 (i1)*)( i1 1 )
        %check = icmp ne i32 %res, 255
        br i1 %check, label %error, label %exit

error:
        call void @klee_abort()
        unreachable

exit:
	ret i32 0
}
