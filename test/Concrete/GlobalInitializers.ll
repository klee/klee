; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

%simple = type { i8, i16, i32, i64 }
@gInt = global i32 2
@gInts = global [2 x i32] [ i32 3, i32 5 ]
@gStruct = global %simple { i8 7, i16 11, i32 13, i64 17 }
@gZero = global %simple zeroinitializer

define i32 @main() {
entry:
	%addr0 = getelementptr i32, i32* @gInt, i32 0
	%addr1 = getelementptr [2 x i32], [2 x i32]* @gInts, i32 0, i32 0
	%addr2 = getelementptr [2 x i32], [2 x i32]* @gInts, i32 0, i32 1
	%addr3 = getelementptr %simple, %simple* @gStruct, i32 0, i32 0
	%addr4 = getelementptr %simple, %simple* @gStruct, i32 0, i32 1
	%addr5 = getelementptr %simple, %simple* @gStruct, i32 0, i32 2
	%addr6 = getelementptr %simple, %simple* @gStruct, i32 0, i32 3
	%addr7 = getelementptr %simple, %simple* @gZero, i32 0, i32 2
	%contents0 = load i32, i32* %addr0
	%contents1 = load i32, i32* %addr1
	%contents2 = load i32, i32* %addr2
	%contents3tmp = load i8, i8* %addr3
	%contents3 = zext i8 %contents3tmp to i32
	%contents4tmp = load i16, i16* %addr4
	%contents4 = zext i16 %contents4tmp to i32
	%contents5 = load i32, i32* %addr5
	%contents6tmp = load i64, i64* %addr6
	%contents6 = trunc i64 %contents6tmp to i32
	%contents7 = load i32, i32* %addr7
	%tmp0 = mul i32 %contents0, %contents1
	%tmp1 = mul i32 %tmp0, %contents2
	%tmp2 = mul i32 %tmp1, %contents3
	%tmp3 = mul i32 %tmp2, %contents4
	%tmp4 = mul i32 %tmp3, %contents5
	%tmp5 = mul i32 %tmp4, %contents6
	%tmp6 = add i32 %tmp5, %contents7
	%p = icmp eq i32 %tmp5, 510510
	br i1 %p, label %exitTrue, label %exitFalse
exitTrue:
	call void @print_i32(i32 1)
	ret i32 0
exitFalse:
	call void @print_i32(i32 0)
	ret i32 0
}
