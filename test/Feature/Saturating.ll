; REQUIRES: geq-llvm-8.0
; RUN: llvm-as %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc | FileCheck %s

declare i8 @llvm.uadd.sat.i8(i8 %a, i8 %b)
declare i8 @llvm.usub.sat.i8(i8 %a, i8 %b)
declare i8 @llvm.sadd.sat.i8(i8 %a, i8 %b)
declare i8 @llvm.ssub.sat.i8(i8 %a, i8 %b)
declare <2 x i8> @llvm.ssub.sat.v2i8(<2 x i8> %a, <2 x i8> %b)
declare i4 @llvm.sadd.sat.i4(i4 %a, i4 %b)
declare i4 @llvm.ssub.sat.i4(i4 %a, i4 %b)

declare i32 @puts(i8*)

@.passstr = private constant [5 x i8] c"PASS\00", align 1
@.failstr = private constant [5 x i8] c"FAIL\00", align 1

define i32 @main() {
bb0:
  %v0 = call i8 @llvm.uadd.sat.i8(i8 1, i8 253)
  %c0 = icmp eq i8 %v0, 254
  br i1 %c0, label %bb1, label %bbfalse
bb1:
  %v1 = call i8 @llvm.uadd.sat.i8(i8 1, i8 255)
  %c1 = icmp eq i8 %v1, 255
  br i1 %c1, label %bb2, label %bbfalse
bb2:
  %v2 = call i8 @llvm.usub.sat.i8(i8 5, i8 3)
  %c2 = icmp eq i8 %v2, 2
  br i1 %c2, label %bb3, label %bbfalse
bb3:
  %v3 = call i8 @llvm.usub.sat.i8(i8 3, i8 5)
  %c3 = icmp eq i8 %v3, 0
  br i1 %c3, label %bb4, label %bbfalse
bb4:
  %v4 = call i8 @llvm.sadd.sat.i8(i8 120, i8 3)
  %c4 = icmp eq i8 %v4, 123
  br i1 %c4, label %bb5, label %bbfalse
bb5:
  %v5 = call i8 @llvm.sadd.sat.i8(i8 120, i8 10)
  %c5 = icmp eq i8 %v5, 127
  br i1 %c5, label %bb6, label %bbfalse
bb6:
  %v6 = call i8 @llvm.ssub.sat.i8(i8 20, i8 70)
  %c6 = icmp eq i8 %v6, -50
  br i1 %c6, label %bb7, label %bbfalse
bb7:
  %v7 = call i8 @llvm.ssub.sat.i8(i8 -10, i8 120)
  %c7 = icmp eq i8 %v7, -128
  br i1 %c7, label %bb8, label %bbfalse
bb8:
  %v8 = call <2 x i8> @llvm.ssub.sat.v2i8(<2 x i8> <i8 -20, i8 -50>, <2 x i8> <i8 110, i8 20>)
  %v8_0 = extractelement <2 x i8> %v8, i32 0
  %v8_1 = extractelement <2 x i8> %v8, i32 1
  %c8_0 = icmp eq i8 %v8_0, -128
  %c8_1 = icmp eq i8 %v8_1, -70
  %c8 = and i1 %c8_0, %c8_1
  br i1 %c8, label %bb9, label %bbfalse
bb9:
  %v9 = call i4 @llvm.sadd.sat.i4(i4 -4, i4 -5)
  %c9 = icmp eq i4 %v9, -8
  br i1 %c9, label %bb10, label %bbfalse
bb10:
  %v10 = call i4 @llvm.ssub.sat.i4(i4 4, i4 -5)
  %c10 = icmp eq i4 %v10, 7
  br i1 %c10, label %bbtrue, label %bbfalse

bbtrue:
; CHECK: PASS
  %0 = call i32 @puts(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.passstr, i64 0, i64 0)) nounwind
  ret i32 0

bbfalse:
  %1 = call i32 @puts(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.failstr, i64 0, i64 0)) nounwind
  ret i32 0
}
