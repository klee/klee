// RUN: %clang %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error %t1.bc

struct X;
extern struct X Y;
void *ptr = &Y;

int main()
{
	return 0;
}
