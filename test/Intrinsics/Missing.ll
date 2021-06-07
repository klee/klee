; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t.bc 2>&1 | FileCheck %s

; Check that IntrinsicCleaner notices missing intrinsic
; CHECK: KLEE: WARNING ONCE: unsupported intrinsic llvm.minnum.f32

; Check that Executor notices missing intrinsic
; CHECK: KLEE: WARNING: unimplemented intrinsic: llvm.minnum.f32
; CHECK: KLEE: ERROR: (location information missing) unimplemented intrinsic
; CHECK: KLEE: WARNING: unimplemented intrinsic: llvm.minnum.f32

; Check that Executor explores all paths
; CHECK: KLEE: done: completed paths = 1
; CHECK: KLEE: done: partially completed paths = 2
; CHECK: KLEE: done: generated tests = 2



; This test checks that KLEE will ignore intrinsics that are not executed
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
  %x = call float @llvm.minnum.f32(float 1.0, float 2.0)
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
