define void @"test_simple_arith"(i16 %i0, i16 %j0)
begin
  %t1 = add i16 %i0, %j0
  %t2 = sub i16 %i0, %j0
  %t3 = mul i16 %t1, %t2

  call void @print_i16(i16 %t3)

  ret void     
end

define void @"test_div_and_mod"(i16 %op1, i16 %op2)
begin
  %t1 = udiv i16 %op1, %op2
  %t2 = urem i16 %op1, %op2  
  %t3 = sdiv i16 %op1, %op2  
  %t4 = srem i16 %op1, %op2  

  call void @print_i16(i16 %t1)
  call void @print_i16(i16 %t2)
  call void @print_i16(i16 %t3)
  call void @print_i16(i16 %t4)

  ret void     
end
        
define void @test_cmp(i16 %op1, i16 %op2)
begin
  %t1 = icmp ule i16 %op1, %op2
  %t2 = icmp ult i16 %op1, %op2  
  %t3 = icmp uge i16 %op1, %op2  
  %t4 = icmp ugt i16 %op1, %op2  
  %t6 = icmp slt i16 %op1, %op2  
  %t5 = icmp sle i16 %op1, %op2
  %t7 = icmp sge i16 %op1, %op2  
  %t8 = icmp sgt i16 %op1, %op2  
  %t9 = icmp eq i16 %op1, %op2  
  %t10 = icmp ne i16 %op1, %op2  

  call void @print_i1(i1 %t1)
  call void @print_i1(i1 %t2)
  call void @print_i1(i1 %t3)
  call void @print_i1(i1 %t4)
  call void @print_i1(i1 %t5)
  call void @print_i1(i1 %t6)
  call void @print_i1(i1 %t7)
  call void @print_i1(i1 %t8)
  call void @print_i1(i1 %t9)
  call void @print_i1(i1 %t10)

  ret void
end

define i32 @main()
begin
    call void @test_simple_arith(i16 111, i16 100)

    call void @test_div_and_mod(i16 63331, i16 3123)
    call void @test_div_and_mod(i16 1000, i16 55444)
    call void @test_div_and_mod(i16 49012, i16 55444)
    call void @test_div_and_mod(i16 1000, i16 25)

    call void @test_cmp(i16 63331, i16 3123)
    call void @test_cmp(i16 1000, i16 55444)
    call void @test_cmp(i16 49012, i16 55444)
    call void @test_cmp(i16 1000, i16 25)
        
    ret i32 0
end

; defined in print_int.c
declare void @print_i1(i1)
declare void @print_i8(i8)
declare void @print_i16(i16)
declare void @print_i32(i32)
declare void @print_i64(i64)
