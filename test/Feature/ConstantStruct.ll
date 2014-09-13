; RUN: llvm-as %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out -disable-opt %t1.bc > %t2
; RUN: grep PASS %t2

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.sfoo = type { i32, i64 }

declare i32 @puts(i8*)
declare i32 @printf(i8*, ...)

@.passstr = private constant [5 x i8] c"PASS\00", align 1
@.failstr = private constant [5 x i8] c"FAIL\00", align 1

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind readnone {
entry:
  %f0 = extractvalue %struct.sfoo { i32 3, i64 1 }, 0
  %f1 = extractvalue %struct.sfoo { i32 3, i64 1 }, 1
  %xf0 = zext i32 %f0 to i64
  %f0mf1 = sub i64 %xf0, %f1
  %r = icmp eq i64 %f0mf1, 2
  br i1 %r, label %bbtrue, label %bbfalse

bbtrue:
  %0 = call i32 @puts(i8* getelementptr inbounds ([5 x i8]* @.passstr, i64 0, i64 0)) nounwind
  ret i32 0

bbfalse:
  %1 = call i32 @puts(i8* getelementptr inbounds ([5 x i8]* @.failstr, i64 0, i64 0)) nounwind
  ret i32 0
}
