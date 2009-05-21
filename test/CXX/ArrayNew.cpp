// RUN: %llvmgxx %s --emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --no-output --exit-on-error --no-externals %t1.bc

#include <cassert>

static int decon = 0;

class Test {
  int x;

public:
  Test() {}
  Test(int _x) : x(_x) { }
  ~Test() { decon += x; }

  int getX() { return x; }
  void setX(int _x) { x = _x; }
};

int main(int argc) {
  Test *rt = new Test[4];
  int i;

  for (i=0; i<4; i++)
    rt[i].setX(i+1);

  int sum = 0;
  for (i=0; i<4; i++)
    sum += rt[i].getX();
  
  assert(sum==10);

  delete[] rt;

  assert(decon==10);

  return 0;
}
