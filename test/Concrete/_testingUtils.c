// This isn't a real test, just common code for the other ones.
//
// RUN: true

int printf(const char *fmt, ...);

void print_int(unsigned long long val);

#define TYPED_PRINT(_name_type, _arg_type)  \
    void print_ ## _name_type(_arg_type val) { print_int(val); }
 
TYPED_PRINT(i1, unsigned char)
TYPED_PRINT(i8, unsigned char)
TYPED_PRINT(i16, unsigned short)
TYPED_PRINT(i32, unsigned int)
TYPED_PRINT(i64, unsigned long long)

// This is a workaround to hide the "%c" only format string from the compiler --
// llvm-gcc can optimize this into putchar even at -O0, and the LLVM JIT doesn't
// recognize putchar() as a valid external function.
char *char_format_string = "%c";

void print_int(unsigned long long val) {
    int cur = 1;

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
