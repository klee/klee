; REQUIRES: geq-llvm-9.0
; RUN: llvm-as %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc

; ModuleID = 'sse2_Intrinsics.c'
source_filename = "sse2_Intrinsics.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [53 x i8] c"%s: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\0A\00", align 1
@.str.1 = private unnamed_addr constant [29 x i8] c"%s: %x %x %x %x %x %x %x %x\0A\00", align 1
@.str.2 = private unnamed_addr constant [17 x i8] c"%s: %x %x %x %x\0A\00", align 1
@.str.3 = private unnamed_addr constant [15 x i8] c"%s: %llx %llx\0A\00", align 1
@.str.4 = private unnamed_addr constant [14 x i8] c"_mm_subs_epu8\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @print128_num_16x8(i8*, <2 x i64>) #0 !dbg !19 {
  %3 = alloca i8*, align 8
  %4 = alloca <2 x i64>, align 16
  %5 = alloca [16 x i8], align 16
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !23, metadata !DIExpression()), !dbg !24
  store <2 x i64> %1, <2 x i64>* %4, align 16
  call void @llvm.dbg.declare(metadata <2 x i64>* %4, metadata !25, metadata !DIExpression()), !dbg !26
  call void @llvm.dbg.declare(metadata [16 x i8]* %5, metadata !27, metadata !DIExpression()), !dbg !29
  %6 = bitcast [16 x i8]* %5 to i8*, !dbg !29
  call void @llvm.memset.p0i8.i64(i8* align 16 %6, i8 0, i64 16, i1 false), !dbg !29
  %7 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 0, !dbg !30
  %8 = bitcast <2 x i64>* %4 to i8*, !dbg !30
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %7, i8* align 16 %8, i64 16, i1 false), !dbg !30
  %9 = load i8*, i8** %3, align 8, !dbg !31
  %10 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 0, !dbg !32
  %11 = load i8, i8* %10, align 16, !dbg !32
  %12 = sext i8 %11 to i32, !dbg !32
  %13 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 1, !dbg !33
  %14 = load i8, i8* %13, align 1, !dbg !33
  %15 = sext i8 %14 to i32, !dbg !33
  %16 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 2, !dbg !34
  %17 = load i8, i8* %16, align 2, !dbg !34
  %18 = sext i8 %17 to i32, !dbg !34
  %19 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 3, !dbg !35
  %20 = load i8, i8* %19, align 1, !dbg !35
  %21 = sext i8 %20 to i32, !dbg !35
  %22 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 4, !dbg !36
  %23 = load i8, i8* %22, align 4, !dbg !36
  %24 = sext i8 %23 to i32, !dbg !36
  %25 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 5, !dbg !37
  %26 = load i8, i8* %25, align 1, !dbg !37
  %27 = sext i8 %26 to i32, !dbg !37
  %28 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 6, !dbg !38
  %29 = load i8, i8* %28, align 2, !dbg !38
  %30 = sext i8 %29 to i32, !dbg !38
  %31 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 7, !dbg !39
  %32 = load i8, i8* %31, align 1, !dbg !39
  %33 = sext i8 %32 to i32, !dbg !39
  %34 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 8, !dbg !40
  %35 = load i8, i8* %34, align 8, !dbg !40
  %36 = sext i8 %35 to i32, !dbg !40
  %37 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 9, !dbg !41
  %38 = load i8, i8* %37, align 1, !dbg !41
  %39 = sext i8 %38 to i32, !dbg !41
  %40 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 10, !dbg !42
  %41 = load i8, i8* %40, align 2, !dbg !42
  %42 = sext i8 %41 to i32, !dbg !42
  %43 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 11, !dbg !43
  %44 = load i8, i8* %43, align 1, !dbg !43
  %45 = sext i8 %44 to i32, !dbg !43
  %46 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 12, !dbg !44
  %47 = load i8, i8* %46, align 4, !dbg !44
  %48 = sext i8 %47 to i32, !dbg !44
  %49 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 13, !dbg !45
  %50 = load i8, i8* %49, align 1, !dbg !45
  %51 = sext i8 %50 to i32, !dbg !45
  %52 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 14, !dbg !46
  %53 = load i8, i8* %52, align 2, !dbg !46
  %54 = sext i8 %53 to i32, !dbg !46
  %55 = getelementptr inbounds [16 x i8], [16 x i8]* %5, i64 0, i64 15, !dbg !47
  %56 = load i8, i8* %55, align 1, !dbg !47
  %57 = sext i8 %56 to i32, !dbg !47
  %58 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([53 x i8], [53 x i8]* @.str, i64 0, i64 0), i8* %9, i32 %12, i32 %15, i32 %18, i32 %21, i32 %24, i32 %27, i32 %30, i32 %33, i32 %36, i32 %39, i32 %42, i32 %45, i32 %48, i32 %51, i32 %54, i32 %57), !dbg !48
  ret void, !dbg !49
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #2

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1 immarg) #2

declare dso_local i32 @printf(i8*, ...) #3

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @print128_num_8x16(i8*, <2 x i64>) #0 !dbg !50 {
  %3 = alloca i8*, align 8
  %4 = alloca <2 x i64>, align 16
  %5 = alloca [8 x i16], align 16
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !51, metadata !DIExpression()), !dbg !52
  store <2 x i64> %1, <2 x i64>* %4, align 16
  call void @llvm.dbg.declare(metadata <2 x i64>* %4, metadata !53, metadata !DIExpression()), !dbg !54
  call void @llvm.dbg.declare(metadata [8 x i16]* %5, metadata !55, metadata !DIExpression()), !dbg !60
  %6 = bitcast [8 x i16]* %5 to i8*, !dbg !60
  call void @llvm.memset.p0i8.i64(i8* align 16 %6, i8 0, i64 16, i1 false), !dbg !60
  %7 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 0, !dbg !61
  %8 = bitcast i16* %7 to i8*, !dbg !61
  %9 = bitcast <2 x i64>* %4 to i8*, !dbg !61
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %8, i8* align 16 %9, i64 16, i1 false), !dbg !61
  %10 = load i8*, i8** %3, align 8, !dbg !62
  %11 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 0, !dbg !63
  %12 = load i16, i16* %11, align 16, !dbg !63
  %13 = sext i16 %12 to i32, !dbg !63
  %14 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 1, !dbg !64
  %15 = load i16, i16* %14, align 2, !dbg !64
  %16 = sext i16 %15 to i32, !dbg !64
  %17 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 2, !dbg !65
  %18 = load i16, i16* %17, align 4, !dbg !65
  %19 = sext i16 %18 to i32, !dbg !65
  %20 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 3, !dbg !66
  %21 = load i16, i16* %20, align 2, !dbg !66
  %22 = sext i16 %21 to i32, !dbg !66
  %23 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 4, !dbg !67
  %24 = load i16, i16* %23, align 8, !dbg !67
  %25 = sext i16 %24 to i32, !dbg !67
  %26 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 5, !dbg !68
  %27 = load i16, i16* %26, align 2, !dbg !68
  %28 = sext i16 %27 to i32, !dbg !68
  %29 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 6, !dbg !69
  %30 = load i16, i16* %29, align 4, !dbg !69
  %31 = sext i16 %30 to i32, !dbg !69
  %32 = getelementptr inbounds [8 x i16], [8 x i16]* %5, i64 0, i64 7, !dbg !70
  %33 = load i16, i16* %32, align 2, !dbg !70
  %34 = sext i16 %33 to i32, !dbg !70
  %35 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([29 x i8], [29 x i8]* @.str.1, i64 0, i64 0), i8* %10, i32 %13, i32 %16, i32 %19, i32 %22, i32 %25, i32 %28, i32 %31, i32 %34), !dbg !71
  ret void, !dbg !72
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @print128_num_4x32(i8*, <2 x i64>) #0 !dbg !73 {
  %3 = alloca i8*, align 8
  %4 = alloca <2 x i64>, align 16
  %5 = alloca [4 x i32], align 16
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !74, metadata !DIExpression()), !dbg !75
  store <2 x i64> %1, <2 x i64>* %4, align 16
  call void @llvm.dbg.declare(metadata <2 x i64>* %4, metadata !76, metadata !DIExpression()), !dbg !77
  call void @llvm.dbg.declare(metadata [4 x i32]* %5, metadata !78, metadata !DIExpression()), !dbg !83
  %6 = bitcast [4 x i32]* %5 to i8*, !dbg !83
  call void @llvm.memset.p0i8.i64(i8* align 16 %6, i8 0, i64 16, i1 false), !dbg !83
  %7 = getelementptr inbounds [4 x i32], [4 x i32]* %5, i64 0, i64 0, !dbg !84
  %8 = bitcast i32* %7 to i8*, !dbg !84
  %9 = bitcast <2 x i64>* %4 to i8*, !dbg !84
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %8, i8* align 16 %9, i64 16, i1 false), !dbg !84
  %10 = load i8*, i8** %3, align 8, !dbg !85
  %11 = getelementptr inbounds [4 x i32], [4 x i32]* %5, i64 0, i64 0, !dbg !86
  %12 = load i32, i32* %11, align 16, !dbg !86
  %13 = getelementptr inbounds [4 x i32], [4 x i32]* %5, i64 0, i64 1, !dbg !87
  %14 = load i32, i32* %13, align 4, !dbg !87
  %15 = getelementptr inbounds [4 x i32], [4 x i32]* %5, i64 0, i64 2, !dbg !88
  %16 = load i32, i32* %15, align 8, !dbg !88
  %17 = getelementptr inbounds [4 x i32], [4 x i32]* %5, i64 0, i64 3, !dbg !89
  %18 = load i32, i32* %17, align 4, !dbg !89
  %19 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.2, i64 0, i64 0), i8* %10, i32 %12, i32 %14, i32 %16, i32 %18), !dbg !90
  ret void, !dbg !91
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @print128_num_2x64(i8*, <2 x i64>) #0 !dbg !92 {
  %3 = alloca i8*, align 8
  %4 = alloca <2 x i64>, align 16
  %5 = alloca [2 x i64], align 16
  store i8* %0, i8** %3, align 8
  call void @llvm.dbg.declare(metadata i8** %3, metadata !93, metadata !DIExpression()), !dbg !94
  store <2 x i64> %1, <2 x i64>* %4, align 16
  call void @llvm.dbg.declare(metadata <2 x i64>* %4, metadata !95, metadata !DIExpression()), !dbg !96
  call void @llvm.dbg.declare(metadata [2 x i64]* %5, metadata !97, metadata !DIExpression()), !dbg !99
  %6 = bitcast [2 x i64]* %5 to i8*, !dbg !99
  call void @llvm.memset.p0i8.i64(i8* align 16 %6, i8 0, i64 16, i1 false), !dbg !99
  %7 = getelementptr inbounds [2 x i64], [2 x i64]* %5, i64 0, i64 0, !dbg !100
  %8 = bitcast i64* %7 to i8*, !dbg !100
  %9 = bitcast <2 x i64>* %4 to i8*, !dbg !100
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %8, i8* align 16 %9, i64 16, i1 false), !dbg !100
  %10 = load i8*, i8** %3, align 8, !dbg !101
  %11 = getelementptr inbounds [2 x i64], [2 x i64]* %5, i64 0, i64 0, !dbg !102
  %12 = load i64, i64* %11, align 16, !dbg !102
  %13 = getelementptr inbounds [2 x i64], [2 x i64]* %5, i64 0, i64 1, !dbg !103
  %14 = load i64, i64* %13, align 8, !dbg !103
  %15 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @.str.3, i64 0, i64 0), i8* %10, i64 %12, i64 %14), !dbg !104
  ret void, !dbg !105
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @usub16x8_sat() #0 !dbg !106 {
  %1 = alloca <2 x i64>, align 16
  %2 = alloca <2 x i64>, align 16
  %3 = alloca i8, align 1
  %4 = alloca i8, align 1
  %5 = alloca i8, align 1
  %6 = alloca i8, align 1
  %7 = alloca i8, align 1
  %8 = alloca i8, align 1
  %9 = alloca i8, align 1
  %10 = alloca i8, align 1
  %11 = alloca i8, align 1
  %12 = alloca i8, align 1
  %13 = alloca i8, align 1
  %14 = alloca i8, align 1
  %15 = alloca i8, align 1
  %16 = alloca i8, align 1
  %17 = alloca i8, align 1
  %18 = alloca i8, align 1
  %19 = alloca <16 x i8>, align 16
  %20 = alloca i8, align 1
  %21 = alloca i8, align 1
  %22 = alloca i8, align 1
  %23 = alloca i8, align 1
  %24 = alloca i8, align 1
  %25 = alloca i8, align 1
  %26 = alloca i8, align 1
  %27 = alloca i8, align 1
  %28 = alloca i8, align 1
  %29 = alloca i8, align 1
  %30 = alloca i8, align 1
  %31 = alloca i8, align 1
  %32 = alloca i8, align 1
  %33 = alloca i8, align 1
  %34 = alloca i8, align 1
  %35 = alloca i8, align 1
  %36 = alloca <16 x i8>, align 16
  %37 = alloca <2 x i64>, align 16
  %38 = alloca <2 x i64>, align 16
  %39 = alloca <2 x i64>, align 16
  call void @llvm.dbg.declare(metadata <2 x i64>* %37, metadata !109, metadata !DIExpression()), !dbg !110
  call void @llvm.dbg.declare(metadata <2 x i64>* %38, metadata !111, metadata !DIExpression()), !dbg !112
  call void @llvm.dbg.declare(metadata <2 x i64>* %39, metadata !113, metadata !DIExpression()), !dbg !114
  store i8 0, i8* %20, align 1, !dbg !115
  store i8 0, i8* %21, align 1, !dbg !115
  store i8 0, i8* %22, align 1, !dbg !115
  store i8 0, i8* %23, align 1, !dbg !115
  store i8 0, i8* %24, align 1, !dbg !115
  store i8 0, i8* %25, align 1, !dbg !115
  store i8 0, i8* %26, align 1, !dbg !115
  store i8 0, i8* %27, align 1, !dbg !115
  store i8 0, i8* %28, align 1, !dbg !115
  store i8 0, i8* %29, align 1, !dbg !115
  store i8 0, i8* %30, align 1, !dbg !115
  store i8 16, i8* %31, align 1, !dbg !115
  store i8 1, i8* %32, align 1, !dbg !115
  store i8 0, i8* %33, align 1, !dbg !115
  store i8 16, i8* %34, align 1, !dbg !115
  store i8 -1, i8* %35, align 1, !dbg !115
  %40 = load i8, i8* %35, align 1, !dbg !115
  %41 = insertelement <16 x i8> undef, i8 %40, i32 0, !dbg !115
  %42 = load i8, i8* %34, align 1, !dbg !115
  %43 = insertelement <16 x i8> %41, i8 %42, i32 1, !dbg !115
  %44 = load i8, i8* %33, align 1, !dbg !115
  %45 = insertelement <16 x i8> %43, i8 %44, i32 2, !dbg !115
  %46 = load i8, i8* %32, align 1, !dbg !115
  %47 = insertelement <16 x i8> %45, i8 %46, i32 3, !dbg !115
  %48 = load i8, i8* %31, align 1, !dbg !115
  %49 = insertelement <16 x i8> %47, i8 %48, i32 4, !dbg !115
  %50 = load i8, i8* %30, align 1, !dbg !115
  %51 = insertelement <16 x i8> %49, i8 %50, i32 5, !dbg !115
  %52 = load i8, i8* %29, align 1, !dbg !115
  %53 = insertelement <16 x i8> %51, i8 %52, i32 6, !dbg !115
  %54 = load i8, i8* %28, align 1, !dbg !115
  %55 = insertelement <16 x i8> %53, i8 %54, i32 7, !dbg !115
  %56 = load i8, i8* %27, align 1, !dbg !115
  %57 = insertelement <16 x i8> %55, i8 %56, i32 8, !dbg !115
  %58 = load i8, i8* %26, align 1, !dbg !115
  %59 = insertelement <16 x i8> %57, i8 %58, i32 9, !dbg !115
  %60 = load i8, i8* %25, align 1, !dbg !115
  %61 = insertelement <16 x i8> %59, i8 %60, i32 10, !dbg !115
  %62 = load i8, i8* %24, align 1, !dbg !115
  %63 = insertelement <16 x i8> %61, i8 %62, i32 11, !dbg !115
  %64 = load i8, i8* %23, align 1, !dbg !115
  %65 = insertelement <16 x i8> %63, i8 %64, i32 12, !dbg !115
  %66 = load i8, i8* %22, align 1, !dbg !115
  %67 = insertelement <16 x i8> %65, i8 %66, i32 13, !dbg !115
  %68 = load i8, i8* %21, align 1, !dbg !115
  %69 = insertelement <16 x i8> %67, i8 %68, i32 14, !dbg !115
  %70 = load i8, i8* %20, align 1, !dbg !115
  %71 = insertelement <16 x i8> %69, i8 %70, i32 15, !dbg !115
  store <16 x i8> %71, <16 x i8>* %36, align 16, !dbg !115
  %72 = load <16 x i8>, <16 x i8>* %36, align 16, !dbg !115
  %73 = bitcast <16 x i8> %72 to <2 x i64>, !dbg !115
  store <2 x i64> %73, <2 x i64>* %38, align 16, !dbg !116
  store i8 0, i8* %3, align 1, !dbg !117
  store i8 0, i8* %4, align 1, !dbg !117
  store i8 0, i8* %5, align 1, !dbg !117
  store i8 0, i8* %6, align 1, !dbg !117
  store i8 0, i8* %7, align 1, !dbg !117
  store i8 0, i8* %8, align 1, !dbg !117
  store i8 0, i8* %9, align 1, !dbg !117
  store i8 0, i8* %10, align 1, !dbg !117
  store i8 0, i8* %11, align 1, !dbg !117
  store i8 0, i8* %12, align 1, !dbg !117
  store i8 0, i8* %13, align 1, !dbg !117
  store i8 16, i8* %14, align 1, !dbg !117
  store i8 2, i8* %15, align 1, !dbg !117
  store i8 -1, i8* %16, align 1, !dbg !117
  store i8 17, i8* %17, align 1, !dbg !117
  store i8 -1, i8* %18, align 1, !dbg !117
  %74 = load i8, i8* %18, align 1, !dbg !117
  %75 = insertelement <16 x i8> undef, i8 %74, i32 0, !dbg !117
  %76 = load i8, i8* %17, align 1, !dbg !117
  %77 = insertelement <16 x i8> %75, i8 %76, i32 1, !dbg !117
  %78 = load i8, i8* %16, align 1, !dbg !117
  %79 = insertelement <16 x i8> %77, i8 %78, i32 2, !dbg !117
  %80 = load i8, i8* %15, align 1, !dbg !117
  %81 = insertelement <16 x i8> %79, i8 %80, i32 3, !dbg !117
  %82 = load i8, i8* %14, align 1, !dbg !117
  %83 = insertelement <16 x i8> %81, i8 %82, i32 4, !dbg !117
  %84 = load i8, i8* %13, align 1, !dbg !117
  %85 = insertelement <16 x i8> %83, i8 %84, i32 5, !dbg !117
  %86 = load i8, i8* %12, align 1, !dbg !117
  %87 = insertelement <16 x i8> %85, i8 %86, i32 6, !dbg !117
  %88 = load i8, i8* %11, align 1, !dbg !117
  %89 = insertelement <16 x i8> %87, i8 %88, i32 7, !dbg !117
  %90 = load i8, i8* %10, align 1, !dbg !117
  %91 = insertelement <16 x i8> %89, i8 %90, i32 8, !dbg !117
  %92 = load i8, i8* %9, align 1, !dbg !117
  %93 = insertelement <16 x i8> %91, i8 %92, i32 9, !dbg !117
  %94 = load i8, i8* %8, align 1, !dbg !117
  %95 = insertelement <16 x i8> %93, i8 %94, i32 10, !dbg !117
  %96 = load i8, i8* %7, align 1, !dbg !117
  %97 = insertelement <16 x i8> %95, i8 %96, i32 11, !dbg !117
  %98 = load i8, i8* %6, align 1, !dbg !117
  %99 = insertelement <16 x i8> %97, i8 %98, i32 12, !dbg !117
  %100 = load i8, i8* %5, align 1, !dbg !117
  %101 = insertelement <16 x i8> %99, i8 %100, i32 13, !dbg !117
  %102 = load i8, i8* %4, align 1, !dbg !117
  %103 = insertelement <16 x i8> %101, i8 %102, i32 14, !dbg !117
  %104 = load i8, i8* %3, align 1, !dbg !117
  %105 = insertelement <16 x i8> %103, i8 %104, i32 15, !dbg !117
  store <16 x i8> %105, <16 x i8>* %19, align 16, !dbg !117
  %106 = load <16 x i8>, <16 x i8>* %19, align 16, !dbg !117
  %107 = bitcast <16 x i8> %106 to <2 x i64>, !dbg !117
  store <2 x i64> %107, <2 x i64>* %39, align 16, !dbg !118
  %108 = load <2 x i64>, <2 x i64>* %38, align 16, !dbg !119
  %109 = load <2 x i64>, <2 x i64>* %39, align 16, !dbg !120
  store <2 x i64> %108, <2 x i64>* %1, align 16, !dbg !121
  store <2 x i64> %109, <2 x i64>* %2, align 16, !dbg !121
  %110 = load <2 x i64>, <2 x i64>* %1, align 16, !dbg !121
  %111 = bitcast <2 x i64> %110 to <16 x i8>, !dbg !121
  %112 = load <2 x i64>, <2 x i64>* %2, align 16, !dbg !121
  %113 = bitcast <2 x i64> %112 to <16 x i8>, !dbg !121
  %114 = call <16 x i8> @llvm.usub.sat.v16i8(<16 x i8> %111, <16 x i8> %113) #5, !dbg !121
  %115 = bitcast <16 x i8> %114 to <2 x i64>, !dbg !121
  store <2 x i64> %115, <2 x i64>* %37, align 16, !dbg !122
  %116 = load <2 x i64>, <2 x i64>* %37, align 16, !dbg !123
  call void @print128_num_16x8(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str.4, i64 0, i64 0), <2 x i64> %116), !dbg !124
  ret void, !dbg !125
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #4 !dbg !126 {
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  call void @usub16x8_sat(), !dbg !129
  ret i32 0, !dbg !130
}

; Function Attrs: nounwind readnone speculatable
declare <16 x i8> @llvm.usub.sat.v16i8(<16 x i8>, <16 x i8>) #1

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="128" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { argmemonly nounwind }
attributes #3 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!15, !16, !17}
!llvm.ident = !{!18}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 9.0.0 (https://github.com/llvm-mirror/llvm c62b24f070c9a4bb1a76409e623042a740cac4cd)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3, nameTableKind: None)
!1 = !DIFile(filename: "sse2_Intrinsics.c", directory: "/home/fire/robot_2019/klee-master/examples/sse_test")
!2 = !{}
!3 = !{!4, !10}
!4 = !DIDerivedType(tag: DW_TAG_typedef, name: "__m128i", file: !5, line: 16, baseType: !6)
!5 = !DIFile(filename: "/usr/local/lib/clang/9.0.0/include/emmintrin.h", directory: "")
!6 = !DICompositeType(tag: DW_TAG_array_type, baseType: !7, size: 128, flags: DIFlagVector, elements: !8)
!7 = !DIBasicType(name: "long long int", size: 64, encoding: DW_ATE_signed)
!8 = !{!9}
!9 = !DISubrange(count: 2)
!10 = !DIDerivedType(tag: DW_TAG_typedef, name: "__v16qi", file: !5, line: 25, baseType: !11)
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !12, size: 128, flags: DIFlagVector, elements: !13)
!12 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!13 = !{!14}
!14 = !DISubrange(count: 16)
!15 = !{i32 2, !"Dwarf Version", i32 4}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = !{i32 1, !"wchar_size", i32 4}
!18 = !{!"clang version 9.0.0 (https://github.com/llvm-mirror/llvm c62b24f070c9a4bb1a76409e623042a740cac4cd)"}
!19 = distinct !DISubprogram(name: "print128_num_16x8", scope: !1, file: !1, line: 7, type: !20, scopeLine: 7, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!20 = !DISubroutineType(types: !21)
!21 = !{null, !22, !4}
!22 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!23 = !DILocalVariable(name: "head", arg: 1, scope: !19, file: !1, line: 7, type: !22)
!24 = !DILocation(line: 7, column: 30, scope: !19)
!25 = !DILocalVariable(name: "var", arg: 2, scope: !19, file: !1, line: 7, type: !4)
!26 = !DILocation(line: 7, column: 44, scope: !19)
!27 = !DILocalVariable(name: "val", scope: !19, file: !1, line: 8, type: !28)
!28 = !DICompositeType(tag: DW_TAG_array_type, baseType: !12, size: 128, elements: !13)
!29 = !DILocation(line: 8, column: 10, scope: !19)
!30 = !DILocation(line: 9, column: 5, scope: !19)
!31 = !DILocation(line: 10, column: 69, scope: !19)
!32 = !DILocation(line: 11, column: 12, scope: !19)
!33 = !DILocation(line: 11, column: 20, scope: !19)
!34 = !DILocation(line: 11, column: 28, scope: !19)
!35 = !DILocation(line: 11, column: 36, scope: !19)
!36 = !DILocation(line: 11, column: 44, scope: !19)
!37 = !DILocation(line: 11, column: 52, scope: !19)
!38 = !DILocation(line: 11, column: 60, scope: !19)
!39 = !DILocation(line: 11, column: 68, scope: !19)
!40 = !DILocation(line: 12, column: 6, scope: !19)
!41 = !DILocation(line: 12, column: 14, scope: !19)
!42 = !DILocation(line: 12, column: 22, scope: !19)
!43 = !DILocation(line: 12, column: 31, scope: !19)
!44 = !DILocation(line: 12, column: 40, scope: !19)
!45 = !DILocation(line: 12, column: 49, scope: !19)
!46 = !DILocation(line: 12, column: 58, scope: !19)
!47 = !DILocation(line: 12, column: 67, scope: !19)
!48 = !DILocation(line: 10, column: 5, scope: !19)
!49 = !DILocation(line: 13, column: 1, scope: !19)
!50 = distinct !DISubprogram(name: "print128_num_8x16", scope: !1, file: !1, line: 15, type: !20, scopeLine: 15, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!51 = !DILocalVariable(name: "head", arg: 1, scope: !50, file: !1, line: 15, type: !22)
!52 = !DILocation(line: 15, column: 30, scope: !50)
!53 = !DILocalVariable(name: "var", arg: 2, scope: !50, file: !1, line: 15, type: !4)
!54 = !DILocation(line: 15, column: 44, scope: !50)
!55 = !DILocalVariable(name: "val", scope: !50, file: !1, line: 16, type: !56)
!56 = !DICompositeType(tag: DW_TAG_array_type, baseType: !57, size: 128, elements: !58)
!57 = !DIBasicType(name: "short", size: 16, encoding: DW_ATE_signed)
!58 = !{!59}
!59 = !DISubrange(count: 8)
!60 = !DILocation(line: 16, column: 11, scope: !50)
!61 = !DILocation(line: 17, column: 5, scope: !50)
!62 = !DILocation(line: 18, column: 45, scope: !50)
!63 = !DILocation(line: 19, column: 12, scope: !50)
!64 = !DILocation(line: 19, column: 20, scope: !50)
!65 = !DILocation(line: 19, column: 28, scope: !50)
!66 = !DILocation(line: 19, column: 36, scope: !50)
!67 = !DILocation(line: 19, column: 44, scope: !50)
!68 = !DILocation(line: 19, column: 52, scope: !50)
!69 = !DILocation(line: 19, column: 60, scope: !50)
!70 = !DILocation(line: 19, column: 68, scope: !50)
!71 = !DILocation(line: 18, column: 5, scope: !50)
!72 = !DILocation(line: 20, column: 1, scope: !50)
!73 = distinct !DISubprogram(name: "print128_num_4x32", scope: !1, file: !1, line: 22, type: !20, scopeLine: 22, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!74 = !DILocalVariable(name: "head", arg: 1, scope: !73, file: !1, line: 22, type: !22)
!75 = !DILocation(line: 22, column: 30, scope: !73)
!76 = !DILocalVariable(name: "var", arg: 2, scope: !73, file: !1, line: 22, type: !4)
!77 = !DILocation(line: 22, column: 44, scope: !73)
!78 = !DILocalVariable(name: "val", scope: !73, file: !1, line: 23, type: !79)
!79 = !DICompositeType(tag: DW_TAG_array_type, baseType: !80, size: 128, elements: !81)
!80 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!81 = !{!82}
!82 = !DISubrange(count: 4)
!83 = !DILocation(line: 23, column: 9, scope: !73)
!84 = !DILocation(line: 24, column: 5, scope: !73)
!85 = !DILocation(line: 25, column: 33, scope: !73)
!86 = !DILocation(line: 26, column: 12, scope: !73)
!87 = !DILocation(line: 26, column: 20, scope: !73)
!88 = !DILocation(line: 26, column: 28, scope: !73)
!89 = !DILocation(line: 26, column: 36, scope: !73)
!90 = !DILocation(line: 25, column: 5, scope: !73)
!91 = !DILocation(line: 27, column: 1, scope: !73)
!92 = distinct !DISubprogram(name: "print128_num_2x64", scope: !1, file: !1, line: 29, type: !20, scopeLine: 29, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!93 = !DILocalVariable(name: "head", arg: 1, scope: !92, file: !1, line: 29, type: !22)
!94 = !DILocation(line: 29, column: 30, scope: !92)
!95 = !DILocalVariable(name: "var", arg: 2, scope: !92, file: !1, line: 29, type: !4)
!96 = !DILocation(line: 29, column: 44, scope: !92)
!97 = !DILocalVariable(name: "val", scope: !92, file: !1, line: 30, type: !98)
!98 = !DICompositeType(tag: DW_TAG_array_type, baseType: !7, size: 128, elements: !8)
!99 = !DILocation(line: 30, column: 15, scope: !92)
!100 = !DILocation(line: 31, column: 5, scope: !92)
!101 = !DILocation(line: 32, column: 31, scope: !92)
!102 = !DILocation(line: 33, column: 12, scope: !92)
!103 = !DILocation(line: 33, column: 20, scope: !92)
!104 = !DILocation(line: 32, column: 5, scope: !92)
!105 = !DILocation(line: 34, column: 1, scope: !92)
!106 = distinct !DISubprogram(name: "usub16x8_sat", scope: !1, file: !1, line: 42, type: !107, scopeLine: 42, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!107 = !DISubroutineType(types: !108)
!108 = !{null}
!109 = !DILocalVariable(name: "round_16x8", scope: !106, file: !1, line: 43, type: !4)
!110 = !DILocation(line: 43, column: 10, scope: !106)
!111 = !DILocalVariable(name: "r16x8_1", scope: !106, file: !1, line: 44, type: !4)
!112 = !DILocation(line: 44, column: 10, scope: !106)
!113 = !DILocalVariable(name: "r16x8_2", scope: !106, file: !1, line: 44, type: !4)
!114 = !DILocation(line: 44, column: 18, scope: !106)
!115 = !DILocation(line: 45, column: 12, scope: !106)
!116 = !DILocation(line: 45, column: 10, scope: !106)
!117 = !DILocation(line: 46, column: 12, scope: !106)
!118 = !DILocation(line: 46, column: 10, scope: !106)
!119 = !DILocation(line: 47, column: 29, scope: !106)
!120 = !DILocation(line: 47, column: 38, scope: !106)
!121 = !DILocation(line: 47, column: 15, scope: !106)
!122 = !DILocation(line: 47, column: 13, scope: !106)
!123 = !DILocation(line: 48, column: 37, scope: !106)
!124 = !DILocation(line: 48, column: 2, scope: !106)
!125 = !DILocation(line: 49, column: 1, scope: !106)
!126 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 144, type: !127, scopeLine: 144, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!127 = !DISubroutineType(types: !128)
!128 = !{!80}
!129 = !DILocation(line: 145, column: 2, scope: !126)
!130 = !DILocation(line: 160, column: 2, scope: !126)
