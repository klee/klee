; RUN: llvm-as %s -f -o %t1.bc
; RUN: %klee -disable-opt %t1.bc > %t2
; RUN: grep PASS %t2

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

declare {i8, i1} @llvm.uadd.with.overflow.i8(i8 %a, i8 %b)

declare i32 @puts(i8*)

@.passstr = private constant [5 x i8] c"PASS\00", align 1
@.failstr = private constant [5 x i8] c"FAIL\00", align 1

define i32 @main() {
bb0:
  %s0 = call {i8, i1} @llvm.uadd.with.overflow.i8(i8 0, i8 -1)
  %v0 = extractvalue {i8, i1} %s0, 0
  %c0 = icmp eq i8 %v0, -1
  br i1 %c0, label %bb0_1, label %bbfalse

bb0_1:
  %o0 = extractvalue {i8, i1} %s0, 1
  br i1 %o0, label %bbfalse, label %bb1

bb1:
  %s1 = call {i8, i1} @llvm.uadd.with.overflow.i8(i8 1, i8 -1)
  %v1 = extractvalue {i8, i1} %s1, 0
  %c1 = icmp eq i8 %v1, 0
  br i1 %c1, label %bb1_1, label %bbfalse

bb1_1:
  %o1 = extractvalue {i8, i1} %s1, 1
  br i1 %o1, label %bbtrue, label %bbfalse
  
bbtrue:
  %0 = call i32 @puts(i8* getelementptr inbounds ([5 x i8]* @.passstr, i64 0, i64 0)) nounwind
  ret i32 0

bbfalse:
  %1 = call i32 @puts(i8* getelementptr inbounds ([5 x i8]* @.failstr, i64 0, i64 0)) nounwind
  ret i32 0
}
