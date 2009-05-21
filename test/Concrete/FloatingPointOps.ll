%struct.stdout = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct.stdout*, i32, i32, i32, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i32, i32, [40 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct.stdout*, i32 }
@stdout = external global %struct.stdout*

; casting error messages
@.strTrunc     = internal constant [15 x i8] c"FPTrunc broken\00"
@.strExt       = internal constant [13 x i8] c"FPExt broken\00"
@.strFPToUIFlt = internal constant [20 x i8] c"FPToUI float broken\00"
@.strFPToUIDbl = internal constant [21 x i8] c"FPToUI double broken\00"
@.strFPToSIFlt = internal constant [20 x i8] c"FPToSI float broken\00"
@.strFPToSIDbl = internal constant [21 x i8] c"FPToSI double broken\00"
@.strUIToFPFlt = internal constant [20 x i8] c"UIToFP float broken\00"
@.strUIToFPDbl = internal constant [21 x i8] c"UIToFP double broken\00"
@.strSIToFPFlt = internal constant [20 x i8] c"SIToFP float broken\00"
@.strSIToFPDbl = internal constant [21 x i8] c"SIToFP double broken\00"

; mathematical operator error messages
@.strDivFlt    = internal constant [18 x i8] c"FDiv float broken\00"
@.strDivDbl    = internal constant [19 x i8] c"FDiv double broken\00"
@.strRemFlt    = internal constant [18 x i8] c"FRem float broken\00"
@.strRemDbl    = internal constant [19 x i8] c"FRem double broken\00"
@.strAddInt    = internal constant [16 x i8] c"Add ints broken\00"
@.strAddFlt    = internal constant [18 x i8] c"Add floats broken\00"
@.strAddDbl    = internal constant [19 x i8] c"Add doubles broken\00"
@.strSubInt    = internal constant [16 x i8] c"Sub ints broken\00"
@.strSubFlt    = internal constant [18 x i8] c"Sub floats broken\00"
@.strSubDbl    = internal constant [19 x i8] c"Sub doubles broken\00"
@.strMulInt    = internal constant [16 x i8] c"Mul ints broken\00"
@.strMulFlt    = internal constant [18 x i8] c"Mul floats broken\00"
@.strMulDbl    = internal constant [19 x i8] c"Mul doubles broken\00"

; fcmp error messages
@.strCmpTrFlt  = internal constant [19 x i8] c"floats TRUE broken\00" ; fcmp::generic broken msgs
@.strCmpFaFlt  = internal constant [20 x i8] c"floats FALSE broken\00"
@.strCmpTrDbl  = internal constant [19 x i8] c"double TRUE broken\00"
@.strCmpFaDbl  = internal constant [20 x i8] c"double FALSE broken\00"
@.strCmpEqFlt  = internal constant [17 x i8] c"floats == broken\00" ; fcmp::ordered broken msgs
@.strCmpGeFlt  = internal constant [17 x i8] c"floats >= broken\00"
@.strCmpGtFlt  = internal constant [17 x i8] c"floats >  broken\00"
@.strCmpLeFlt  = internal constant [17 x i8] c"floats <= broken\00"
@.strCmpLtFlt  = internal constant [17 x i8] c"floats <  broken\00"
@.strCmpNeFlt  = internal constant [17 x i8] c"floats != broken\00"
@.strCmpOrdFlt = internal constant [18 x i8] c"floats ORD broken\00"
@.strCmpEqDbl  = internal constant [18 x i8] c"doubles == broken\00"
@.strCmpGeDbl  = internal constant [18 x i8] c"doubles >= broken\00"
@.strCmpGtDbl  = internal constant [18 x i8] c"doubles >  broken\00"
@.strCmpLeDbl  = internal constant [18 x i8] c"doubles <= broken\00"
@.strCmpLtDbl  = internal constant [18 x i8] c"doubles <  broken\00"
@.strCmpNeDbl  = internal constant [18 x i8] c"doubles != broken\00"
@.strCmpOrdDbl = internal constant [19 x i8] c"doubles ORD broken\00"
@.strCmpEqFltU = internal constant [17 x i8] c"U:floats==broken\00" ; fcmp::unordered broken msgs
@.strCmpGeFltU = internal constant [17 x i8] c"U:floats>=broken\00"
@.strCmpGtFltU = internal constant [17 x i8] c"U:floats> broken\00"
@.strCmpLeFltU = internal constant [17 x i8] c"U:floats<=broken\00"
@.strCmpLtFltU = internal constant [17 x i8] c"U:floats< broken\00"
@.strCmpNeFltU = internal constant [17 x i8] c"U:floats!=broken\00"
@.strCmpUnoFlt = internal constant [20 x i8] c"U:floats UNO broken\00"
@.strCmpEqDblU = internal constant [18 x i8] c"U:doubles==broken\00"
@.strCmpGeDblU = internal constant [18 x i8] c"U:doubles>=broken\00"
@.strCmpGtDblU = internal constant [18 x i8] c"U:doubles> broken\00"
@.strCmpLeDblU = internal constant [18 x i8] c"U:doubles<=broken\00"
@.strCmpLtDblU = internal constant [18 x i8] c"U:doubles< broken\00"
@.strCmpNeDblU = internal constant [18 x i8] c"U:doubles!=broken\00"
@.strCmpUnoDbl = internal constant [21 x i8] c"U:doubles UNO broken\00"

@.strWorks     = internal constant [20 x i8] c"Everything works!\0D\0A\00"
@.strNL        = internal constant [3 x i8]  c"\0D\0A\00"

declare i32 @fprintf(%struct.stdout*, i8*, ...)
declare void @exit(i32)
declare void @llvm.memcpy.i32(i8*, i8*, i32, i32)

; if isOk is false, then print errMsg to stdout and exit(1)
define void @failCheck(i1 %isOk, i8* %errMsg) {
entry:
  %fail = icmp eq i1 %isOk, 0
  br i1 %fail, label %failed, label %return

failed:
  ; print the error msg
  %err_stream = load %struct.stdout** @stdout
  %ret = call i32 (%struct.stdout*, i8*, ...)* @fprintf( %struct.stdout* %err_stream, i8* %errMsg )

  ; add a newline to the ostream
  %nl = getelementptr [3 x i8]* @.strNL, i32 0, i32 0
  %ret2 = call i32 (%struct.stdout*, i8*, ...)* @fprintf( %struct.stdout* %err_stream, i8* %nl )

  ; exit with return value 1 to denote that an error occurred
  call void @exit( i32 1 )
  unreachable

return:
  ret void
}

; test FPTrunc which casts doubles to floats
define void @testFPTrunc() {
entry:
  %d_addr = alloca double, align 8
  store double 8.000000e+00, double* %d_addr
  %d = load double* %d_addr
  %f = fptrunc double %d to float
  %matches = fcmp oeq float %f, 8.000000e+00
  %err_msg = getelementptr [15 x i8]* @.strTrunc, i32 0, i32 0
  call void @failCheck( i1 %matches, i8* %err_msg )
  ret void
}

; test FPExt which casts floats to doubles
define void @testFPExt() {
entry:
  %f_addr = alloca float, align 4
  store float 8.000000e+00, float* %f_addr
  %f = load float* %f_addr
  %d = fpext float %f to double
  %matches = fcmp oeq double %d, 8.000000e+00
  %err_msg = getelementptr [13 x i8]* @.strExt, i32 0, i32 0
  call void @failCheck( i1 %matches, i8* %err_msg )
  ret void
}

; test casting fp to an unsigned int
define void @testFPToUI() {
entry:
  %f_addr = alloca float, align 4
  %d_addr = alloca double, align 8

  ; test float to UI
  store float 0x4020333340000000, float* %f_addr; %f = 8.1
  %f = load float* %f_addr
  %uf = fptoui float %f to i32
  %matchesf = icmp eq i32 %uf, 8
  %err_msgf = getelementptr [20 x i8]* @.strFPToUIFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; test double to UI
  store double 8.100000e+00, double* %d_addr
  %d = load double* %d_addr
  %ud = fptoui double %d to i32
  %matchesd = icmp eq i32 %ud, 8
  %err_msgd = getelementptr [21 x i8]* @.strFPToUIDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; test casting fp to a signed int
define void @testFPToSI() {
entry:
  %f_addr = alloca float, align 4
  %d_addr = alloca double, align 8

  ; test float 8.1 to signed int
  store float 0x4020333340000000, float* %f_addr
  %f = load float* %f_addr
  %sf = fptosi float %f to i32
  %matchesf = icmp eq i32 %sf, 8
  %err_msgf = getelementptr [20 x i8]* @.strFPToSIFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; test double -8.1 to signed int
  store double -8.100000e+00, double* %d_addr
  %d = load double* %d_addr
  %sd = fptosi double %d to i32
  %matchesd = icmp eq i32 %sd, -8
  %err_msgd = getelementptr [21 x i8]* @.strFPToSIDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; test casting unsigned int to fp
define void @testUIToFP() {
entry:
  ; unsigned int to float
  %f = uitofp i32 7 to float
  %matchesf = fcmp oeq float %f, 7.000000e+00
  %err_msgf = getelementptr [20 x i8]* @.strUIToFPFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; unsigned int to double
  %d = uitofp i32 7 to double
  %matchesd = fcmp oeq double %d, 7.000000e+00
  %err_msgd = getelementptr [21 x i8]* @.strUIToFPDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; test casting signed int to fp
define void @testSIToFP() {
entry:
  ; signed int to float
  %f = sitofp i32 -7 to float
  %matchesf = fcmp oeq float %f, -7.000000e+00
  %err_msgf = getelementptr [20 x i8]* @.strSIToFPFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; signed int to double
  %d = sitofp i32 -7 to double
  %matchesd = fcmp oeq double %d, -7.000000e+00
  %err_msgd = getelementptr [21 x i8]* @.strSIToFPDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; testing fp division
define void @testFDiv() {
entry:
  %fN_addr = alloca float, align 4
  %fD_addr = alloca float, align 4
  %dN_addr = alloca double, align 8
  %dD_addr = alloca double, align 8

  ; float division
  store float 2.200000e+01, float* %fN_addr
  store float 4.000000e+00, float* %fD_addr
  %fN = load float* %fN_addr
  %fD = load float* %fD_addr
  %f = fdiv float %fN, %fD
  %matchesf = fcmp oeq float %f, 5.500000e+00
  %err_msgf = getelementptr [18 x i8]* @.strDivFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; double division
  store double 2.200000e+01, double* %dN_addr
  store double -4.000000e+00, double* %dD_addr
  %dN = load double* %dN_addr
  %dD = load double* %dD_addr
  %d = fdiv double %dN, %dD
  %matchesd = fcmp oeq double %d, -5.500000e+00
  %err_msgd = getelementptr [19 x i8]* @.strDivDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; testing fp modulo
define void @testFRem() {
entry:
  %fN_addr = alloca float, align 4
  %fD_addr = alloca float, align 4
  %dN_addr = alloca double, align 8
  %dD_addr = alloca double, align 8

  ; float modoulo
  store float 2.200000e+01, float* %fN_addr
  store float 4.000000e+00, float* %fD_addr
  %fN = load float* %fN_addr
  %fD = load float* %fD_addr
  %f = frem float %fN, %fD
  %matchesf = fcmp oeq float %f, 2.000000e+00
  %err_msgf = getelementptr [18 x i8]* @.strRemFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; double modulo
  store double -2.200000e+01, double* %dN_addr
  store double 4.000000e+00, double* %dD_addr
  %dN = load double* %dN_addr
  %dD = load double* %dD_addr
  %d = frem double %dN, %dD
  %matchesd = fcmp oeq double %d, -2.000000e+00
  %err_msgd = getelementptr [19 x i8]* @.strRemDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; test addition (fp and int since add is polymorphic)
define void @testAdd() {
entry:
  %f1_addr = alloca float, align 4
  %f2_addr = alloca float, align 4
  %d1_addr = alloca double, align 8
  %d2_addr = alloca double, align 8

  ; test integer addition (3 + 4)
  %sumi = add i32 3, 4
  %matchesi = icmp eq i32 %sumi, 7
  %err_msgi = getelementptr [16 x i8]* @.strAddInt, i32 0, i32 0
  call void @failCheck( i1 %matchesi, i8* %err_msgi )

  ; test float addition (3.5 + 4.2)
  store float 3.500000e+00, float* %f1_addr
  store float 0x4010CCCCC0000000, float* %f2_addr
  %f1 = load float* %f1_addr
  %f2 = load float* %f2_addr
  %sumf = add float %f1, %f2
  %matchesf = fcmp oeq float %sumf, 0x401ECCCCC0000000
  %err_msgf = getelementptr [18 x i8]* @.strAddFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; test double addition (3.5 + -4.2)
  store double 3.500000e+00, double* %d1_addr
  store double -4.200000e+00, double* %d2_addr
  %d1 = load double* %d1_addr
  %d2 = load double* %d2_addr
  %sumd = add double %d1, %d2
  %matchesd = fcmp oeq double %sumd, 0xBFE6666666666668
  %err_msgd = getelementptr [19 x i8]* @.strAddDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; test subtraction (fp and int since sub is polymorphic)
define void @testSub() {
entry:
  %f1_addr = alloca float, align 4
  %f2_addr = alloca float, align 4
  %d1_addr = alloca double, align 8
  %d2_addr = alloca double, align 8

  ; test integer subtraction (3 - 4)
  %subi = sub i32 3, 4
  %matchesi = icmp eq i32 %subi, -1
  %err_msgi = getelementptr [16 x i8]* @.strSubInt, i32 0, i32 0
  call void @failCheck( i1 %matchesi, i8* %err_msgi )

  ; test float subtraction (3.5 - 4.2)
  store float 3.500000e+00, float* %f1_addr
  store float 0x4010CCCCC0000000, float* %f2_addr
  %f1 = load float* %f1_addr
  %f2 = load float* %f2_addr
  %subf = sub float %f1, %f2
  %matchesf = fcmp oeq float %subf, 0xBFE6666600000000
  %err_msgf = getelementptr [18 x i8]* @.strSubFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; test double subtraction (3.5 - -4.2)
  store double 3.500000e+00, double* %d1_addr
  store double -4.200000e+00, double* %d2_addr
  %d1 = load double* %d1_addr
  %d2 = load double* %d2_addr
  %subd = sub double %d1, %d2
  %matchesd = fcmp oeq double %subd, 7.700000e+00
  %err_msgd = getelementptr [19 x i8]* @.strSubDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; test multiplication (fp and int since mul is polymorphic)
define void @testMul() {
entry:
  %f1_addr = alloca float, align 4
  %f2_addr = alloca float, align 4
  %d1_addr = alloca double, align 8
  %d2_addr = alloca double, align 8

  ; test integer multiplication (3 * 4)
  %muli = mul i32 3, 4
  %matchesi = icmp eq i32 %muli, 12
  %err_msgi = getelementptr [16 x i8]* @.strMulInt, i32 0, i32 0
  call void @failCheck( i1 %matchesi, i8* %err_msgi )

  ; test float multiplication (3.5 * 4.2)
  store float 3.500000e+00, float* %f1_addr
  store float 0x4010CCCCC0000000, float* %f2_addr
  %f1 = load float* %f1_addr
  %f2 = load float* %f2_addr
  %mulf = mul float %f1, %f2
  %matchesf = fcmp oeq float %mulf, 0x402D666640000000
  %err_msgf = getelementptr [18 x i8]* @.strMulFlt, i32 0, i32 0
  call void @failCheck( i1 %matchesf, i8* %err_msgf )

  ; test double multiplication (3.5 * -4.2)
  store double 3.500000e+00, double* %d1_addr
  store double -4.200000e+00, double* %d2_addr
  %d1 = load double* %d1_addr
  %d2 = load double* %d2_addr
  %muld = mul double %d1, %d2
  %matchesd = fcmp oeq double %muld, 0xC02D666666666667
  %err_msgd = getelementptr [19 x i8]* @.strMulDbl, i32 0, i32 0
  call void @failCheck( i1 %matchesd, i8* %err_msgd )

  ret void
}

; test float comparisons (ordered)
define void @testFCmpFOrdered(float %f1, float %f2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %ord) {
entry:
  ; test fcmp::true -- should always return true
  %cmp_t    = fcmp true float %f1, %f2
  %cmp_t_ok = icmp eq i1 %cmp_t, 1
  %cmp_t_em = getelementptr [19 x i8]* @.strCmpTrFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_t_ok, i8* %cmp_t_em )

  ; test fcmp::false -- should always return false
  %cmp_f    = fcmp false float %f1, %f2
  %cmp_f_ok = icmp eq i1 %cmp_f, 0
  %cmp_f_em = getelementptr [20 x i8]* @.strCmpFaFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_f_ok, i8* %cmp_f_em )

  ; test fcmp::ord -- should return true if neither operand is NaN
  %cmp_o    = fcmp ord float %f1, %f2
  %cmp_o_ok = icmp eq i1 %cmp_o, %ord
  %cmp_o_em = getelementptr [18 x i8]* @.strCmpOrdFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_o_ok, i8* %cmp_o_em )

  ; test fcmp::oeq -- should return true if neither operand is a NaN and they are equal
  %cmp_eq    = fcmp oeq float %f1, %f2
  %cmp_eq_ok = icmp eq i1 %cmp_eq, %eq
  %cmp_eq_em = getelementptr [17 x i8]* @.strCmpEqFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_eq_ok, i8* %cmp_eq_em )

  ; test fcmp::oge -- should return true if neither operand is a NaN and the first is greater or equal
  %cmp_ge    = fcmp oge float %f1, %f2
  %cmp_ge_ok = icmp eq i1 %cmp_ge, %ge
  %cmp_ge_em = getelementptr [17 x i8]* @.strCmpGeFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_ge_ok, i8* %cmp_ge_em )

  ; test fcmp::ogt -- should return true if neither operand is a NaN and the first is greater
  %cmp_gt    = fcmp ogt float %f1, %f2
  %cmp_gt_ok = icmp eq i1 %cmp_gt, %gt
  %cmp_gt_em = getelementptr [17 x i8]* @.strCmpGtFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_gt_ok, i8* %cmp_gt_em )

  ; test fcmp::ole -- should return true if neither operand is a NaN and the first is less or equal
  %cmp_le    = fcmp ole float %f1, %f2
  %cmp_le_ok = icmp eq i1 %cmp_le, %le
  %cmp_le_em = getelementptr [17 x i8]* @.strCmpLeFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_le_ok, i8* %cmp_le_em )

  ; test fcmp::olt -- should return true if neither operand is a NaN and the first is less
  %cmp_lt    = fcmp olt float %f1, %f2
  %cmp_lt_ok = icmp eq i1 %cmp_lt, %lt
  %cmp_lt_em = getelementptr [17 x i8]* @.strCmpLtFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_lt_ok, i8* %cmp_lt_em )

  ; test fcmp::one -- should return true if neither operand is a NaN and they are not equal
  %cmp_ne    = fcmp one float %f1, %f2
  %cmp_ne_ok = icmp eq i1 %cmp_ne, %ne
  %cmp_ne_em = getelementptr [17 x i8]* @.strCmpNeFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_ne_ok, i8* %cmp_ne_em )

  ret void
}

; test double comparisons (ordered)
define void @testFCmpDOrdered(double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %ord) {
entry:
  ; test fcmp::true -- should always return true
  %cmp_t    = fcmp true double %d1, %d2
  %cmp_t_ok = icmp eq i1 %cmp_t, 1
  %cmp_t_em = getelementptr [19 x i8]* @.strCmpTrDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_t_ok, i8* %cmp_t_em )

  ; test fcmp::false -- should always return false
  %cmp_f    = fcmp false double %d1, %d2
  %cmp_f_ok = icmp eq i1 %cmp_f, 0
  %cmp_f_em = getelementptr [20 x i8]* @.strCmpFaDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_f_ok, i8* %cmp_f_em )

  ; test fcmp::ord -- should return true if neither operand is NaN
  %cmp_o    = fcmp ord double %d1, %d2
  %cmp_o_ok = icmp eq i1 %cmp_o, %ord
  %cmp_o_em = getelementptr [19 x i8]* @.strCmpOrdDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_o_ok, i8* %cmp_o_em )

  ; test fcmp::oeq -- should return true if neither operand is a NaN and they are equal
  %cmp_eq    = fcmp oeq double %d1, %d2
  %cmp_eq_ok = icmp eq i1 %cmp_eq, %eq
  %cmp_eq_em = getelementptr [18 x i8]* @.strCmpEqDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_eq_ok, i8* %cmp_eq_em )

  ; test fcmp::oge -- should return true if neither operand is a NaN and the first is greater or equal
  %cmp_ge    = fcmp oge double %d1, %d2
  %cmp_ge_ok = icmp eq i1 %cmp_ge, %ge
  %cmp_ge_em = getelementptr [18 x i8]* @.strCmpGeDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_ge_ok, i8* %cmp_ge_em )

  ; test fcmp::ogt -- should return true if neither operand is a NaN and the first is greater
  %cmp_gt    = fcmp ogt double %d1, %d2
  %cmp_gt_ok = icmp eq i1 %cmp_gt, %gt
  %cmp_gt_em = getelementptr [18 x i8]* @.strCmpGtDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_gt_ok, i8* %cmp_gt_em )

  ; test fcmp::ole -- should return true if neither operand is a NaN and the first is less or equal
  %cmp_le    = fcmp ole double %d1, %d2
  %cmp_le_ok = icmp eq i1 %cmp_le, %le
  %cmp_le_em = getelementptr [18 x i8]* @.strCmpLeDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_le_ok, i8* %cmp_le_em )

  ; test fcmp::olt -- should return true if neither operand is a NaN and the first is less
  %cmp_lt    = fcmp olt double %d1, %d2
  %cmp_lt_ok = icmp eq i1 %cmp_lt, %lt
  %cmp_lt_em = getelementptr [18 x i8]* @.strCmpLtDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_lt_ok, i8* %cmp_lt_em )

  ; test fcmp::one -- should return true if neither operand is a NaN and they are not equal
  %cmp_ne    = fcmp one double %d1, %d2
  %cmp_ne_ok = icmp eq i1 %cmp_ne, %ne
  %cmp_ne_em = getelementptr [18 x i8]* @.strCmpNeDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_ne_ok, i8* %cmp_ne_em )

  ret void
}

; test floating point comparisons (ordered)
define void @testFCmpBothOrdered(double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %ord) {
entry:
  call void @testFCmpDOrdered( double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %ord )

  %f1 = fptrunc double %d1 to float
  %f2 = fptrunc double %d2 to float
  call void @testFCmpFOrdered( float %f1, float %f2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %ord )

  ret void
}

; test float comparisons (unordered)
define void @testFCmpFUnordered(float %f1, float %f2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %uno) {
entry:
  ; test fcmp::uno -- should return true if either operand is NaN
  %cmp_o    = fcmp uno float %f1, %f2
  %cmp_o_ok = icmp eq i1 %cmp_o, %uno
  %cmp_o_em = getelementptr [20 x i8]* @.strCmpUnoFlt, i32 0, i32 0
  call void @failCheck( i1 %cmp_o_ok, i8* %cmp_o_em )

  ; test fcmp::oeq -- should return true if either operand is a NaN and they are equal
  %cmp_eq    = fcmp ueq float %f1, %f2
  %cmp_eq_ok = icmp eq i1 %cmp_eq, %eq
  %cmp_eq_em = getelementptr [17 x i8]* @.strCmpEqFltU, i32 0, i32 0
  call void @failCheck( i1 %cmp_eq_ok, i8* %cmp_eq_em )

  ; test fcmp::oge -- should return true if either operand is a NaN and the first is greater or equal
  %cmp_ge    = fcmp uge float %f1, %f2
  %cmp_ge_ok = icmp eq i1 %cmp_ge, %ge
  %cmp_ge_em = getelementptr [17 x i8]* @.strCmpGeFltU, i32 0, i32 0
  call void @failCheck( i1 %cmp_ge_ok, i8* %cmp_ge_em )

  ; test fcmp::ogt -- should return true if either operand is a NaN and the first is greater
  %cmp_gt    = fcmp ugt float %f1, %f2
  %cmp_gt_ok = icmp eq i1 %cmp_gt, %gt
  %cmp_gt_em = getelementptr [17 x i8]* @.strCmpGtFltU, i32 0, i32 0
  call void @failCheck( i1 %cmp_gt_ok, i8* %cmp_gt_em )

  ; test fcmp::ole -- should return true if either operand is a NaN and the first is less or equal
  %cmp_le    = fcmp ule float %f1, %f2
  %cmp_le_ok = icmp eq i1 %cmp_le, %le
  %cmp_le_em = getelementptr [17 x i8]* @.strCmpLeFltU, i32 0, i32 0
  call void @failCheck( i1 %cmp_le_ok, i8* %cmp_le_em )

  ; test fcmp::olt -- should return true if either operand is a NaN and the first is less
  %cmp_lt    = fcmp ult float %f1, %f2
  %cmp_lt_ok = icmp eq i1 %cmp_lt, %lt
  %cmp_lt_em = getelementptr [17 x i8]* @.strCmpLtFltU, i32 0, i32 0
  call void @failCheck( i1 %cmp_lt_ok, i8* %cmp_lt_em )

  ; test fcmp::one -- should return true if either operand is a NaN and they are not equal
  %cmp_ne    = fcmp une float %f1, %f2
  %cmp_ne_ok = icmp eq i1 %cmp_ne, %ne
  %cmp_ne_em = getelementptr [17 x i8]* @.strCmpNeFltU, i32 0, i32 0
  call void @failCheck( i1 %cmp_ne_ok, i8* %cmp_ne_em )

  ret void
}

; test double comparisons (unordered)
define void @testFCmpDUnordered(double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %uno) {
entry:
  ; test fcmp::uno -- should return true if either operand is NaN
  %cmp_o    = fcmp uno double %d1, %d2
  %cmp_o_ok = icmp eq i1 %cmp_o, %uno
  %cmp_o_em = getelementptr [21 x i8]* @.strCmpUnoDbl, i32 0, i32 0
  call void @failCheck( i1 %cmp_o_ok, i8* %cmp_o_em )

  ; test fcmp::ueq -- should return true if either operand is a NaN and they are equal
  %cmp_eq    = fcmp ueq double %d1, %d2
  %cmp_eq_ok = icmp eq i1 %cmp_eq, %eq
  %cmp_eq_em = getelementptr [18 x i8]* @.strCmpEqDblU, i32 0, i32 0
  call void @failCheck( i1 %cmp_eq_ok, i8* %cmp_eq_em )

  ; test fcmp::uge -- should return true if either operand is a NaN and the first is greater or equal
  %cmp_ge    = fcmp uge double %d1, %d2
  %cmp_ge_ok = icmp eq i1 %cmp_ge, %ge
  %cmp_ge_em = getelementptr [18 x i8]* @.strCmpGeDblU, i32 0, i32 0
  call void @failCheck( i1 %cmp_ge_ok, i8* %cmp_ge_em )

  ; test fcmp::ugt -- should return true if either operand is a NaN and the first is greater
  %cmp_gt    = fcmp ugt double %d1, %d2
  %cmp_gt_ok = icmp eq i1 %cmp_gt, %gt
  %cmp_gt_em = getelementptr [18 x i8]* @.strCmpGtDblU, i32 0, i32 0
  call void @failCheck( i1 %cmp_gt_ok, i8* %cmp_gt_em )

  ; test fcmp::ule -- should return true if either operand is a NaN and the first is less or equal
  %cmp_le    = fcmp ule double %d1, %d2
  %cmp_le_ok = icmp eq i1 %cmp_le, %le
  %cmp_le_em = getelementptr [18 x i8]* @.strCmpLeDblU, i32 0, i32 0
  call void @failCheck( i1 %cmp_le_ok, i8* %cmp_le_em )

  ; test fcmp::ult -- should return true if either operand is a NaN and the first is less
  %cmp_lt    = fcmp ult double %d1, %d2
  %cmp_lt_ok = icmp eq i1 %cmp_lt, %lt
  %cmp_lt_em = getelementptr [18 x i8]* @.strCmpLtDblU, i32 0, i32 0
  call void @failCheck( i1 %cmp_lt_ok, i8* %cmp_lt_em )

  ; test fcmp::une -- should return true if either operand is a NaN and they are not equal
  %cmp_ne    = fcmp une double %d1, %d2
  %cmp_ne_ok = icmp eq i1 %cmp_ne, %ne
  %cmp_ne_em = getelementptr [18 x i8]* @.strCmpNeDblU, i32 0, i32 0
  call void @failCheck( i1 %cmp_ne_ok, i8* %cmp_ne_em )

  ret void
}

; test floating point comparisons (unordered)
define void @testFCmpBothUnordered(double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %uno) {
entry:
  call void @testFCmpDUnordered( double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %uno )

  %f1 = fptrunc double %d1 to float
  %f2 = fptrunc double %d2 to float
  call void @testFCmpFUnordered( float %f1, float %f2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %uno )

  ret void
}

; test floating point comparisons (ordered and unordered)
define void @testFCmpBoth(double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %ord, i1 %uno) {
entry:
  call void @testFCmpBothOrdered( double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %ord )
  call void @testFCmpBothUnordered( double %d1, double %d2, i1 %eq, i1 %ge, i1 %gt, i1 %le, i1 %lt, i1 %ne, i1 %uno )

  ret void
}

; test floating point comparisons (ordered and unordered) with a variety of real numbers and NaNs as operands
define void @testFCmp() {
entry:
  %x = alloca i64, align 8
  %nan = alloca double, align 8

  ; test FCmp on some real number inputs
  call void @testFCmpBoth( double 0.000000e+00, double 0.000000e+00, i1 1, i1 1, i1 0, i1 1, i1 0, i1 0, i1 1, i1 0 )
  call void @testFCmpBoth( double 0.000000e+00, double 1.000000e+00, i1 0, i1 0, i1 0, i1 1, i1 1, i1 1, i1 1, i1 0 )
  call void @testFCmpBoth( double 1.000000e+00, double 0.000000e+00, i1 0, i1 1, i1 1, i1 0, i1 0, i1 1, i1 1, i1 0 )

  ; build NaN
  store i64 -1, i64* %x
  %nan_as_i8 = bitcast double* %nan to i8*
  %x_as_i8 = bitcast i64* %x to i8*
  call void @llvm.memcpy.i32( i8* %nan_as_i8, i8* %x_as_i8, i32 8, i32 8 )

  ; load two copies of our NaN
  %nan1 = load double* %nan
  %nan2 = load double* %nan

  ; Warning: NaN comparisons with normal operators is BROKEN in LLVM JIT v2.0.  Fixed in v2.1.
  ; NaNs do different things depending on ordered vs unordered
;  call void @testFCmpBothOrdered( double %nan1, double 0.000000e+00, i1 0, i1 0, i1 0, i1 0, i1 0, i1 0, i1 0 )
;  call void @testFCmpBothOrdered( double %nan1, double %nan2, i1 0, i1 0, i1 0, i1 0, i1 0, i1 0, i1 0 )
;  call void @testFCmpBothUnordered( double %nan1, double 0.000000e+00, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1 )
;  call void @testFCmpBothUnordered( double %nan1, double %nan2, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1 )

  ret void
}

; tes all floating point instructions
define i32 @main() {
entry:
  call void @testFPTrunc( )
  call void @testFPExt( )
  call void @testFPToUI( )
  call void @testFPToSI( )
  call void @testUIToFP( )
  call void @testSIToFP( )

  call void @testFDiv( )
  call void @testFRem( )
  call void @testAdd( )
  call void @testSub( )
  call void @testMul( )

  call void @testFCmp( )

  ; everything worked -- print a message saying so
  %works_msg = getelementptr [20 x i8]* @.strWorks, i32 0, i32 0
  %err_stream = load %struct.stdout** @stdout
  %ret = call i32 (%struct.stdout*, i8*, ...)* @fprintf( %struct.stdout* %err_stream, i8* %works_msg )

  ret i32 0
}
