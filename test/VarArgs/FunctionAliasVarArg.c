// RUN: %clang %s -emit-llvm %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=printf:highlight_prefix_printf %t.bc 2>&1 | FileCheck --check-prefix=CHECK-HIGHLIGHT %s
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out -function-alias=printf:printf_format_only %t.bc 2>&1 | FileCheck --check-prefix=CHECK-VA-FAIL %s
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out -function-alias=printf:printf_format_only_with_varargs %t.bc 2>&1 | FileCheck --check-prefix=CHECK-VA-SUCCESS %s

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CHECK-HIGHLIGHT: KLEE: function-alias: replaced @printf with @highlight_prefix_printf
int highlight_prefix_printf(const char *format, ...) {
  va_list args;
  int result;
  const char *bold_start = "*+*";
  const char *bold_end = "*+*";

  va_start(args, format);
  const char *prefix = va_arg(args, const char *);

  const char *prefixless_format = strstr(format, "%s") + 2;
  size_t new_length = strlen(bold_start) + strlen(prefix) + strlen(bold_end) + strlen(prefixless_format) + 1;
  char *new_format = (char *)malloc(new_length);
  memset(new_format, 0, new_length);
  strcat(new_format, bold_start);
  strcat(new_format, prefix);
  strcat(new_format, bold_end);
  strcat(new_format, prefixless_format);
  result = vfprintf(stdout, new_format, args);
  va_end(args);

  free(new_format);
  return result;
}

// CHECK-VA-FAIL: KLEE: WARNING: function-alias: @printf could not be replaced with @printf_format_only: one has varargs while the other does not
int printf_format_only(const char *format) {
  int result;
  result = puts(format);
  return result;
}

// CHECK-VA-SUCCESS: KLEE: function-alias: replaced @printf with @printf_format_only_with_varargs
int printf_format_only_with_varargs(const char *format, ...) {
  int result;
  result = puts(format);
  return result;
}

int main() {
  int i = 42;
  // CHECK: KLEE: some warning about 42
  // CHECK-HIGHLIGHT: *+*KLEE*+*
  // CHECK-VA-SUCCESS: %s: some warning about %d
  printf("%s: some warning about %d\n", "KLEE", i);
}
