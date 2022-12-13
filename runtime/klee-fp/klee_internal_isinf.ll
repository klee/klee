;;===-- klee_internal_isinff.ll --------------------------------------------===;;
;;
;;                     The KLEE Symbolic Virtual Machine
;;
;; This file is distributed under the University of Illinois Open Source
;; License. See LICENSE.TXT for details.
;;
;;===----------------------------------------------------------------------===;;
;target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
;target triple = "x86_64-unknown-linux-gnu"

;; These are implementations of internal functions found in libm for classifying
;; floating point numbers. They have different names to avoid name collisions
;; during linking.
;;
;; These are implemented in IR to avoid generation of branching code.

declare zeroext i1 @klee_is_infinite_float(float) #2
declare zeroext i1 @klee_is_infinite_double(double) #2
declare zeroext i1 @klee_is_infinite_long_double(x86_fp80) #2

define i32 @klee_internal_isinff(float %f) #1 #0 {
entry:
  %isinf = tail call zeroext i1 @klee_is_infinite_float(float %f) #3
  %cmp = fcmp ogt float %f, 0.000000e+00
  %posOrNeg = select i1 %cmp, i32 1, i32 -1
  %result = select i1 %isinf, i32 %posOrNeg, i32 0
  ret i32 %result
}

define i32 @klee_internal_isinf(double %d) #1 #0 {
entry:
  %isinf = tail call zeroext i1 @klee_is_infinite_double(double %d) #3
  %cmp = fcmp ogt double %d, 0.000000e+00
  %posOrNeg = select i1 %cmp, i32 1, i32 -1
  %result = select i1 %isinf, i32 %posOrNeg, i32 0
  ret i32 %result
}

define i32 @klee_internal_isinfl(x86_fp80 %d) #0 {
entry:
  %isinf = tail call zeroext i1 @klee_is_infinite_long_double(x86_fp80 %d) #3
  %cmp = fcmp ogt x86_fp80 %d, 0xK00000000000000000000
  %posOrNeg = select i1 %cmp, i32 1, i32 -1
  %result = select i1 %isinf, i32 %posOrNeg, i32 0
  ret i32 %result
}



; NOTE: Use of optnone and noinline here are important so that the KLEE
; internal functions remain non-forking, even under optimization.
attributes #0 = { optnone noinline }

attributes #1 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin nounwind }
