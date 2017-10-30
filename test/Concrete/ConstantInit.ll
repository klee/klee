; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

%struct.dirent = type { i64, i64, i16, i8, [256 x i8] }
declare void @print_i64(i64)

define i32 @main() {
entry:
	%a = alloca %struct.dirent
	%tmp1 = getelementptr %struct.dirent* %a, i32 0
	%tmp2 = bitcast %struct.dirent* %tmp1 to <2 x i64>*
	; Initialize with constant vector
	store <2 x i64> <i64 0, i64 4096>, <2 x i64>* %tmp2, align 8
	br label %exit
exit:
	; Print first initialized element
	%idx = getelementptr %struct.dirent* %a, i32 0, i32 0
	%val = load i64* %idx	
	call void @print_i64(i64 %val)

	; Print second initialized element	
	%idx2 = getelementptr %struct.dirent* %a, i32 0, i32 1
	%val2 = load i64* %idx2	
	call void @print_i64(i64 %val2)
	
	ret i32 0
}
