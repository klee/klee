; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i32(i32)

@anInt = global i32 1
@aRef = global i32* @anInt

define i32 @main() {
        call void @print_i32(i32 0)
	ret i32 0
}
