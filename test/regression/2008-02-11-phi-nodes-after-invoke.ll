; RUN: llvm-as -f %s -o - | %klee --no-output --exit-on-error

declare void @klee_abort()

define i32 @foo(i32 %val, i32 %fail) {
        %code = icmp ne i32 0, %fail
        br i1 %code, label %failing, label %return
failing:   
        unwind        
return:        
	ret i32 %val
}

define void @test(i32 %should_fail) {
entry:  
	%res0 = invoke i32 (i32, i32)* @foo(i32 0, i32 %should_fail) 
                        to label %check_phi unwind label %error
        
error:
        %res1 = zext i8 1 to i32
        br label %check_phi

check_phi:
        %val = phi i32 [%never_used, %never_used_label], [%res0, %entry], [%res1, %error]
        %ok = icmp eq i32 %val, %should_fail
        br i1 %ok, label %exit, label %on_error
        call void @klee_abort()
        unreachable

on_error:
        call void @klee_abort()
        unreachable
            
exit:
	ret void

        ;; this is so we hopefully fail if incomingBBIndex isn't set properly
never_used_label:       
        %never_used = zext i8 undef to i32
        br label %check_phi
}

define i32 @main() {
        call void (i32)* @test(i32 0)
        call void (i32)* @test(i32 1)
	ret i32 0
}
