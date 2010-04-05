// RUN: %llvmgcc %s -emit-llvm -O0 -c -o %t2.bc
// RUN: rm -f %t2.log
// RUN: %klee --stop-after-n-instructions=100 --optimize %t2.bc > %t3.log
// RUN: echo "good" > %t3.good
// RUN: diff %t3.log %t3.good

// should complete by 100 instructions if opt is on

int main() {
  int i, res = 0;

  for (i=1; i<=1000; i++)
    res += i;

  if (res == (1000*1001)/2) {
    printf("good\n");
  } else {
    printf("bad\n");
  }
  
  return 0;
}
