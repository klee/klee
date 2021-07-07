; REQUIRES: geq-llvm-12.0
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %s 2> %t.stderr.log
; RUN: FileCheck %s < %t.stderr.log

define i32 @main() {
  %1 = call i32 @llvm.abs.i32(i32 -2147483648, i1 true)
  ; CHECK: llvm.abs called with poison and INT_MIN
  ret i32 0
}

declare i32 @llvm.abs.i32(i32, i1 immarg)
