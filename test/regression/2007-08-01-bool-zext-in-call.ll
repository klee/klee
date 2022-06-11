; RUN: rm -rf %t.klee-out
; RUN: %llvmas -f %s -o - | %klee --output-dir=%t.klee-out
; RUN: not test -f %t.klee-out/test0001.abort.err
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"
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
