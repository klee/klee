// RUN: %clang -D_TESTINGUTILS_TEST %s -o %t
// RUN: %t | FileCheck %s

#include <stdint.h>

int printf(const char *fmt, ...);

void print_int(uint64_t val);

#define TYPED_PRINT(_name_type, _arg_type)  \
    void print_ ## _name_type(_arg_type val) { print_int(val); }
 
TYPED_PRINT(i1, uint8_t)
TYPED_PRINT(i8, uint8_t)
TYPED_PRINT(i16, uint16_t)
TYPED_PRINT(i32, uint32_t)
TYPED_PRINT(i64, uint64_t)

// This is a workaround to hide the "%c" only format string from the compiler --
// llvm-gcc can optimize this into putchar even at -O0, and the LLVM JIT doesn't
// recognize putchar() as a valid external function.
char *char_format_string = "%c";

void print_int(uint64_t val) {
    uint64_t cur = 1;

    // effectively do a log (can't call log because it returns floats)
    // do the nasty divide to prevent overflow
    while (cur <= val / 10)
        cur *= 10;

    while (cur) {
        int digit = val / cur;

        printf(char_format_string, digit + '0');
        
        val = val % cur;
        cur /= 10;
    }
    
    printf(char_format_string, '\n');
}


#ifdef _TESTINGUTILS_TEST
int main(int argc, char *argv[])
{
        print_i64(0xf000000000000064);
        // CHECK: 17293822569102704740
        print_i64(0x7000000000000064);
        // CHECK: 8070450532247928932

        print_i32(0xf0000032);
        // CHECK: 4026531890
        print_i32(0x70000032);
        // CHECK: 1879048242

        print_i16(0xf016);
        // CHECK: 61462
        print_i16(0x7016);
        // CHECK: 28694


        print_i8(0xf8);
        // CHECK: 248
        print_i8(0x78);
        // CHECK: 120

        printf("print_i1(0)\n");
        print_i1(0);
        // CHECK: i1(0)
        // CHECK_NEXT: 0

        printf("print_i1(1)\n");
        print_i1(1);
        // CHECK: i1(1)
        // CHECK_NEXT: 1

}
#endif
