// RUN: %clang %s -emit-llvm -g -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --exit-on-error --write-xml-tests --xml-metadata-programfile=WriteXML.c --xml-metadata-programhash=0 %t2.bc
// RUN: test -f %t.klee-out/test000001.xml
// RUN: test -f %t.klee-out/test000002.xml
#include "klee/klee.h"
#include <stdio.h>

int main() {
  if (klee_range(0, 2, "range")) {
    printf("A\n");
  } else {
    printf("B\n");
  }
  return 0;
}
