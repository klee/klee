// Darwin does not support section attribute: `argument to 'section' attribute is not valid for this target: mach-o section specifier requires a segment whose length is between 1 and 16 characters`
// REQUIRES: not-darwin
// RUN: %clang %s -g -emit-llvm %O0opt -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --mock-all-externals --write-xml-tests --output-dir=%t.klee-out %t1.bc
// RUN: FileCheck --input-file %t.klee-out/test000001.xml %s

extern void *__crc_mc44s803_attach __attribute__((__weak__));
static unsigned long const __kcrctab_mc44s803_attach __attribute__((__used__, __unused__,
                                                                    __section__("___kcrctab+mc44s803_attach"))) = (unsigned long const)((unsigned long)(&__crc_mc44s803_attach));

int main() {
  return 0;
}

// CHECK-NOT: <input