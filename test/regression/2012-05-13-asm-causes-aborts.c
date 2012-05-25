// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee %t1.bc

int main(int argc, char *argv[]){
	__asm__ __volatile__ ("movl %eax, %eax");
	return 0;
}

