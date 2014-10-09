; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

define i1 @checkSlt() {
	%c0 = icmp slt i8 -1, 1		; 1
	%c1 = icmp slt i8 0, 1		; 1
	%c2 = icmp slt i8 1, 1		; 0
	%c3 = icmp slt i8 1, -1		; 0
	%c4 = icmp slt i8 1, 0		; 0
	%c5 = icmp slt i8 1, 1		; 0
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 3	; 0bin000011
	ret i1 %test
}

define i1 @checkSle() {
	%c0 = icmp sle i8 -1, 1		; 1
	%c1 = icmp sle i8 0, 1		; 1
	%c2 = icmp sle i8 1, 1		; 1
	%c3 = icmp sle i8 1, -1		; 0
	%c4 = icmp sle i8 1, 0		; 0
	%c5 = icmp sle i8 1, 1		; 1
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 39	; 0bin100111
	ret i1 %test
}

define i1 @checkSgt() {
	%c0 = icmp sgt i8 -1, 1		; 0
	%c1 = icmp sgt i8 0, 1		; 0
	%c2 = icmp sgt i8 1, 1		; 0
	%c3 = icmp sgt i8 1, -1		; 1
	%c4 = icmp sgt i8 1, 0		; 1
	%c5 = icmp sgt i8 1, 1		; 0
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 24	; 0bin011000
	ret i1 %test
}

define i1 @checkSge() {
	%c0 = icmp sge i8 -1, 1		; 0
	%c1 = icmp sge i8 0, 1		; 0
	%c2 = icmp sge i8 1, 1		; 1
	%c3 = icmp sge i8 1, -1		; 1
	%c4 = icmp sge i8 1, 0		; 1
	%c5 = icmp sge i8 1, 1		; 1
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 60	; 0bin111100
	ret i1 %test
}

define i1 @checkUlt() {
	%c0 = icmp ult i8 -1, 1		; 0
	%c1 = icmp ult i8 0, 1		; 1
	%c2 = icmp ult i8 1, 1		; 0
	%c3 = icmp ult i8 1, -1		; 1
	%c4 = icmp ult i8 1, 0		; 0
	%c5 = icmp ult i8 1, 1		; 0
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 10	; 0bin001010
	ret i1 %test
}

define i1 @checkUle() {
	%c0 = icmp ule i8 -1, 1		; 0
	%c1 = icmp ule i8 0, 1		; 1
	%c2 = icmp ule i8 1, 1		; 1
	%c3 = icmp ule i8 1, -1		; 1
	%c4 = icmp ule i8 1, 0		; 0
	%c5 = icmp ule i8 1, 1		; 1
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 46	; 0bin101110
	ret i1 %test
}

define i1 @checkUgt() {
	%c0 = icmp ugt i8 -1, 1		; 1
	%c1 = icmp ugt i8 0, 1		; 0
	%c2 = icmp ugt i8 1, 1		; 0
	%c3 = icmp ugt i8 1, -1		; 0
	%c4 = icmp ugt i8 1, 0		; 1
	%c5 = icmp ugt i8 1, 1		; 0
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 17	; 0bin010001
	ret i1 %test
}

define i1 @checkUge() {
	%c0 = icmp uge i8 -1, 1		; 1
	%c1 = icmp uge i8 0, 1		; 0
	%c2 = icmp uge i8 1, 1		; 1
	%c3 = icmp uge i8 1, -1		; 0
	%c4 = icmp uge i8 1, 0		; 1
	%c5 = icmp uge i8 1, 1		; 1
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%a3 = select i1 %c3, i8 8, i8 0
	%a4 = select i1 %c4, i8 16, i8 0
	%a5 = select i1 %c5, i8 32, i8 0
	%sum0 = add i8 %a0, %a1
	%sum1 = add i8 %sum0, %a2
	%sum2 = add i8 %sum1, %a3
	%sum3 = add i8 %sum2, %a4
	%sum = add i8 %sum3, %a5
	%test = icmp eq i8 %sum, 53	; 0bin110101
	ret i1 %test
}

define i1 @checkEq() {
	%c0 = icmp eq i8 -1, 1		; 0
	%c1 = icmp eq i8 1, 1		; 1
	%c2 = icmp eq i8 1, -1		; 0
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%sum0 = add i8 %a0, %a1
	%sum = add i8 %sum0, %a2
	%test = icmp eq i8 %sum, 2
	ret i1 %test
}

define i1 @checkNe() {
	%c0 = icmp ne i8 -1, 1		; 1
	%c1 = icmp ne i8 1, 1		; 0
	%c2 = icmp ne i8 1, -1		; 1
	%a0 = select i1 %c0, i8 1, i8 0
	%a1 = select i1 %c1, i8 2, i8 0
	%a2 = select i1 %c2, i8 4, i8 0
	%sum0 = add i8 %a0, %a1
	%sum = add i8 %sum0, %a2
	%test = icmp eq i8 %sum, 5
	ret i1 %test
}

define i32 @main() {
	%c0 = call i1 @checkSlt ()
	%c1 = call i1 @checkSle ()
	%c2 = call i1 @checkSgt ()
	%c3 = call i1 @checkSge ()
	%c4 = call i1 @checkUlt ()
	%c5 = call i1 @checkUle ()
	%c6 = call i1 @checkUgt ()
	%c7 = call i1 @checkUge ()
	%c8 = call i1 @checkEq ()
	%c9 = call i1 @checkNe ()
	%a0 = select i1 %c0, i16 1, i16 0
	%a1 = select i1 %c1, i16 2, i16 0
	%a2 = select i1 %c2, i16 4, i16 0
	%a3 = select i1 %c3, i16 8, i16 0
	%a4 = select i1 %c4, i16 16, i16 0
	%a5 = select i1 %c5, i16 32, i16 0
	%a6 = select i1 %c6, i16 64, i16 0
	%a7 = select i1 %c7, i16 128, i16 0
	%a8 = select i1 %c8, i16 256, i16 0
	%a9 = select i1 %c9, i16 512, i16 0
	%sum0 = add i16 %a0, %a1
	%sum1 = add i16 %sum0, %a2
	%sum2 = add i16 %sum1, %a3
	%sum3 = add i16 %sum2, %a4
	%sum4 = add i16 %sum3, %a5
	%sum5 = add i16 %sum4, %a6
	%sum6 = add i16 %sum5, %a7
	%sum7 = add i16 %sum6, %a8
	%sum8 = add i16 %sum7, %a9
	%t = shl i16 63, 10
	%sum = add i16 %sum8, %t
	%test = icmp eq i16 %sum, -1
	br i1 %test, label %exitTrue, label %exitFalse
exitTrue:
	call void @print_i32(i32 1)
	ret i32 0
exitFalse:
	call void @print_i32(i32 0)
	ret i32 0
}
