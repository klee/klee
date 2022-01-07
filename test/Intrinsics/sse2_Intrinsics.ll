; REQUIRES: geq-llvm-9.0
; RUN: llvm-as %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc 

; Function Attrs: noinline nounwind uwtable
define dso_local void @usub16x8_sat() #0 {
  %1 = alloca <2 x i64>, align 16
  %2 = alloca <2 x i64>, align 16
  %3 = alloca <2 x i64>, align 16
  %4 = alloca <2 x i64>, align 16
  %5 = alloca <2 x i64>, align 16
  %6 = load <2 x i64>, <2 x i64>* %4, align 16
  %7 = load <2 x i64>, <2 x i64>* %5, align 16
  store <2 x i64> %6, <2 x i64>* %1, align 16
  store <2 x i64> %7, <2 x i64>* %2, align 16
  %8 = load <2 x i64>, <2 x i64>* %1, align 16
  %9 = bitcast <2 x i64> %8 to <16 x i8>
  %10 = load <2 x i64>, <2 x i64>* %2, align 16
  %11 = bitcast <2 x i64> %10 to <16 x i8>
  %12 = call <16 x i8> @llvm.usub.sat.v16i8(<16 x i8> %9, <16 x i8> %11) #3
  %13 = bitcast <16 x i8> %12 to <2 x i64>
  store <2 x i64> %13, <2 x i64>* %3, align 16
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #1 {
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  call void @usub16x8_sat()
  ret i32 0
}

; Function Attrs: nounwind readnone speculatable
declare <16 x i8> @llvm.usub.sat.v16i8(<16 x i8>, <16 x i8>) #2
