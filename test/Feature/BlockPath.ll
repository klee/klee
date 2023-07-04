; REQUIRES: geq-llvm-12.0
; RUN: %llvmas %s -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --libc=klee --write-kpaths %t1.bc
; RUN: grep "(path: 0 (main: %0 %5 %6 %8 (abs: %1 %8 %11 %13) %8 %10 %16 %17 %19" %t.klee-out/test000001.kpath
; RUN: grep "(path: 0 (main: %0 %5 %6 %8 (abs: %1 %6 %11 %13) %8 %10 %16 %17 %19" %t.klee-out/test000002.kpath

@.str = private unnamed_addr constant [2 x i8] c"x\00", align 1

define i32 @abs(i32 %0) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  %4 = load i32, i32* %3, align 4
  %5 = icmp sge i32 %4, 0
  br i1 %5, label %6, label %8

6:                                                ; preds = %1
  %7 = load i32, i32* %3, align 4
  store i32 %7, i32* %2, align 4
  br label %11

8:                                                ; preds = %1
  %9 = load i32, i32* %3, align 4
  %10 = sub nsw i32 0, %9
  store i32 %10, i32* %2, align 4
  br label %11

11:                                               ; preds = %8, %6
  %12 = load i32, i32* %2, align 4
  br label %13

13:                                               ; preds = %11
  ret i32 %12
}

define i32 @main() {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %4 = bitcast i32* %2 to i8*
  br label %5

5:                                                ; preds = %0
  call void @klee_make_symbolic(i8* %4, i64 4, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i64 0, i64 0))
  br label %6

6:                                                ; preds = %5
  %7 = load i32, i32* %2, align 4
  br label %8

8:                                                ; preds = %6
  %9 = call i32 @abs(i32 %7) #4
  br label %10

10:                                               ; preds = %8
  store i32 %9, i32* %3, align 4
  %11 = load i32, i32* %3, align 4
  %12 = icmp ne i32 %11, -2147483648
  %13 = load i32, i32* %3, align 4
  %14 = icmp slt i32 %13, 0
  %or.cond = select i1 %12, i1 %14, i1 false
  br i1 %or.cond, label %15, label %16

15:                                               ; preds = %10
  store i32 1, i32* %1, align 4
  br label %17

16:                                               ; preds = %10
  store i32 0, i32* %1, align 4
  br label %17

17:                                               ; preds = %16, %15
  %18 = load i32, i32* %1, align 4
  br label %19

19:                                               ; preds = %17
  ret i32 %18
}

declare void @klee_make_symbolic(i8*, i64, i8*)
