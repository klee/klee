// RUN: %llvmgcc -emit-llvm -O0 -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -debug-print-escaping-functions --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: FileCheck --input-file=%t.log %s

int functionpointer(void) {
    return 1;
}

int functionpointer_as_argument(void) {
    return 2;
}

short bitcasted_functionpointer(void) {
    return 3;
}

int receives_functionpointer(int (*f)(void));

int blockaddress(int x) {
    void * target = &&one;
    switch (x) {
        case 1: break;
        case 2:
            target = &&two;
            goto *target;
        default:
            goto *target;
    }
one:
    return 1;
two:
    return 2;
}

int main(int argc, char *argv[]) {
    int (*f1)(void) = functionpointer;
    f1();

    receives_functionpointer(functionpointer_as_argument);

    int (*f2)(void) =(int (*)(void))bitcasted_functionpointer;
    f2();

    blockaddress(argc);

    // CHECK: KLEE: escaping functions: {{\[((functionpointer|functionpointer_as_argument|bitcasted_functionpointer), ){3}\]}}
    return 0;
}
