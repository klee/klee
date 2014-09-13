// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out %t1.bc

int main(int argc, char *argv[]){
	__asm__ __volatile__ ("movl %eax, %eax");
	return 0;
}

