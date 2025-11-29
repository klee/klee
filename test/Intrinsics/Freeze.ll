; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %s

; Checking support for removing 'freeze' instructions.
; Adapted from the example in the LLVM Language Reference Manual.

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define i32 @main() {
  ; scalar + freeze
  %x = freeze i32 undef
  %z = add i32 %x, %x            ; even number

  ; example with vectors
  %v = add <2 x i32> <i32 undef, i32 poison>, zeroinitializer
  %a = extractelement <2 x i32> %v, i32 0    ; undef
  %b = extractelement <2 x i32> %v, i32 1    ; poison
  %add = add i32 %a, %a                      ; undef

  %v.fr = freeze <2 x i32> %v                ; element-wise freeze
  %d = extractelement <2 x i32> %v.fr, i32 0 ; not undef
  %add.f = add i32 %d, %d                    ; even number

  ; branching on frozen value
  %i = add i1 undef, 0          ; undef
  %k = freeze i1 %i
  %poison = add nsw i1 %k, undef   ; poison
  %c = freeze i1 %poison
  br i1 %c, label %foo, label %bar ; non-deterministic branch to %foo or %bar

foo:
  ret i32 1

bar:
  ret i32 0
}
