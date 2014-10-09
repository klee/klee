; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

declare void @print_i1(i1)

define i32 @main() {
        %mem = alloca i1
        store i1 1, i1* %mem
        %v = load i1* %mem
        br i1 %v, label %ok, label %exit
ok:
	call void @print_i1(i1 %v)
        br label %exit
exit:
	ret i32 0
}
