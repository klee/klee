// RUN: %clangxx %s -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --write-no-tests --exit-on-error --external-calls=none %t1.bc

#include <cassert>

static int decon = 0;

class Thing {
public:
  Thing() {}
  virtual ~Thing() { decon += getX(); }

  virtual int getX() { return 1; };
};

class Thing2 : public Thing {
public:
  virtual int getX() { return 2; };
};

Thing *getThing(bool which) { 
  return which ? new Thing() : new Thing2();
}

int main(int argc) {
  Thing *one = getThing(false);
  Thing *two = getThing(true);

  int x = one->getX() + two->getX();
  assert(x==3);

  delete two;
  delete one;
  
  assert(decon==2);

  return 0;
}
