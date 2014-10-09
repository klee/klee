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
  %t2 = and i64 sub(i64 sext(i32 ptrtoint(i32* @gInt to i32) to i64), i64 ptrtoint(i32* @gInt to i64)), 4294967295
  %t3 = and i64 sub(i64 zext(i32 ptrtoint(i32* @gInt to i32) to i64), i64 ptrtoint(i32* @gInt to i64)), 4294967295

  %t4 = icmp eq i8 trunc(i64 ptrtoint(i32* @gInt to i64) to i8), %t1
  %t5 = zext i1 %t4 to i8
    
  call void @print_i8(i8 %t5)
  call void @print_i64(i64 %t2)
  call void @print_i64(i64 %t3)
  
  ret void
}

define void @"test_logical_ops"() {
  %t1 = add i32 -10, and(i32 ptrtoint(i32* @gInt to i32), i32 xor(i32 ptrtoint(i32* @gInt to i32), i32 -1))
  %t2 = add i32 -10, or(i32 ptrtoint(i32* @gInt to i32), i32 xor(i32 ptrtoint(i32* @gInt to i32), i32 -1))
  %t3 = add i32 -10, xor(i32 xor(i32 ptrtoint(i32* @gInt to i32), i32 1024),  i32 ptrtoint(i32* @gInt to i32))

  call void @print_i32(i32 %t1)
  call void @print_i32(i32 %t2)
  call void @print_i32(i32 %t3)
  
  %t4 = shl i32 lshr(i32 ptrtoint(i32* @gInt to i32), i32 8), 8
  %t5 = shl i32 ashr(i32 ptrtoint(i32* @gInt to i32), i32 8), 8
  %t6 = lshr i32 shl(i32 ptrtoint(i32* @gInt to i32), i32 8), 8
  
  %t7 = icmp eq i32 %t4, %t5     
  %t8 = icmp ne i32 %t4, %t6     
  
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
  %t1 = add i32 select(i1 icmp eq (i32* @gInt, i32* inttoptr(i32 100 to i32*)), i32 10, i32 0), 0
  call void @print_i32(i32 %t1)

  %t2 = load i32* getelementptr(%test.struct.type* @test_struct, i32 0, i32 1)
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

define void @"test_div_and_mod"() {
  %t1 = add i32 udiv(i32 ptrtoint(i32* @gInt to i32), i32 13), 0
  %t2 = add i32 urem(i32 ptrtoint(i32* @gInt to i32), i32 13), 0
  %t3 = add i32 sdiv(i32 ptrtoint(i32* @gInt to i32), i32 13), 0
  %t4 = add i32 srem(i32 ptrtoint(i32* @gInt to i32), i32 13), 0

  %p = ptrtoint i32* @gInt to i32

  %i1 = udiv i32 %p, 13
  %i2 = urem i32 %p, 13
  %i3 = sdiv i32 %p, 13
  %i4 = srem i32 %p, 13

  %x1 = sub i32 %t1, %i1
  %x2 = sub i32 %t2, %i2
  %x3 = sub i32 %t3, %i3
  %x4 = sub i32 %t4, %i4

  call void @print_i32(i32 %x1)
  call void @print_i32(i32 %x2)
  call void @print_i32(i32 %x3)
  call void @print_i32(i32 %x4)

  ret void     
}
        
define void @test_cmp() {
  %t1 = add i8 zext(i1 icmp ult (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t2 = add i8 zext(i1 icmp ule (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t3 = add i8 zext(i1 icmp uge (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t4 = add i8 zext(i1 icmp ugt (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t5 = add i8 zext(i1 icmp slt (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t6 = add i8 zext(i1 icmp sle (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t7 = add i8 zext(i1 icmp sge (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t8 = add i8 zext(i1 icmp sgt (i64 ptrtoint(i32* @gInt to i64), i64 0) to i8), 1
  %t9 = add i8 zext(i1 icmp eq (i64 ptrtoint(i32* @gInt to i64), i64 10) to i8), 1
  %t10 = add i8 zext(i1 icmp ne (i64 ptrtoint(i32* @gInt to i64), i64 10) to i8), 1

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

    call void @test_div_and_mod()

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
