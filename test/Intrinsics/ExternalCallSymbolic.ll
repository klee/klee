; RUN: %llvmas %s -o=%t1.bc
; RUN: %llvmas %s -o=%t2.bc
; RUN: rm -rf %t1.klee-out
; RUN: rm -rf %t2.klee-out
; RUN: %klee --external-calls=concrete --output-dir=%t1.klee-out --optimize=false %t1.bc 2>&1 | FileCheck %s -check-prefix=CHECK-EXTERNAL-CONCRETE --check-prefix=CHECK
; RUN: %klee --external-calls=all -exit-on-error --output-dir=%t2.klee-out --optimize=false %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-EXTERNAL-ALL --check-prefix=CHECK

; Check that IntrinsicCleaner notices missing intrinsic
; CHECK: KLEE: WARNING ONCE: unsupported intrinsic llvm.minnum.f32

; Check that Executor doesn't notice missing intrinsic
; CHECK-NOT: KLEE: WARNING: unimplemented intrinsic: llvm.minnum.f32
; CHECK-NOT: KLEE: ERROR: (location information missing) unimplemented intrinsic

; Check that Executor doesn't calls missing intrinsic as external
; when only external function calls with concrete arguments are allowed
; CHECK-EXTERNAL-CONCRETE: KLEE: ERROR: (location information missing) external call with symbolic argument: llvm.minnum.f32

; Check that Executor explores all paths
; CHECK-EXTERNAL-CONCRETE: KLEE: done: completed paths = 1
; CHECK-EXTERNAL-CONCRETE: KLEE: done: partially completed paths = 2
; CHECK-EXTERNAL-CONCRETE: KLEE: done: generated tests = 2


; Check that Executor calls missing intrinsic as external
; when all external function calls are allowed
; CHECK-EXTERNAL-ALL: KLEE: WARNING ONCE: calling external: llvm.minnum.f32

; Check that Executor explores all paths
; CHECK-EXTERNAL-ALL: KLEE: done: completed paths = 3
; CHECK-EXTERNAL-ALL: KLEE: done: partially completed paths = 0
; CHECK-EXTERNAL-ALL: KLEE: done: generated tests = 3



; This test checks that KLEE will call intrinsics that are not implemented
; when policy allows to do that
; It consists of a dead function that is called on one path but not on
; another path.
;
; The choice of which unimplemented intrinsic to use in this test is arbitrary
; and will need to be updated if the intrinsic is implemented.
;
; The dead function is called twice so that we can distinguish between
; KLEE executing both paths successfully and KLEE aborting on
; the last path it explores.

declare float @llvm.minnum.f32(float, float)

define void @dead() {
  %sym_addr = alloca float, align 4
  %cast_addr = bitcast float* %sym_addr to i8*
  call void @klee_make_symbolic(i8* %cast_addr, i64 4, i8* null)
  %sym_value = load float, float* %sym_addr, align 4
  %x = call float @llvm.minnum.f32(float 1.0, float %sym_value)
  ret void
}

declare void @klee_make_symbolic(i8*, i64, i8*)

define i32 @main() {
  %x1_addr = alloca i8, i8 1
  call void @klee_make_symbolic(i8* %x1_addr, i64 1, i8* null)
  %x1 = load i8, i8* %x1_addr

  %x2_addr = alloca i8, i8 1
  call void @klee_make_symbolic(i8* %x2_addr, i64 1, i8* null)
  %x2 = load i8, i8* %x2_addr

  %c1 = icmp eq i8 %x1, 0
  br i1 %c1, label %useDeadIntrinsic1, label %skip1
useDeadIntrinsic1:
  call void @dead()
  ret i32 1
skip1:

  %c2 = icmp eq i8 %x2, 0
  br i1 %c2, label %skip2, label %useDeadIntrinsic2
skip2:
  ret i32 0
useDeadIntrinsic2:
  call void @dead()
  ret i32 2
}
