; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %s

define dso_local i32 @main() local_unnamed_addr {
fma.float:
  %0 = call float @llvm.fma.f32(float 2.0, float 2.0, float 1.0)
  %cmp0 = fcmp one float %0, 5.0
  br i1 %cmp0, label %abort.block, label %fma.double

fma.double:
  %1 = call double @llvm.fma.f64(double 2.0, double 2.0, double 1.0)
  %cmp1 = fcmp one double %1, 5.0
  br i1 %cmp1, label %abort.block, label %fma.fp80

fma.fp80:
  %2 = call x86_fp80 @llvm.fma.f80(x86_fp80 0xK40008000000000000000,
                                   x86_fp80 0xK40008000000000000000,
                                   x86_fp80 0xK3FFF8000000000000000)
  %cmp2 = fcmp one x86_fp80 %2, 0xK4001A000000000000000
  br i1 %cmp2, label %abort.block, label %fmuladd.float

fmuladd.float:
  %3 = call float @llvm.fmuladd.f32(float 2.0, float 2.0, float 1.0)
  %cmp3 = fcmp one float %3, 5.0
  br i1 %cmp3, label %abort.block, label %fmuladd.double

fmuladd.double:
  %4 = call double @llvm.fmuladd.f64(double 2.0, double 2.0, double 1.0)
  %cmp4 = fcmp one double %4, 5.0
  br i1 %cmp4, label %abort.block, label %fmuladd.fp80

fmuladd.fp80:
  %5 = call x86_fp80 @llvm.fmuladd.f80(x86_fp80 0xK40008000000000000000,
                                       x86_fp80 0xK40008000000000000000,
                                       x86_fp80 0xK3FFF8000000000000000)
  %cmp5 = fcmp one x86_fp80 %5, 0xK4001A000000000000000
  br i1 %cmp4, label %abort.block, label %exit.block

exit.block:
  ret i32 0

abort.block:
  call void @abort()
  unreachable
}

declare void      @abort() noreturn nounwind

declare float     @llvm.fma.f32(float    %a, float    %b, float    %c)
declare double    @llvm.fma.f64(double   %a, double   %b, double   %c)
declare x86_fp80  @llvm.fma.f80(x86_fp80 %a, x86_fp80 %b, x86_fp80 %c)

declare float     @llvm.fmuladd.f32(float    %a, float    %b, float    %c)
declare double    @llvm.fmuladd.f64(double   %a, double   %b, double   %c)
declare x86_fp80  @llvm.fmuladd.f80(x86_fp80 %a, x86_fp80 %b, x86_fp80 %c)
