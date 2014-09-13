// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee %t2.bc > %t3.log
// RUN: echo "good" > %t3.good
// RUN: diff %t3.log %t3.good

int main() {
  char buf[4];
  char *s = "foo";

  klee_make_symbolic(buf, sizeof buf);
  buf[3] = 0;

  if (strcmp(buf, s)==0) {
    if (buf[0]=='f' && buf[1]=='o' && buf[2]=='o' && buf[3]==0) {
      printf("good\n");
    } else {
      printf("bad\n");
    }
  }
  
  return 0;
}
