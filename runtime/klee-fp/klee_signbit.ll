;;===-- klee_signbit.ll ---------------------------------------------------===;;
;;
;;                     The KLEE Symbolic Virtual Machine
;;
;; This file is distributed under the University of Illinois Open Source
;; License. See LICENSE.TXT for details.
;;
;;===----------------------------------------------------------------------===;;
;target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
;target triple = "x86_64-unknown-linux-gnu"

define i32 @klee_internal_signbitf(float %f) #1 #0 {
entry:
  %0 = bitcast float %f to i32
  %1 = icmp slt i32 %0, 0
  %2 = zext i1 %1 to i32
  ret i32 %2
}

define i32 @klee_internal_signbit(double %d) #1 #0 {
entry:
  %0 = bitcast double %d to i64
  %1 = icmp slt i64 %0, 0
  %2 = zext i1 %1 to i32
  ret i32 %2
}

define i32 @klee_internal_signbitl(x86_fp80 %d) #0 {
entry:
  %0 = bitcast x86_fp80 %d to i80
  %1 = icmp slt i80 %0, 0
  %2 = zext i1 %1 to i32
  ret i32 %2
}

attributes #0 = { optnone noinline }
attributes #1 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
