; REQUIRES: geq-llvm-12.0
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %s 2>&1 | FileCheck %s

; Check that IntrinsicCleaner strips out intrinsic
; CHECK-NOT: KLEE: WARNING ONCE: unsupported intrinsic llvm.experimental.noalias.scope.decl

; Check that Executor doesn't notice stripped intrinsic
; CHECK-NOT: KLEE: WARNING: unimplemented intrinsic: llvm.experimental.noalias.scope.decl
; CHECK-NOT: KLEE: ERROR: (location information missing) unimplemented intrinsic
; CHECK-NOT: KLEE: WARNING: unimplemented intrinsic: llvm.experimental.noalias.scope.decl

; Check that Executor explores all paths
; CHECK: KLEE: done: completed paths = 1
; CHECK: KLEE: done: partially completed paths = 0
; CHECK: KLEE: done: generated tests = 1



declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture, i64, i1) nounwind
declare void @llvm.experimental.noalias.scope.decl(metadata)
declare void @abort() noreturn nounwind

define void @test1(i8* %P, i8* %Q) nounwind {
  tail call void @llvm.experimental.noalias.scope.decl(metadata !0)
  tail call void @llvm.memcpy.p0i8.p0i8.i64(i8* %P, i8* %Q, i64 1, i1 false)
  ret void
}

define dso_local i32 @main() local_unnamed_addr {
  %1 = alloca i32, align 4
  %2 = alloca i8, align 1
  %3 = alloca i8, align 1
  store i32 0, i32* %1, align 4
  store i8 0, i8* %2, align 1
  store i8 1, i8* %3, align 1

  call void @test1(i8* %2, i8* %3)

  %4 = load i8, i8* %2, align 1
  %5 = sext i8 %4 to i32
  %6 = load i8, i8* %3, align 1
  %7 = sext i8 %6 to i32
  %8 = icmp eq i32 %5, %7
  br i1 %8, label %exit.block, label %abort.block

  exit.block:
    ret i32 0

  abort.block:
    call void @abort()
    unreachable
}

!0 = !{ !1 }
!1 = distinct !{ !1, !2, !"test1: var" }
!2 = distinct !{ !2, !"test1" }
