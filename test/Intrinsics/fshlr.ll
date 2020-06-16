; Tests for the Funnel Shift intrinsics fshl/fshr(a, b, c) which
; - concatenate 'a' and 'b'
; - shift the result by an amount 'c' (left or right)
; - return the top or bottom half of result (depending on direction)
;
; This file consists of a sequence of tests of the form
; ```
;  T = llvm_fshl(a, b, c);
;  C = T != r;
;  if C {
;    klee_abort();
;  }
; ```
; where the constants a, b, c and r are copied from the constants
; used in the LLVM testfile llvm/test/Analysis/ConstantFolding/funnel-shift.ll

; REQUIRES: geq-llvm-7.0
; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %t.bc

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)
declare i7 @llvm.fshl.i7(i7, i7, i7)
declare i7 @llvm.fshr.i7(i7, i7, i7)

declare void @klee_abort()

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {

  ; T1: extract(concat(0x12345678, 0xABCDEF01) << 5) = 0x468ACF15
  %T1 = call i32 @llvm.fshl.i32(i32 305419896, i32 2882400001, i32 5)
  %C1 = icmp ne i32 %T1, 1183502101
  br i1 %C1, label %FAIL1, label %OK1
FAIL1:
  call void @klee_abort()
  unreachable
OK1:

  ; T2: extract(concat(0x12345678, 0xABCDEF01) >> 5) = 0xC55E6F78
  ; Try an oversized shift (37) to test modulo functionality.
  %T2 = call i32 @llvm.fshr.i32(i32 305419896, i32 2882400001, i32 37)
  %C2 = icmp ne i32 %T2, 3311300472
  br i1 %C2, label %FAIL2, label %OK2
FAIL2:
  call void @klee_abort()
  unreachable
OK2:

  ; T3: extract(concat(0b1110000, 0b1111111) << 2) = 0b1000011
  ; Use a weird type.
  ; Try an oversized shift (9) to test modulo functionality.
  %T3 = call i7 @llvm.fshl.i7(i7 112, i7 127, i7 9)
  %C3 = icmp ne i7 %T3, 67
  br i1 %C3, label %FAIL3, label %OK3
FAIL3:
  call void @klee_abort()
  unreachable
OK3:

  ; T4: extract(concat(0b1110000, 0b1111111) >> 2) = 0b0011111
  ; Try an oversized shift (16) to test modulo functionality.
  %T4 = call i7 @llvm.fshr.i7(i7 112, i7 127, i7 16)
  %C4 = icmp ne i7 %T4, 31
  br i1 %C4, label %FAIL4, label %OK4
FAIL4:
  call void @klee_abort()
  unreachable
OK4:

  ret i32 0
}
