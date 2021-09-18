; LLVM has an intrinsic for abs.
; This file is based on the following code with poisoning of llvm.abs disabled.
; ```
; #include "klee/klee.h"
;
; #include <assert.h>
; #include <limits.h>
;
; volatile int abs_a;
;
; int main(void)
; {
;   int a;
;   klee_make_symbolic(&a, sizeof(a), "a");
;
;   abs_a = a < 0 ? -a : a;
;   if (abs_a == INT_MIN)
;     assert(abs_a == a);
;   else
;     assert(abs_a >= 0);
;   return abs_a;
; }
; ```
; REQUIRES: geq-llvm-12.0
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %s

@0 = private unnamed_addr constant [2 x i8] c"a\00", align 1
define dso_local i32 @main() local_unnamed_addr {
  %1 = alloca i32, align 4
  %2 = bitcast i32* %1 to i8*
  call void @klee_make_symbolic(i8* nonnull %2, i64 4, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @0, i64 0, i64 0))
  %3 = load i32, i32* %1, align 4
  ; Call with is_int_min_poison == 0 (false), expects INT_MIN to be returned
  %abs_a = call i32 @llvm.abs.i32(i32 %3, i1 false)
  %4 = icmp eq i32 %abs_a, -2147483648
  br i1 %4, label %int_min, label %not_int_min

int_min:
  %5 = icmp eq i32 %abs_a, %3
  br i1 %5, label %exit.block, label %abort.block

not_int_min:
  %6 = icmp sgt i32 %abs_a, -1
  br i1 %6, label %exit.block, label %abort.block

exit.block:
  ret i32 0

abort.block:
  call void @abort()
  unreachable
}

declare dso_local void @klee_make_symbolic(i8*, i64, i8*)
declare i32 @llvm.abs.i32(i32, i1 immarg)
declare void @abort() noreturn nounwind
