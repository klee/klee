; ModuleID = 'sse_test_1.c'
source_filename = "sse_test_1.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @subs8x16_sat() #0 !dbg !19 {
  %1 = alloca <2 x i64>, align 16
  %2 = alloca <2 x i64>, align 16
  %3 = alloca <2 x i64>, align 16
  %4 = alloca <2 x i64>, align 16
  call void @llvm.dbg.declare(metadata <2 x i64>* %3, metadata !22, metadata !DIExpression()), !dbg !23
  call void @llvm.dbg.declare(metadata <2 x i64>* %4, metadata !24, metadata !DIExpression()), !dbg !25
  %5 = load <2 x i64>, <2 x i64>* %3, align 16, !dbg !26
  %6 = load <2 x i64>, <2 x i64>* %4, align 16, !dbg !27
  store <2 x i64> %5, <2 x i64>* %1, align 16, !dbg !28
  store <2 x i64> %6, <2 x i64>* %2, align 16, !dbg !28
  %7 = load <2 x i64>, <2 x i64>* %1, align 16, !dbg !28
  %8 = bitcast <2 x i64> %7 to <8 x i16>, !dbg !28
  %9 = load <2 x i64>, <2 x i64>* %2, align 16, !dbg !28
  %10 = bitcast <2 x i64> %9 to <8 x i16>, !dbg !28
  %11 = call <8 x i16> @llvm.x86.sse2.psubs.w(<8 x i16> %8, <8 x i16> %10) #3, !dbg !28
  %12 = bitcast <8 x i16> %11 to <2 x i64>, !dbg !28
  store <2 x i64> %12, <2 x i64>* %4, align 16, !dbg !29
  ret void, !dbg !30
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare <8 x i16> @llvm.x86.sse2.psubs.w(<8 x i16>, <8 x i16>) #2

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!15, !16, !17}
!llvm.ident = !{!18}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 6.0.1-14 (tags/RELEASE_601/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3)
!1 = !DIFile(filename: "sse_test_1.c", directory: "/home/fire/robot_2019/klee-master/examples/sse_test")
!2 = !{}
!3 = !{!4, !10}
!4 = !DIDerivedType(tag: DW_TAG_typedef, name: "__m128i", file: !5, line: 30, baseType: !6)
!5 = !DIFile(filename: "/usr/lib/llvm-6.0/lib/clang/6.0.1/include/emmintrin.h", directory: "/home/fire/robot_2019/klee-master/examples/sse_test")
!6 = !DICompositeType(tag: DW_TAG_array_type, baseType: !7, size: 128, flags: DIFlagVector, elements: !8)
!7 = !DIBasicType(name: "long long int", size: 64, encoding: DW_ATE_signed)
!8 = !{!9}
!9 = !DISubrange(count: 2)
!10 = !DIDerivedType(tag: DW_TAG_typedef, name: "__v8hi", file: !5, line: 35, baseType: !11)
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !12, size: 128, flags: DIFlagVector, elements: !13)
!12 = !DIBasicType(name: "short", size: 16, encoding: DW_ATE_signed)
!13 = !{!14}
!14 = !DISubrange(count: 8)
!15 = !{i32 2, !"Dwarf Version", i32 4}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = !{i32 1, !"wchar_size", i32 4}
!18 = !{!"clang version 6.0.1-14 (tags/RELEASE_601/final)"}
!19 = distinct !DISubprogram(name: "subs8x16_sat", scope: !1, file: !1, line: 5, type: !20, isLocal: false, isDefinition: true, scopeLine: 5, isOptimized: false, unit: !0, variables: !2)
!20 = !DISubroutineType(types: !21)
!21 = !{null}
!22 = !DILocalVariable(name: "round_8x16b", scope: !19, file: !1, line: 6, type: !4)
!23 = !DILocation(line: 6, column: 10, scope: !19)
!24 = !DILocalVariable(name: "y_0_8x16b", scope: !19, file: !1, line: 6, type: !4)
!25 = !DILocation(line: 6, column: 22, scope: !19)
!26 = !DILocation(line: 7, column: 29, scope: !19)
!27 = !DILocation(line: 7, column: 42, scope: !19)
!28 = !DILocation(line: 7, column: 14, scope: !19)
!29 = !DILocation(line: 7, column: 12, scope: !19)
!30 = !DILocation(line: 8, column: 1, scope: !19)
