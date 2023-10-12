// REQUIRES: uclibc
// REQUIRES: libcxx
// REQUIRES: eh-cxx
// RUN: %clangxx %s -emit-llvm %O0opt -std=c++11 -c %libcxx_includes -g -nostdinc++ -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --libcxx --libc=uclibc %t.bc 2>&1 | FileCheck %s

#include <cstdio>
#include <typeinfo>

extern bool _klee_eh_can_catch(const void *catcher, const void *exception);

class other {};
class yet_another {};
class ya_derived : private yet_another {};

class TestClass {
public:
  const char *classname = "TestClass";

  TestClass() { printf("%s: Normal constructor called\n", classname); }
  TestClass(const TestClass &other) {
    printf("%s: Copy constructor called\n", classname);
  }
  TestClass(TestClass &&other) {
    printf("%s: Move constructor called\n", classname);
  }
  TestClass &operator=(const TestClass &&other) {
    printf("%s: Assignment constructor called\n", classname);
    return *this;
  }
  virtual ~TestClass() { printf("%s: Destructor called\n", classname); }
};

class DerivedClass : public TestClass {
public:
  const char *classname = "DerivedClass";

  DerivedClass() { printf("%s: Normal constructor called\n", classname); }
  DerivedClass(const DerivedClass &other) {
    printf("%s: Copy constructor called\n", classname);
  }
  DerivedClass(DerivedClass &&other) {
    printf("%s: Move constructor called\n", classname);
  }
  DerivedClass &operator=(const DerivedClass &&other) {
    printf("%s: Assignment constructor called\n", classname);
    return *this;
  }
  ~DerivedClass() { printf("%s: Destructor called\n", classname); }
};

class SecondDerivedClass : public DerivedClass, other, yet_another {
public:
  const char *classname = "SecondDerivedClass";

  SecondDerivedClass() { printf("%s: Normal constructor called\n", classname); }
  SecondDerivedClass(const SecondDerivedClass &other) {
    printf("%s: Copy constructor called\n", classname);
  }
  SecondDerivedClass(SecondDerivedClass &&other) {
    printf("%s: Move constructor called\n", classname);
  }
  SecondDerivedClass &operator=(const SecondDerivedClass &&other) {
    printf("%s: Assignment constructor called\n", classname);
    return *this;
  }
  ~SecondDerivedClass() { printf("%s: Destructor called\n", classname); }
};

int main(int argc, char **args) {
  try {
    try {
      throw SecondDerivedClass();
    } catch (SecondDerivedClass &p) {
      puts("DerivedClass caught\n");
      // CHECK: DerivedClass caught
      throw;
    }
  } catch (TestClass &tc) {
    puts("TestClass caught\n");
    // CHECK: TestClass caught
  } catch (int) {
  } catch (int *) {
  } catch (ya_derived &) {
  }

  return 0;
}
