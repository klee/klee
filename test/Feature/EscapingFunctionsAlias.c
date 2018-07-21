// Darwin does not support strong aliases.
// REQUIRES: not-darwin
// RUN: %llvmgcc -emit-llvm -O0 -g -c %s -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee -debug-print-escaping-functions --output-dir=%t.klee-out %t.bc 2> %t.log
// RUN: FileCheck --input-file=%t.log %s

void global_alias(void) __attribute__((alias("global_aliasee")));
void global_aliasee(void) {
    return;
}

short bitcast_of_alias(void) __attribute__((alias("bitcast_of_global_alias")));
short bitcast_of_global_alias(void) {
    return 1;
}

short bitcast_of_aliasee(void) __attribute__((alias("bitcast_of_global_aliasee")));
short bitcast_of_global_aliasee(void) {
    return 1;
}

int bitcast_in_global_alias(void) __attribute__((alias("bitcast_in_alias")));
short bitcast_in_alias(void) {
    return 1;
}

int main(int argc, char *argv[]) {
    global_aliasee();
    global_alias();

    int (*f1)(void) =(int (*)(void))bitcast_of_alias;
    f1();

    int (*f2)(void) =(int (*)(void))bitcast_of_global_aliasee;
    f2();

    bitcast_in_alias();
    bitcast_in_global_alias();

    // CHECK: KLEE: escaping functions: {{\[((bitcast_of_global_alias|bitcast_of_global_aliasee), ){2}\]}}
    return 0;
}
