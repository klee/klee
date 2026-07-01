; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

; Most of the test below use the *address* of gInt as part of their computation,
; and then perform some operation (like x | ~x) which makes the result
; deterministic. They do, however, assume that the sign bit of the address as a
; 64-bit value will never be set.
@gInt = global i32 10
@gIntWithConstant = global i32 sub(i32 ptrtoint(i32* @gInt to i32), 
                                 i32 ptrtoint(i32* @gInt to i32))

define void @"test_int_to_ptr"() {
  %t1 = add i8 ptrtoint(i8* inttoptr(i32 100 to i8*) to i8), 0
  %t2 = add i32 ptrtoint(i32* inttoptr(i8 100 to i32*) to i32), 0
  %t3 = add i32 ptrtoint(i32* inttoptr(i64 100 to i32*) to i32), 0
  %t4 = add i64 ptrtoint(i8* inttoptr(i32 100 to i8*) to i64), 0

  call void @print_i8(i8 %t1)
  call void @print_i32(i32 %t2)
  call void @print_i32(i32 %t3)
  call void @print_i64(i64 %t4)
    
  ret void
}

define void @"test_constant_ops"() {
  %t1 = add i8 trunc(i64 add(i64 ptrtoint(i32* @gInt to i64), i64 -10) to i8), 10
  %addr32 = ptrtoint i32* @gInt to i32
  %addr64 = ptrtoint i32* @gInt to i64
  %t2.ext = sext i32 %addr32 to i64
  %t2.sub = sub i64 %t2.ext, %addr64
  %t2 = and i64 %t2.sub, 4294967295
  %t3.ext = zext i32 %addr32 to i64
  %t3.sub = sub i64 %t3.ext, %addr64
  %t3 = and i64 %t3.sub, 4294967295

  %t4 = icmp eq i8 trunc(i64 ptrtoint(i32* @gInt to i64) to i8), %t1
  %t5 = zext i1 %t4 to i8
    
  call void @print_i8(i8 %t5)
  call void @print_i64(i64 %t2)
  call void @print_i64(i64 %t3)
  
  ret void
}

define void @"test_logical_ops"() {
  %addr32 = ptrtoint i32* @gInt to i32
  %not.addr32 = xor i32 %addr32, -1
  %and.addr32 = and i32 %addr32, %not.addr32
  %t1 = add i32 -10, %and.addr32
  %or.addr32 = or i32 %addr32, %not.addr32
  %t2 = add i32 -10, %or.addr32
  %xor.tmp = xor i32 %addr32, 1024
  %xor.addr32 = xor i32 %xor.tmp, %addr32
  %t3 = add i32 -10, %xor.addr32

  call void @print_i32(i32 %t1)
  call void @print_i32(i32 %t2)
  call void @print_i32(i32 %t3)

  ; or the address with 1 to ensure the addresses will differ in 'ne' below
  %addr64 = ptrtoint i32* @gInt to i64
  %or.addr64 = or i64 %addr64, 1
  %t4.shift = lshr i64 %or.addr64, 8
  %t4 = shl i64 %t4.shift, 8
  %t5.shift = ashr i64 %or.addr64, 8
  %t5 = shl i64 %t5.shift, 8
  %t6.shift = shl i64 %or.addr64, 8
  %t6 = lshr i64 %t6.shift, 8
  
  %t7 = icmp eq i64 %t4, %t5
  %t8 = icmp ne i64 %t4, %t6
  
  %t9 = zext i1 %t7 to i8
  %t10 = zext i1 %t8 to i8
  
  call void @print_i8(i8 %t9)
  call void @print_i8(i8 %t10)
  
  ret void   
}

%test.struct.type = type { i32, i32 }
@test_struct = global %test.struct.type { i32 0, i32 10 }

define void @"test_misc"() {
  ; probability that @gInt == 100 is very very low 
  %t1.cmp = icmp eq i32* @gInt, inttoptr(i32 100 to i32*)
  %t1.select = select i1 %t1.cmp, i32 10, i32 0
  %t1 = add i32 %t1.select, 0
  call void @print_i32(i32 %t1)

  %t2 = load i32, i32* getelementptr(%test.struct.type, %test.struct.type* @test_struct, i32 0, i32 1)
  call void @print_i32(i32 %t2)                             
        
  ret void
}

define void @"test_simple_arith"() {
  %t1 = add i32 add(i32 ptrtoint(i32* @gInt to i32), i32 0), 0
  %t2 = add i32 sub(i32 0, i32 ptrtoint(i32* @gInt to i32)), %t1
  %t3 = mul i32 mul(i32 ptrtoint(i32* @gInt to i32), i32 10), %t2

  call void @print_i32(i32 %t3)

  ret void     
}
        
define void @test_cmp() {
  %addr64 = ptrtoint i32* @gInt to i64
  %t1.cmp = icmp ult i64 %addr64, 0
  %t1.ext = zext i1 %t1.cmp to i8
  %t1 = add i8 %t1.ext, 1
  %t2.cmp = icmp ule i64 %addr64, 0
  %t2.ext = zext i1 %t2.cmp to i8
  %t2 = add i8 %t2.ext, 1
  %t3.cmp = icmp uge i64 %addr64, 0
  %t3.ext = zext i1 %t3.cmp to i8
  %t3 = add i8 %t3.ext, 1
  %t4.cmp = icmp ugt i64 %addr64, 0
  %t4.ext = zext i1 %t4.cmp to i8
  %t4 = add i8 %t4.ext, 1
  %t5.cmp = icmp slt i64 %addr64, 0
  %t5.ext = zext i1 %t5.cmp to i8
  %t5 = add i8 %t5.ext, 1
  %t6.cmp = icmp sle i64 %addr64, 0
  %t6.ext = zext i1 %t6.cmp to i8
  %t6 = add i8 %t6.ext, 1
  %t7.cmp = icmp sge i64 %addr64, 0
  %t7.ext = zext i1 %t7.cmp to i8
  %t7 = add i8 %t7.ext, 1
  %t8.cmp = icmp sgt i64 %addr64, 0
  %t8.ext = zext i1 %t8.cmp to i8
  %t8 = add i8 %t8.ext, 1
  %t9.cmp = icmp eq i64 %addr64, 10
  %t9.ext = zext i1 %t9.cmp to i8
  %t9 = add i8 %t9.ext, 1
  %t10.cmp = icmp ne i64 %addr64, 10
  %t10.ext = zext i1 %t10.cmp to i8
  %t10 = add i8 %t10.ext, 1

  call void @print_i1(i8 %t1)
  call void @print_i1(i8 %t2)
  call void @print_i1(i8 %t3)
  call void @print_i1(i8 %t4)
  call void @print_i1(i8 %t5)
  call void @print_i1(i8 %t6)
  call void @print_i1(i8 %t7)
  call void @print_i1(i8 %t8)
  call void @print_i1(i8 %t9)
  call void @print_i1(i8 %t10)

  ret void
}

define i32 @main() {
    call void @test_simple_arith()

    call void @test_cmp()
 
    call void @test_int_to_ptr()

    call void @test_constant_ops()

    call void @test_logical_ops()

    call void @test_misc()
    
    ret i32 0
}

; defined in print_int.c
declare void @print_i1(i8)
declare void @print_i8(i8)
declare void @print_i16(i16)
declare void @print_i32(i32)
declare void @print_i64(i64)
