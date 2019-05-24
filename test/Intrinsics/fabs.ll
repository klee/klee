; LLVM has an intrinsic for fabs.
; This file is generated from the following code:
; ```
;#include <math.h>
;
;int main(void) {
;  float f = -1.;
;  f = fabs(f);
;
;  if(f != 1.)
;    return 1;
;
;  double d = -2.;
;  d = fabs(d);
;
;  if(d != 2.)
;    return 2;
;
;  return 0;
;}
; ```
; To clean the resulting llvm-ir up for LLVM-versions < 6,
; I ran `opt -S -strip -strip-debug -strip-named-metadata'
; on the resulting ir. Additionally I removed the 'speculatable'
; attribute where it appeared. 
;
; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %t.bc
; ModuleID = 'fabs.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca float, align 4
  %3 = alloca double, align 8
  store i32 0, i32* %1, align 4
  store float -1.000000e+00, float* %2, align 4
  %4 = load float, float* %2, align 4
  %5 = fpext float %4 to double
  %6 = call double @llvm.fabs.f64(double %5)
  %7 = fptrunc double %6 to float
  store float %7, float* %2, align 4
  %8 = load float, float* %2, align 4
  %9 = fpext float %8 to double
  %10 = fcmp une double %9, 1.000000e+00
  br i1 %10, label %11, label %12

; <label>:11:                                     ; preds = %0
  store i32 1, i32* %1, align 4
  br label %19

; <label>:12:                                     ; preds = %0
  store double -2.000000e+00, double* %3, align 8
  %13 = load double, double* %3, align 8
  %14 = call double @llvm.fabs.f64(double %13)
  store double %14, double* %3, align 8
  %15 = load double, double* %3, align 8
  %16 = fcmp une double %15, 2.000000e+00
  br i1 %16, label %17, label %18

; <label>:17:                                     ; preds = %12
  store i32 2, i32* %1, align 4
  br label %19

; <label>:18:                                     ; preds = %12
  store i32 0, i32* %1, align 4
  br label %19

; <label>:19:                                     ; preds = %18, %17, %11
  %20 = load i32, i32* %1, align 4
  ret i32 %20
}

; Function Attrs: nounwind readnone
declare double @llvm.fabs.f64(double) #1

attributes #0 = { noinline nounwind optnone sspstrong uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
