; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

%struct.dirent = type { i64, i64, i16, i8 }
declare void @print_i64(i64)

define void @test_constant_vector_simple() {
  entry:
	%a = alloca %struct.dirent
	%tmp1 = getelementptr %struct.dirent, %struct.dirent* %a, i32 0
	%tmp2 = bitcast %struct.dirent* %tmp1 to <2 x i64>*
	; Initialize with constant vector parsed as ConstantDataSequential
	store <2 x i64> <i64 0, i64 4096>, <2 x i64>* %tmp2, align 8

	br label %exit
exit:
	; Print first initialized element
	%idx = getelementptr %struct.dirent, %struct.dirent* %a, i32 0, i32 0
	%val = load i64, i64* %idx
	call void @print_i64(i64 %val)

	; Print second initialized element
	%idx2 = getelementptr %struct.dirent, %struct.dirent* %a, i32 0, i32 1
	%val2 = load i64, i64* %idx2
	call void @print_i64(i64 %val2)
	ret void
}

define void @test_constant_vector_complex() {
entry:
    ; Create a vector using an uncommon type: i1024: Force usage of constant vector
	%a = alloca <2 x i1024>
	store <2 x i1024> <i1024 5, i1024 1024>, <2 x i1024>* %a, align 8

	br label %exit
exit:
	; Print first initialized element
	%idx = getelementptr <2 x i1024>, <2 x i1024>* %a, i32 0, i32 0
	%narrow = bitcast i1024* %idx to i64*
	%val = load i64, i64* %narrow
	call void @print_i64(i64 %val)

	; Print second initialized element
	%idx2 = getelementptr <2 x i1024>, <2 x i1024>* %a, i32 0, i32 1
	%narrow2 = bitcast i1024* %idx2 to i64*
	%val2 = load i64, i64* %narrow2
	call void @print_i64(i64 %val2)

	ret void
}

define i32 @main() {
entry:
	call void @test_constant_vector_simple()
	call void @test_constant_vector_complex()

	br label %exit
exit:
	ret i32 0
}
