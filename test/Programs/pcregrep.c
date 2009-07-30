// RUN: %llvmgcc -m32 %s -emit-llvm -O0 -c -o %t1.bc
// RUN: %klee --libc=klee --exit-on-error %t1.bc 2 2
// XFAIL: x86_64

/* Provide Declarations */
#include <stdarg.h>
#include <setjmp.h>
/* get a declaration for alloca */
#if defined(__CYGWIN__) || defined(__MINGW32__)
#define  alloca(x) __builtin_alloca((x))
#define _alloca(x) __builtin_alloca((x))
#elif defined(__APPLE__)
extern void *__builtin_alloca(unsigned long);
#define alloca(x) __builtin_alloca(x)
#define longjmp _longjmp
#define setjmp _setjmp
#elif defined(__sun__)
#if defined(__sparcv9)
extern void *__builtin_alloca(unsigned long);
#else
extern void *__builtin_alloca(unsigned int);
#endif
#define alloca(x) __builtin_alloca(x)
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
#define alloca(x) __builtin_alloca(x)
#elif defined(_MSC_VER)
#define inline _inline
#define alloca(x) _alloca(x)
#else
#include <alloca.h>
#endif

#ifndef __GNUC__  /* Can only support "linkonce" vars with GCC */
#define __attribute__(X)
#endif

#if defined(__GNUC__) && defined(__APPLE_CC__)
#define __EXTERNAL_WEAK__ __attribute__((weak_import))
#elif defined(__GNUC__)
#define __EXTERNAL_WEAK__ __attribute__((weak))
#else
#define __EXTERNAL_WEAK__
#endif

#if defined(__GNUC__) && defined(__APPLE_CC__)
#define __ATTRIBUTE_WEAK__
#elif defined(__GNUC__)
#define __ATTRIBUTE_WEAK__ __attribute__((weak))
#else
#define __ATTRIBUTE_WEAK__
#endif

#if defined(__GNUC__)
#define __HIDDEN__ __attribute__((visibility("hidden")))
#endif

#ifdef __GNUC__
#define LLVM_NAN(NanStr)   __builtin_nan(NanStr)   /* Double */
#define LLVM_NANF(NanStr)  __builtin_nanf(NanStr)  /* Float */
#define LLVM_NANS(NanStr)  __builtin_nans(NanStr)  /* Double */
#define LLVM_NANSF(NanStr) __builtin_nansf(NanStr) /* Float */
#define LLVM_INF           __builtin_inf()         /* Double */
#define LLVM_INFF          __builtin_inff()        /* Float */
#define LLVM_PREFETCH(addr,rw,locality) __builtin_prefetch(addr,rw,locality)
#define __ATTRIBUTE_CTOR__ __attribute__((constructor))
#define __ATTRIBUTE_DTOR__ __attribute__((destructor))
#define LLVM_ASM           __asm__
#else
#define LLVM_NAN(NanStr)   ((double)0.0)           /* Double */
#define LLVM_NANF(NanStr)  0.0F                    /* Float */
#define LLVM_NANS(NanStr)  ((double)0.0)           /* Double */
#define LLVM_NANSF(NanStr) 0.0F                    /* Float */
#define LLVM_INF           ((double)0.0)           /* Double */
#define LLVM_INFF          0.0F                    /* Float */
#define LLVM_PREFETCH(addr,rw,locality)            /* PREFETCH */
#define __ATTRIBUTE_CTOR__
#define __ATTRIBUTE_DTOR__
#define LLVM_ASM(X)
#endif

#if __GNUC__ < 4 /* Old GCC's, or compilers not GCC */ 
#define __builtin_stack_save() 0   /* not implemented */
#define __builtin_stack_restore(X) /* noop */
#endif

#define CODE_FOR_MAIN() /* Any target-specific code for main()*/
#if defined(__GNUC__) && !defined(__llvm__)
#if defined(i386) || defined(__i386__) || defined(__i386) || defined(__x86_64__)
#undef CODE_FOR_MAIN
#define CODE_FOR_MAIN() \
  {short F;__asm__ ("fnstcw %0" : "=m" (*&F)); \
  F=(F&~0x300)|0x200;__asm__("fldcw %0"::"m"(*&F));}
#endif
#endif

#ifndef __cplusplus
typedef unsigned char bool;
#endif


/* Support for floating point constants */
typedef unsigned long long ConstantDoubleTy;
typedef unsigned int        ConstantFloatTy;


/* Global Declarations */
/* Helper union for bitcasts */
typedef union {
  unsigned int Int32;
  unsigned long long Int64;
  float Float;
  double Double;
} llvmBitCastUnion;
/* Structure forward decls */
struct l_struct_2E__IO_FILE;
struct l_struct_2E__IO_marker;
struct l_struct_2E_branch_chain;
struct l_struct_2E_compile_data;
struct l_struct_2E_pcre;

/* Typedefs */
typedef struct l_struct_2E__IO_FILE l_struct_2E__IO_FILE;
typedef struct l_struct_2E__IO_marker l_struct_2E__IO_marker;
typedef struct l_struct_2E_branch_chain l_struct_2E_branch_chain;
typedef struct l_struct_2E_compile_data l_struct_2E_compile_data;
typedef struct l_struct_2E_pcre l_struct_2E_pcre;

/* Structure contents */
struct l_struct_2E__IO_FILE {
  unsigned int field0;
  unsigned char *field1;
  unsigned char *field2;
  unsigned char *field3;
  unsigned char *field4;
  unsigned char *field5;
  unsigned char *field6;
  unsigned char *field7;
  unsigned char *field8;
  unsigned char *field9;
  unsigned char *field10;
  unsigned char *field11;
  struct l_struct_2E__IO_marker *field12;
  struct l_struct_2E__IO_FILE *field13;
  unsigned int field14;
  unsigned int field15;
  unsigned int field16;
  unsigned short field17;
  unsigned char field18;
  unsigned char field19[1];
  unsigned char *field20;
  unsigned long long field21;
  unsigned char *field22;
  unsigned char *field23;
  unsigned int field24;
  unsigned char field25[52];
};

struct l_struct_2E__IO_marker {
  struct l_struct_2E__IO_marker *field0;
  struct l_struct_2E__IO_FILE *field1;
  unsigned int field2;
};

struct l_struct_2E_branch_chain {
  struct l_struct_2E_branch_chain *field0;
  unsigned char *field1;
};

struct l_struct_2E_compile_data {
  unsigned char *field0;
  unsigned char *field1;
  unsigned char *field2;
  unsigned char *field3;
  unsigned char *field4;
  unsigned char *field5;
  unsigned char *field6;
  unsigned char *field7;
  unsigned char *field8;
  unsigned char *field9;
  unsigned int field10;
  unsigned int field11;
  unsigned int field12;
  unsigned int field13;
  unsigned int field14;
  unsigned int field15;
  unsigned int field16;
  unsigned int field17;
  unsigned int field18;
  unsigned int field19;
  unsigned char field20[4];
};

struct l_struct_2E_pcre {
  unsigned int field0;
  unsigned int field1;
  unsigned int field2;
  unsigned int field3;
  unsigned short field4;
  unsigned short field5;
  unsigned short field6;
  unsigned short field7;
  unsigned short field8;
  unsigned short field9;
  unsigned short field10;
  unsigned short field11;
  unsigned char *field12;
  unsigned char *field13;
};

/* Function Declarations */
double fmod(double, double);
float fmodf(float, float);
unsigned int main(unsigned int llvm_cbe_argc, unsigned char **llvm_cbe_argv);
unsigned int fprintf(struct l_struct_2E__IO_FILE *, unsigned char *, ...);
unsigned int __strtol_internal(unsigned char *, unsigned char **, unsigned int , unsigned int );
unsigned int printf(unsigned char *, ...);
unsigned int fwrite(unsigned char *, unsigned int , unsigned int , unsigned char *);
unsigned int klee_make_symbolic();
unsigned int puts(unsigned char *);
static unsigned int check_escape(unsigned char **llvm_cbe_ptrptr, unsigned int *llvm_cbe_errorcodeptr, unsigned int llvm_cbe_bracount, unsigned int llvm_cbe_options, unsigned int llvm_cbe_isclass);
static unsigned int find_parens(unsigned char *llvm_cbe_ptr, unsigned int llvm_cbe_count, unsigned char *llvm_cbe_name, unsigned int llvm_cbe_lorn, unsigned int llvm_cbe_xmode);
unsigned int strncmp(unsigned char *, unsigned char *, unsigned int );
static unsigned char *first_significant_code(unsigned char *llvm_cbe_code, unsigned int *llvm_cbe_options, unsigned int llvm_cbe_optbit, unsigned int llvm_cbe_skipassert);
static unsigned int find_fixedlength(unsigned char *llvm_cbe_code);
static unsigned char *find_bracket(unsigned char *llvm_cbe_code, unsigned int llvm_cbe_number);
static unsigned int could_be_empty_branch(unsigned char *llvm_cbe_code, unsigned char *llvm_cbe_endcode);
static unsigned int check_posix_syntax(unsigned char *llvm_cbe_ptr, unsigned char **llvm_cbe_endptr, struct l_struct_2E_compile_data *llvm_cbe_cd);
static void adjust_recurse(unsigned char *llvm_cbe_group, unsigned int llvm_cbe_adjust, struct l_struct_2E_compile_data *llvm_cbe_cd, unsigned char *llvm_cbe_save_hwm);
static unsigned int check_auto_possessive(unsigned int llvm_cbe_op_code, unsigned int llvm_cbe_item, unsigned char *llvm_cbe_ptr, unsigned int llvm_cbe_options, struct l_struct_2E_compile_data *llvm_cbe_cd);
static unsigned int _pcre_is_newline(unsigned char *llvm_cbe_ptr, unsigned int llvm_cbe_type, unsigned char *llvm_cbe_endptr, unsigned int *llvm_cbe_lenptr);
static unsigned int compile_regex(unsigned int llvm_cbe_options, unsigned int llvm_cbe_oldims, unsigned char **llvm_cbe_codeptr, unsigned char **llvm_cbe_ptrptr, unsigned int *llvm_cbe_errorcodeptr, unsigned int llvm_cbe_lookbehind, unsigned int llvm_cbe_reset_bracount, unsigned int llvm_cbe_skipbytes, unsigned int *llvm_cbe_firstbyteptr, unsigned int *llvm_cbe_reqbyteptr, struct l_struct_2E_branch_chain *llvm_cbe_bcptr, struct l_struct_2E_compile_data *llvm_cbe_cd, unsigned int *llvm_cbe_lengthptr);
unsigned int memcmp(unsigned char *, unsigned char *, unsigned int );
static unsigned int is_anchored(unsigned char *llvm_cbe_code, unsigned int *llvm_cbe_options, unsigned int llvm_cbe_bracket_map, unsigned int llvm_cbe_backref_map);
static unsigned int is_startline(unsigned char *llvm_cbe_code, unsigned int llvm_cbe_bracket_map, unsigned int llvm_cbe_backref_map);
static unsigned int find_firstassertedchar(unsigned char *llvm_cbe_code, unsigned int *llvm_cbe_options, unsigned int llvm_cbe_inassert);
unsigned int strlen(unsigned char *);
void free(unsigned char *);
unsigned char *malloc(unsigned int );
void abort(void);
unsigned char *memmove(unsigned char *, unsigned char *, unsigned int );
unsigned char *memset(unsigned char *, unsigned int , unsigned int );
//unsigned char *memcpy(unsigned char *, unsigned char *, unsigned int );


/* Global Variable Declarations */
static unsigned char _2E_str[41];
static unsigned char _2E_str1[42];
static unsigned char _2E_str2[15];
static unsigned char _2E_str3[24];
static unsigned char digitab[256];
static unsigned int posix_class_maps[42];
static unsigned char *posix_names[14];
static unsigned char _2E_str59[6];
static unsigned char _2E_str60[6];
static unsigned char _2E_str61[6];
static unsigned char _2E_str62[6];
static unsigned char _2E_str63[6];
static unsigned char _2E_str64[6];
static unsigned char _2E_str65[6];
static unsigned char _2E_str66[6];
static unsigned char _2E_str67[6];
static unsigned char _2E_str68[6];
static unsigned char _2E_str69[6];
static unsigned char _2E_str70[6];
static unsigned char _2E_str71[5];
static unsigned char _2E_str72[7];
static unsigned char posix_name_lengths[15];
static unsigned short escapes[75];
static unsigned char _pcre_OP_lengths[104];
static unsigned char _2E_str73[4];
static unsigned char _2E_str74[7];
static unsigned char _pcre_default_tables[1088];


/* Global Variable Definitions and Initialization */
static unsigned char _2E_str[41] = "Usage: %s <pattern size> <subject size>\n";
static unsigned char _2E_str1[42] = "Using pattern size: %d, subject size: %d\n";
static unsigned char _2E_str2[15] = "invalid sizes\n";
static unsigned char _2E_str3[24] = "PCRE compilation failed";
static unsigned char digitab[256] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x00\x00\x00\x00\x00\x00\x00\x08\x08\x08\x08\x08\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x08\x08\x08\x08\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
static unsigned int posix_class_maps[42] = { ((unsigned int )160), ((unsigned int )64), ((unsigned int )-2), ((unsigned int )128), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )96), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )160), ((unsigned int )-1), ((unsigned int )2), ((unsigned int )224), ((unsigned int )288), ((unsigned int )0), ((unsigned int )0), ((unsigned int )-1), ((unsigned int )1), ((unsigned int )288), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )64), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )192), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )224), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )256), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )0), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )160), ((unsigned int )-1), ((unsigned int )0), ((unsigned int )32), ((unsigned int )-1), ((unsigned int )0) };
static unsigned char *posix_names[14] = { (&(_2E_str59[((unsigned int )0)])), (&(_2E_str60[((unsigned int )0)])), (&(_2E_str61[((unsigned int )0)])), (&(_2E_str62[((unsigned int )0)])), (&(_2E_str63[((unsigned int )0)])), (&(_2E_str64[((unsigned int )0)])), (&(_2E_str65[((unsigned int )0)])), (&(_2E_str66[((unsigned int )0)])), (&(_2E_str67[((unsigned int )0)])), (&(_2E_str68[((unsigned int )0)])), (&(_2E_str69[((unsigned int )0)])), (&(_2E_str70[((unsigned int )0)])), (&(_2E_str71[((unsigned int )0)])), (&(_2E_str72[((unsigned int )0)])) };
static unsigned char _2E_str59[6] = "alpha";
static unsigned char _2E_str60[6] = "lower";
static unsigned char _2E_str61[6] = "upper";
static unsigned char _2E_str62[6] = "alnum";
static unsigned char _2E_str63[6] = "ascii";
static unsigned char _2E_str64[6] = "blank";
static unsigned char _2E_str65[6] = "cntrl";
static unsigned char _2E_str66[6] = "digit";
static unsigned char _2E_str67[6] = "graph";
static unsigned char _2E_str68[6] = "print";
static unsigned char _2E_str69[6] = "punct";
static unsigned char _2E_str70[6] = "space";
static unsigned char _2E_str71[5] = "word";
static unsigned char _2E_str72[7] = "xdigit";
static unsigned char posix_name_lengths[15] = "\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x04\x06";
static unsigned short escapes[75] = { ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )58), ((unsigned short )59), ((unsigned short )60), ((unsigned short )61), ((unsigned short )62), ((unsigned short )63), ((unsigned short )64), ((unsigned short )-1), ((unsigned short )-4), ((unsigned short )-13), ((unsigned short )-6), ((unsigned short )-24), ((unsigned short )0), ((unsigned short )-2), ((unsigned short )-17), ((unsigned short )0), ((unsigned short )0), ((unsigned short )-3), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )0), ((unsigned short )-14), ((unsigned short )-25), ((unsigned short )-16), ((unsigned short )-8), ((unsigned short )0), ((unsigned short )0), ((unsigned short )-19), ((unsigned short )-10), ((unsigned short )-21), ((unsigned short )0), ((unsigned short )-22), ((unsigned short )91), ((unsigned short )92), ((unsigned short )93), ((unsigned short )94), ((unsigned short )95), ((unsigned short )96), ((unsigned short )7), ((unsigned short )-5), ((unsigned short )0), ((unsigned short )-7), ((unsigned short )27), ((unsigned short )12), ((unsigned short )0), ((unsigned short )-18), ((unsigned short )0), ((unsigned short )0), ((unsigned short )-26), ((unsigned short )0), ((unsigned short )0), ((unsigned short )10), ((unsigned short )0), ((unsigned short )-15), ((unsigned short )0), ((unsigned short )13), ((unsigned short )-9), ((unsigned short )9), ((unsigned short )0), ((unsigned short )-20), ((unsigned short )-11), ((unsigned short )0), ((unsigned short )0), ((unsigned short )-23) };
static unsigned char _pcre_OP_lengths[104] = { ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )3), ((unsigned char )3), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )2), ((unsigned char )1), ((unsigned char )1), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )4), ((unsigned char )4), ((unsigned char )4), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )4), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )4), ((unsigned char )4), ((unsigned char )4), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )4), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )4), ((unsigned char )4), ((unsigned char )4), ((unsigned char )2), ((unsigned char )2), ((unsigned char )2), ((unsigned char )4), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1), ((unsigned char )5), ((unsigned char )5), ((unsigned char )33), ((unsigned char )33), ((unsigned char )0), ((unsigned char )3), ((unsigned char )3), ((unsigned char )6), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )5), ((unsigned char )3), ((unsigned char )3), ((unsigned char )5), ((unsigned char )3), ((unsigned char )3), ((unsigned char )3), ((unsigned char )1), ((unsigned char )1), ((unsigned char )1) };
static unsigned char _2E_str73[4] = "{0,";
static unsigned char _2E_str74[7] = "DEFINE";
static unsigned char _pcre_default_tables[1088] = "\x00\x01\x02\x03\x04\x05\x06\a\x08\t\n\v\x0C\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF\x00\x01\x02\x03\x04\x05\x06\a\x08\t\n\v\x0C\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz[\\]^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~\x7F\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF\x00>\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\x03~\x00\x00\x00~\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFE\xFF\xFF\a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFE\xFF\xFF\a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\x03\xFE\xFF\xFF\x87\xFE\xFF\xFF\a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFE\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFE\xFF\x00\xFC\x01\x00\x00\xF8\x01\x00\x00x\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x01\x01\x00\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x80\x00\x00\x00\x80\x80\x80\x80\x00\x00\x80\x00\x1C\x1C\x1C\x1C\x1C\x1C\x1C\x1C\x1C\x1C\x00\x00\x00\x00\x00\x80\x00\x1A\x1A\x1A\x1A\x1A\x1A\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x80\x80\x00\x80\x10\x00\x1A\x1A\x1A\x1A\x1A\x1A\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x12\x80\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";


/* Function Bodies */
static inline int llvm_fcmp_ord(double X, double Y) { return X == X && Y == Y; }
static inline int llvm_fcmp_uno(double X, double Y) { return X != X || Y != Y; }
static inline int llvm_fcmp_ueq(double X, double Y) { return X == Y || llvm_fcmp_uno(X, Y); }
static inline int llvm_fcmp_une(double X, double Y) { return X != Y; }
static inline int llvm_fcmp_ult(double X, double Y) { return X <  Y || llvm_fcmp_uno(X, Y); }
static inline int llvm_fcmp_ugt(double X, double Y) { return X >  Y || llvm_fcmp_uno(X, Y); }
static inline int llvm_fcmp_ule(double X, double Y) { return X <= Y || llvm_fcmp_uno(X, Y); }
static inline int llvm_fcmp_uge(double X, double Y) { return X >= Y || llvm_fcmp_uno(X, Y); }
static inline int llvm_fcmp_oeq(double X, double Y) { return X == Y ; }
static inline int llvm_fcmp_one(double X, double Y) { return X != Y && llvm_fcmp_ord(X, Y); }
static inline int llvm_fcmp_olt(double X, double Y) { return X <  Y ; }
static inline int llvm_fcmp_ogt(double X, double Y) { return X >  Y ; }
static inline int llvm_fcmp_ole(double X, double Y) { return X <= Y ; }
static inline int llvm_fcmp_oge(double X, double Y) { return X >= Y ; }

unsigned int main(unsigned int llvm_cbe_argc, unsigned char **llvm_cbe_argv) {
  unsigned int llvm_cbe_length_i_i;    /* Address-exposed local */
  unsigned int llvm_cbe_firstbyte_i_i;    /* Address-exposed local */
  unsigned int llvm_cbe_reqbyte_i_i;    /* Address-exposed local */
  unsigned int llvm_cbe_errorcode_i_i;    /* Address-exposed local */
  unsigned char *llvm_cbe_code_i_i;    /* Address-exposed local */
  unsigned char *llvm_cbe_ptr_i_i;    /* Address-exposed local */
  struct l_struct_2E_compile_data llvm_cbe_compile_block_i_i;    /* Address-exposed local */
  unsigned char llvm_cbe_cworkspace_i_i[4096];    /* Address-exposed local */
  unsigned int llvm_cbe_temp_options_i_i;    /* Address-exposed local */
  unsigned char *llvm_cbe_tmp6;
  struct l_struct_2E__IO_FILE *llvm_cbe_tmp7;
  unsigned int llvm_cbe_tmp9;
  unsigned char *llvm_cbe_tmp13;
  unsigned int llvm_cbe_tmp22;
  unsigned char *llvm_cbe_tmp30;
  unsigned int llvm_cbe_tmp42;
  unsigned int llvm_cbe_tmp51;
  struct l_struct_2E__IO_FILE *llvm_cbe_tmp64;
  unsigned int llvm_cbe_tmp67;
  unsigned char *ltmp_0_1;
  unsigned char *ltmp_1_1;
  unsigned char *ltmp_2_1;
  unsigned char *ltmp_3_1;
  unsigned int llvm_cbe_tmp76;
  unsigned int llvm_cbe_tmp79;
  unsigned char **llvm_cbe_tmp55_i_i;
  unsigned int *llvm_cbe_tmp122_i_i;
  unsigned int *llvm_cbe_tmp124_i_i;
  unsigned int *llvm_cbe_tmp126_i_i;
  unsigned int *llvm_cbe_tmp128_i_i;
  unsigned int *llvm_cbe_tmp130_i_i;
  unsigned char **llvm_cbe_tmp132_i_i;
  unsigned char *llvm_cbe_cworkspace135_i_i;
  unsigned char **llvm_cbe_tmp137_i_i;
  unsigned char **llvm_cbe_tmp140_i_i;
  unsigned int llvm_cbe_tmp146_i_i;
  unsigned int *llvm_cbe_tmp152_i_i;
  unsigned int *llvm_cbe_tmp154_i_i;
  unsigned int *llvm_cbe_tmp156_i_i;
  unsigned int llvm_cbe_tmp168_i_i;
  unsigned int llvm_cbe_tmp169_i_i;
  unsigned int llvm_cbe_tmp175_i_i;
  unsigned int llvm_cbe_tmp183_i_i;
  unsigned int llvm_cbe_tmp186_i_i;
  unsigned int llvm_cbe_tmp191_i_i;
  unsigned char *llvm_cbe_tmp194_i_i;
  struct l_struct_2E_pcre *llvm_cbe_tmp194195_i_i;
  unsigned int llvm_cbe_tmp209_i_i;
  unsigned int *llvm_cbe_tmp211_i_i;
  unsigned short *llvm_cbe_tmp215_i_i;
  unsigned short *llvm_cbe_tmp217_i_i;
  unsigned short *llvm_cbe_tmp219_i_i;
  unsigned int llvm_cbe_tmp222_i_i;
  unsigned short *llvm_cbe_tmp225_i_i;
  unsigned int llvm_cbe_tmp228_i_i;
  unsigned short *llvm_cbe_tmp231_i_i;
  unsigned short llvm_cbe_tmp255_i_i;
  unsigned int llvm_cbe_tmp255256_i_i;
  unsigned short llvm_cbe_tmp265_i_i;
  unsigned short llvm_cbe_tmp269_i_i;
  unsigned int llvm_cbe_tmp271_i_i;
  unsigned char *llvm_cbe_tmp272_i_i;
  unsigned int llvm_cbe_tmp288_i_i;
  unsigned int llvm_cbe_tmp294_i_i;
  unsigned int llvm_cbe_tmp297_i_i;
  unsigned short *llvm_cbe_tmp300_i_i;
  unsigned int llvm_cbe_tmp303_i_i;
  unsigned short *llvm_cbe_tmp306_i_i;
  unsigned int llvm_cbe_tmp309_i_i;
  unsigned int llvm_cbe_tmp316_i_i;
  unsigned int llvm_cbe_tmp321_i_i;
  unsigned char *llvm_cbe_tmp326_i_i;
  unsigned char llvm_cbe_tmp327_i_i;
  unsigned char *llvm_cbe_tmp334_i_i;
  unsigned char *llvm_cbe_tmp335_i_i;
  unsigned int llvm_cbe_tmp338339_i_i;
  unsigned int llvm_cbe_tmp341_i_i;
  unsigned int llvm_cbe_tmp375_i_i;
  unsigned char llvm_cbe_tmp410411412_i_i;
  unsigned char *llvm_cbe_tmp351_i_i;
  unsigned char llvm_cbe_tmp358_i_i;
  unsigned char llvm_cbe_tmp365_i_i;
  unsigned int llvm_cbe_tmp367_i_i;
  unsigned char *llvm_cbe_tmp370_i_i;
  unsigned char llvm_cbe_tmp371_i_i;
  unsigned char *llvm_cbe_tmp377_i_i;
  unsigned char llvm_cbe_tmp378_i_i;
  unsigned char *llvm_cbe_tmp388_i_i;
  unsigned int llvm_cbe_tmp419_i_i;
  unsigned char *llvm_cbe_tmp427_i_i;
  unsigned int llvm_cbe_tmp433_i_i;
  unsigned short llvm_cbe_tmp440_i_i;
  unsigned short llvm_cbe_tmp443_i_i;
  unsigned int llvm_cbe_tmp450_i_i;
  unsigned int llvm_cbe_tmp480_i_i;
  unsigned int llvm_cbe_tmp492_i_i;
  unsigned int llvm_cbe_tmp494_i_i;
  unsigned int llvm_cbe_tmp501_i_i;
  unsigned int llvm_cbe_tmp578724_i_i;
  unsigned int llvm_cbe_tmp506_i_i;
  unsigned int llvm_cbe_tmp512_i_i;
  unsigned int llvm_cbe_tmp514_i_i;
  unsigned int llvm_cbe_tmp520_i_i;
  unsigned char *llvm_cbe_tmp532_i_i;
  unsigned char llvm_cbe_tmp535_i_i;
  unsigned int llvm_cbe_iftmp_595_0_in_i_i;
  unsigned int llvm_cbe_iftmp_595_0_in_i_i__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp554_i_i;
  unsigned int llvm_cbe_tmp578726_i_i;
  unsigned int llvm_cbe_tmp561_i_i;
  unsigned int llvm_cbe_tmp563_i_i;
  unsigned int llvm_cbe_tmp570_i_i;
  unsigned int llvm_cbe_tmp578_i_i;
  unsigned int llvm_cbe_tmp585_i_i;
  unsigned int llvm_cbe_tmp593_i_i;
  unsigned int llvm_cbe_tmp600_i_i;
  unsigned int llvm_cbe_tmp601_i_i;
  unsigned char *llvm_cbe_tmp613_i_i;
  unsigned char llvm_cbe_tmp616_i_i;
  unsigned short llvm_cbe_tmp626_i_i;
  unsigned short llvm_cbe_tmp628629_i_i;
  unsigned short llvm_cbe_iftmp_603_0_i_i;
  unsigned short llvm_cbe_iftmp_603_0_i_i__PHI_TEMPORARY;
  struct l_struct_2E_pcre *llvm_cbe_tmp61_i;
  struct l_struct_2E_pcre *llvm_cbe_tmp61_i__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp96;

  CODE_FOR_MAIN();
  if ((llvm_cbe_argc == 1) || (llvm_cbe_argc == ((unsigned int )3))) {
    goto llvm_cbe_cond_next;
  } else {
    goto llvm_cbe_cond_true;
  }

llvm_cbe_cond_true:
  llvm_cbe_tmp6 = *llvm_cbe_argv;
  llvm_cbe_tmp9 = printf((&(_2E_str[((unsigned int )0)])), llvm_cbe_tmp6);
  return ((unsigned int )1);
llvm_cbe_cond_next:
  // XXX here
  if (llvm_cbe_argc==1) {
    llvm_cbe_tmp22 = 2;
    llvm_cbe_tmp42 = 2;
  } else {
    llvm_cbe_tmp22 = atoi(llvm_cbe_argv[1]); // XXX here!
    llvm_cbe_tmp42 = atoi(llvm_cbe_argv[2]); // XXX here!
  }
  llvm_cbe_tmp51 = printf((&(_2E_str1[((unsigned int )0)])), llvm_cbe_tmp22, llvm_cbe_tmp42);
  if (((((signed int )llvm_cbe_tmp42) < ((signed int )((unsigned int )1))) | (((signed int )llvm_cbe_tmp22) < ((signed int )((unsigned int )1))))) {
    goto llvm_cbe_cond_true63;
  } else {
    goto llvm_cbe_cond_next69;
  }

llvm_cbe_cond_true63:
  llvm_cbe_tmp67 = printf("error: invalid sizes\n");
  return ((unsigned int )1);
llvm_cbe_cond_next69:
  ltmp_0_1 =  /*tail*/ ((unsigned char * (*) ())(void*)malloc)(llvm_cbe_tmp22);
  ltmp_1_1 = ((unsigned char *)ltmp_0_1);
  ltmp_2_1 =  /*tail*/ ((unsigned char * (*) ())(void*)malloc)(llvm_cbe_tmp42);
  ltmp_3_1 = ((unsigned char *)ltmp_2_1);
#ifdef FT
  ft_make_symbolic_array(ltmp_1_1, llvm_cbe_tmp22, "pattern");
  ft_make_symbolic_array(ltmp_3_1, llvm_cbe_tmp42, "source");
#else
  klee_make_symbolic(ltmp_1_1, llvm_cbe_tmp22);
  klee_make_symbolic(ltmp_3_1, llvm_cbe_tmp42);
#endif
  *(&ltmp_1_1[(llvm_cbe_tmp22 + ((unsigned int )-1))]) = ((unsigned char )0);
  *(&ltmp_3_1[(llvm_cbe_tmp42 + ((unsigned int )-1))]) = ((unsigned char )0);
  *(&llvm_cbe_length_i_i) = ((unsigned int )1);
  *(&llvm_cbe_errorcode_i_i) = ((unsigned int )0);
  *(&llvm_cbe_ptr_i_i) = ltmp_1_1;
  *(&llvm_cbe_compile_block_i_i.field0) = (&(_pcre_default_tables[((unsigned int )0)]));
  llvm_cbe_tmp55_i_i = &llvm_cbe_compile_block_i_i.field1;
  *llvm_cbe_tmp55_i_i = (&(_pcre_default_tables[((unsigned int )256)]));
  *(&llvm_cbe_compile_block_i_i.field2) = (&(_pcre_default_tables[((unsigned int )512)]));
  *(&llvm_cbe_compile_block_i_i.field3) = (&(_pcre_default_tables[((unsigned int )832)]));
  *(&llvm_cbe_compile_block_i_i.field18) = ((unsigned int )0);
  *(&llvm_cbe_compile_block_i_i.field19) = ((unsigned int )1);
  *(&llvm_cbe_compile_block_i_i.field20[((unsigned int )0)]) = ((unsigned char )10);
  llvm_cbe_tmp122_i_i = &llvm_cbe_compile_block_i_i.field13;
  *llvm_cbe_tmp122_i_i = ((unsigned int )0);
  llvm_cbe_tmp124_i_i = &llvm_cbe_compile_block_i_i.field14;
  *llvm_cbe_tmp124_i_i = ((unsigned int )0);
  llvm_cbe_tmp126_i_i = &llvm_cbe_compile_block_i_i.field12;
  *llvm_cbe_tmp126_i_i = ((unsigned int )0);
  llvm_cbe_tmp128_i_i = &llvm_cbe_compile_block_i_i.field10;
  *llvm_cbe_tmp128_i_i = ((unsigned int )0);
  llvm_cbe_tmp130_i_i = &llvm_cbe_compile_block_i_i.field11;
  *llvm_cbe_tmp130_i_i = ((unsigned int )0);
  llvm_cbe_tmp132_i_i = &llvm_cbe_compile_block_i_i.field9;
  *llvm_cbe_tmp132_i_i = ((unsigned char *)/*NULL*/0);
  llvm_cbe_cworkspace135_i_i = &llvm_cbe_cworkspace_i_i[((unsigned int )0)];
  *(&llvm_cbe_compile_block_i_i.field4) = llvm_cbe_cworkspace135_i_i;
  llvm_cbe_tmp137_i_i = &llvm_cbe_compile_block_i_i.field5;
  *llvm_cbe_tmp137_i_i = llvm_cbe_cworkspace135_i_i;
  llvm_cbe_tmp140_i_i = &llvm_cbe_compile_block_i_i.field8;
  *llvm_cbe_tmp140_i_i = llvm_cbe_cworkspace135_i_i;
  *(&llvm_cbe_compile_block_i_i.field6) = ltmp_1_1;
  llvm_cbe_tmp146_i_i = strlen(ltmp_1_1);
  *(&llvm_cbe_compile_block_i_i.field7) = (&ltmp_1_1[llvm_cbe_tmp146_i_i]);
  llvm_cbe_tmp152_i_i = &llvm_cbe_compile_block_i_i.field16;
  *llvm_cbe_tmp152_i_i = ((unsigned int )0);
  llvm_cbe_tmp154_i_i = &llvm_cbe_compile_block_i_i.field17;
  *llvm_cbe_tmp154_i_i = ((unsigned int )0);
  llvm_cbe_tmp156_i_i = &llvm_cbe_compile_block_i_i.field15;
  *llvm_cbe_tmp156_i_i = ((unsigned int )0);
  *(&llvm_cbe_code_i_i) = llvm_cbe_cworkspace135_i_i;
  *llvm_cbe_cworkspace135_i_i = ((unsigned char )93);
  llvm_cbe_tmp168_i_i = compile_regex(((unsigned int )0), ((unsigned int )0), (&llvm_cbe_code_i_i), (&llvm_cbe_ptr_i_i), (&llvm_cbe_errorcode_i_i), ((unsigned int )0), ((unsigned int )0), ((unsigned int )0), (&llvm_cbe_firstbyte_i_i), (&llvm_cbe_reqbyte_i_i), ((struct l_struct_2E_branch_chain *)/*NULL*/0), (&llvm_cbe_compile_block_i_i), (&llvm_cbe_length_i_i));
  llvm_cbe_tmp169_i_i = *(&llvm_cbe_errorcode_i_i);
  if ((llvm_cbe_tmp169_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_next174_i_i;
  } else {
    llvm_cbe_tmp61_i__PHI_TEMPORARY = ((struct l_struct_2E_pcre *)/*NULL*/0);   /* for PHI node */
    goto llvm_cbe_pcre_compile_exit;
  }

llvm_cbe_cond_next174_i_i:
  llvm_cbe_tmp175_i_i = *(&llvm_cbe_length_i_i);
  if ((((signed int )llvm_cbe_tmp175_i_i) > ((signed int )((unsigned int )65536)))) {
    goto llvm_cbe_cond_true179_i_i;
  } else {
    goto llvm_cbe_cond_next180_i_i;
  }

llvm_cbe_cond_true179_i_i:
  *(&llvm_cbe_errorcode_i_i) = ((unsigned int )20);
  llvm_cbe_tmp61_i__PHI_TEMPORARY = ((struct l_struct_2E_pcre *)/*NULL*/0);   /* for PHI node */
  goto llvm_cbe_pcre_compile_exit;

llvm_cbe_cond_next180_i_i:
  llvm_cbe_tmp183_i_i = *llvm_cbe_tmp128_i_i;
  llvm_cbe_tmp186_i_i = *llvm_cbe_tmp130_i_i;
  llvm_cbe_tmp191_i_i = (llvm_cbe_tmp175_i_i + ((unsigned int )40)) + ((llvm_cbe_tmp186_i_i + ((unsigned int )3)) * llvm_cbe_tmp183_i_i);
  llvm_cbe_tmp194_i_i = malloc(llvm_cbe_tmp191_i_i);
  llvm_cbe_tmp194195_i_i = ((struct l_struct_2E_pcre *)llvm_cbe_tmp194_i_i);
  if ((llvm_cbe_tmp194_i_i == ((unsigned char *)/*NULL*/0))) {
    goto llvm_cbe_cond_true200_i_i;
  } else {
    goto llvm_cbe_cond_next201_i_i;
  }

llvm_cbe_cond_true200_i_i:
  *(&llvm_cbe_errorcode_i_i) = ((unsigned int )21);
  llvm_cbe_tmp61_i__PHI_TEMPORARY = ((struct l_struct_2E_pcre *)/*NULL*/0);   /* for PHI node */
  goto llvm_cbe_pcre_compile_exit;

llvm_cbe_cond_next201_i_i:
  *(((unsigned int *)llvm_cbe_tmp194_i_i)) = ((unsigned int )1346589253);
  *(&llvm_cbe_tmp194195_i_i->field1) = llvm_cbe_tmp191_i_i;
  llvm_cbe_tmp209_i_i = *llvm_cbe_tmp156_i_i;
  llvm_cbe_tmp211_i_i = &llvm_cbe_tmp194195_i_i->field2;
  *llvm_cbe_tmp211_i_i = llvm_cbe_tmp209_i_i;
  *(&llvm_cbe_tmp194195_i_i->field3) = ((unsigned int )0);
  llvm_cbe_tmp215_i_i = &llvm_cbe_tmp194195_i_i->field6;
  *llvm_cbe_tmp215_i_i = ((unsigned short )0);
  llvm_cbe_tmp217_i_i = &llvm_cbe_tmp194195_i_i->field7;
  *llvm_cbe_tmp217_i_i = ((unsigned short )0);
  llvm_cbe_tmp219_i_i = &llvm_cbe_tmp194195_i_i->field8;
  *llvm_cbe_tmp219_i_i = ((unsigned short )40);
  llvm_cbe_tmp222_i_i = *llvm_cbe_tmp130_i_i;
  llvm_cbe_tmp225_i_i = &llvm_cbe_tmp194195_i_i->field9;
  *llvm_cbe_tmp225_i_i = (((unsigned short )llvm_cbe_tmp222_i_i));
  llvm_cbe_tmp228_i_i = *llvm_cbe_tmp128_i_i;
  llvm_cbe_tmp231_i_i = &llvm_cbe_tmp194195_i_i->field10;
  *llvm_cbe_tmp231_i_i = (((unsigned short )llvm_cbe_tmp228_i_i));
  *(&llvm_cbe_tmp194195_i_i->field11) = ((unsigned short )0);
  *(&llvm_cbe_tmp194195_i_i->field12) = ((unsigned char *)/*NULL*/0);
  *(&llvm_cbe_tmp194195_i_i->field13) = ((unsigned char *)/*NULL*/0);
  *llvm_cbe_tmp126_i_i = ((unsigned int )0);
  *llvm_cbe_tmp128_i_i = ((unsigned int )0);
  llvm_cbe_tmp255_i_i = *llvm_cbe_tmp219_i_i;
  llvm_cbe_tmp255256_i_i = ((unsigned int )(unsigned short )llvm_cbe_tmp255_i_i);
  *llvm_cbe_tmp132_i_i = (&llvm_cbe_tmp194_i_i[llvm_cbe_tmp255256_i_i]);
  llvm_cbe_tmp265_i_i = *llvm_cbe_tmp225_i_i;
  llvm_cbe_tmp269_i_i = *llvm_cbe_tmp231_i_i;
  llvm_cbe_tmp271_i_i = (((unsigned int )(unsigned short )llvm_cbe_tmp269_i_i)) * (((unsigned int )(unsigned short )llvm_cbe_tmp265_i_i));
  llvm_cbe_tmp272_i_i = &llvm_cbe_tmp194_i_i[(llvm_cbe_tmp255256_i_i + llvm_cbe_tmp271_i_i)];
  *llvm_cbe_tmp137_i_i = llvm_cbe_tmp272_i_i;
  *llvm_cbe_tmp140_i_i = llvm_cbe_cworkspace135_i_i;
  *llvm_cbe_tmp152_i_i = ((unsigned int )0);
  *llvm_cbe_tmp154_i_i = ((unsigned int )0);
  *(&llvm_cbe_ptr_i_i) = ltmp_1_1;
  *(&llvm_cbe_code_i_i) = llvm_cbe_tmp272_i_i;
  *llvm_cbe_tmp272_i_i = ((unsigned char )93);
  llvm_cbe_tmp288_i_i = *llvm_cbe_tmp211_i_i;
  llvm_cbe_tmp294_i_i = compile_regex(llvm_cbe_tmp288_i_i, (llvm_cbe_tmp288_i_i & ((unsigned int )7)), (&llvm_cbe_code_i_i), (&llvm_cbe_ptr_i_i), (&llvm_cbe_errorcode_i_i), ((unsigned int )0), ((unsigned int )0), ((unsigned int )0), (&llvm_cbe_firstbyte_i_i), (&llvm_cbe_reqbyte_i_i), ((struct l_struct_2E_branch_chain *)/*NULL*/0), (&llvm_cbe_compile_block_i_i), ((unsigned int *)/*NULL*/0));
  llvm_cbe_tmp297_i_i = *llvm_cbe_tmp126_i_i;
  llvm_cbe_tmp300_i_i = &llvm_cbe_tmp194195_i_i->field4;
  *llvm_cbe_tmp300_i_i = (((unsigned short )llvm_cbe_tmp297_i_i));
  llvm_cbe_tmp303_i_i = *llvm_cbe_tmp122_i_i;
  llvm_cbe_tmp306_i_i = &llvm_cbe_tmp194195_i_i->field5;
  *llvm_cbe_tmp306_i_i = (((unsigned short )llvm_cbe_tmp303_i_i));
  llvm_cbe_tmp309_i_i = *llvm_cbe_tmp154_i_i;
  if ((llvm_cbe_tmp309_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_next320_i_i;
  } else {
    goto llvm_cbe_cond_true313_i_i;
  }

llvm_cbe_cond_true313_i_i:
  llvm_cbe_tmp316_i_i = *llvm_cbe_tmp211_i_i;
  *llvm_cbe_tmp211_i_i = (llvm_cbe_tmp316_i_i | ((unsigned int )2147483648u));
  goto llvm_cbe_cond_next320_i_i;

llvm_cbe_cond_next320_i_i:
  llvm_cbe_tmp321_i_i = *(&llvm_cbe_errorcode_i_i);
  if ((llvm_cbe_tmp321_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_true325_i_i;
  } else {
    goto llvm_cbe_cond_next333_i_i;
  }

llvm_cbe_cond_true325_i_i:
  llvm_cbe_tmp326_i_i = *(&llvm_cbe_ptr_i_i);
  llvm_cbe_tmp327_i_i = *llvm_cbe_tmp326_i_i;
  if ((llvm_cbe_tmp327_i_i == ((unsigned char )0))) {
    goto llvm_cbe_cond_next333_i_i;
  } else {
    goto llvm_cbe_cond_true331_i_i;
  }

llvm_cbe_cond_true331_i_i:
  *(&llvm_cbe_errorcode_i_i) = ((unsigned int )22);
  goto llvm_cbe_cond_next333_i_i;

llvm_cbe_cond_next333_i_i:
  llvm_cbe_tmp334_i_i = *(&llvm_cbe_code_i_i);
  *llvm_cbe_tmp334_i_i = ((unsigned char )0);
  llvm_cbe_tmp335_i_i = &llvm_cbe_tmp334_i_i[((unsigned int )1)];
  *(&llvm_cbe_code_i_i) = llvm_cbe_tmp335_i_i;
  llvm_cbe_tmp338339_i_i = ((unsigned int )(unsigned long)llvm_cbe_tmp272_i_i);
  llvm_cbe_tmp341_i_i = *(&llvm_cbe_length_i_i);
  if ((((signed int )((((unsigned int )(unsigned long)llvm_cbe_tmp335_i_i)) - llvm_cbe_tmp338339_i_i)) > ((signed int )llvm_cbe_tmp341_i_i))) {
    goto llvm_cbe_cond_true345_i_i;
  } else {
    goto llvm_cbe_bb418_preheader_i_i;
  }

llvm_cbe_bb418_preheader_i_i:
  llvm_cbe_tmp375_i_i = llvm_cbe_tmp271_i_i + ((unsigned int )1);
  llvm_cbe_tmp410411412_i_i = ((unsigned char )(unsigned long)llvm_cbe_tmp272_i_i);
  goto llvm_cbe_bb418_i_i;

llvm_cbe_cond_true345_i_i:
  *(&llvm_cbe_errorcode_i_i) = ((unsigned int )23);
  goto llvm_cbe_bb432_i_i;

llvm_cbe_cond_true393_i_i:
  *(&llvm_cbe_errorcode_i_i) = ((unsigned int )53);
  goto llvm_cbe_bb432_i_i;

  do {     /* Syntactic loop 'bb418.i.i' to make GCC happy */
llvm_cbe_bb418_i_i:
  llvm_cbe_tmp419_i_i = *(&llvm_cbe_errorcode_i_i);
  if ((llvm_cbe_tmp419_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_next424_i_i;
  } else {
    goto llvm_cbe_bb432_i_i;
  }

llvm_cbe_cond_false394_i_i:
  *llvm_cbe_tmp370_i_i = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)llvm_cbe_tmp388_i_i)) - llvm_cbe_tmp338339_i_i)) >> ((unsigned int )((unsigned int )8)))))));
  *llvm_cbe_tmp377_i_i = (((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_tmp388_i_i)) - llvm_cbe_tmp410411412_i_i)));
  goto llvm_cbe_bb418_i_i;

llvm_cbe_bb347_i_i:
  llvm_cbe_tmp351_i_i = &llvm_cbe_tmp427_i_i[((unsigned int )-2)];
  *llvm_cbe_tmp140_i_i = llvm_cbe_tmp351_i_i;
  llvm_cbe_tmp358_i_i = *llvm_cbe_tmp351_i_i;
  llvm_cbe_tmp365_i_i = *(&llvm_cbe_tmp427_i_i[((unsigned int )-1)]);
  llvm_cbe_tmp367_i_i = (((unsigned int )(unsigned char )llvm_cbe_tmp365_i_i)) | ((((unsigned int )(unsigned char )llvm_cbe_tmp358_i_i)) << ((unsigned int )8));
  llvm_cbe_tmp370_i_i = &llvm_cbe_tmp194_i_i[(llvm_cbe_tmp255256_i_i + (llvm_cbe_tmp367_i_i + llvm_cbe_tmp271_i_i))];
  llvm_cbe_tmp371_i_i = *llvm_cbe_tmp370_i_i;
  llvm_cbe_tmp377_i_i = &llvm_cbe_tmp194_i_i[(llvm_cbe_tmp255256_i_i + (llvm_cbe_tmp375_i_i + llvm_cbe_tmp367_i_i))];
  llvm_cbe_tmp378_i_i = *llvm_cbe_tmp377_i_i;
  llvm_cbe_tmp388_i_i = find_bracket(llvm_cbe_tmp272_i_i, (((((unsigned int )(unsigned char )llvm_cbe_tmp371_i_i)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp378_i_i))));
  if ((llvm_cbe_tmp388_i_i == ((unsigned char *)/*NULL*/0))) {
    goto llvm_cbe_cond_true393_i_i;
  } else {
    goto llvm_cbe_cond_false394_i_i;
  }

llvm_cbe_cond_next424_i_i:
  llvm_cbe_tmp427_i_i = *llvm_cbe_tmp140_i_i;
  if ((((unsigned char *)llvm_cbe_tmp427_i_i) > ((unsigned char *)llvm_cbe_cworkspace135_i_i))) {
    goto llvm_cbe_bb347_i_i;
  } else {
    goto llvm_cbe_bb432_i_i;
  }

  } while (1); /* end of syntactic loop 'bb418.i.i' */
llvm_cbe_bb432_i_i:
  llvm_cbe_tmp433_i_i = *(&llvm_cbe_errorcode_i_i);
  if ((llvm_cbe_tmp433_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_true437_i_i;
  } else {
    goto llvm_cbe_cond_next449_i_i;
  }

llvm_cbe_cond_true437_i_i:
  llvm_cbe_tmp440_i_i = *llvm_cbe_tmp306_i_i;
  llvm_cbe_tmp443_i_i = *llvm_cbe_tmp300_i_i;
  if ((((unsigned short )llvm_cbe_tmp440_i_i) > ((unsigned short )llvm_cbe_tmp443_i_i))) {
    goto llvm_cbe_cond_true447_i_i;
  } else {
    goto llvm_cbe_cond_next449_i_i;
  }

llvm_cbe_cond_true447_i_i:
  *(&llvm_cbe_errorcode_i_i) = ((unsigned int )15);
  goto llvm_cbe_cond_next449_i_i;

llvm_cbe_cond_next449_i_i:
  llvm_cbe_tmp450_i_i = *(&llvm_cbe_errorcode_i_i);
  if ((llvm_cbe_tmp450_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_next477_i_i;
  } else {
    goto llvm_cbe_cond_true454_i_i;
  }

llvm_cbe_cond_true454_i_i:
  free(llvm_cbe_tmp194_i_i);
  llvm_cbe_tmp61_i__PHI_TEMPORARY = ((struct l_struct_2E_pcre *)/*NULL*/0);   /* for PHI node */
  goto llvm_cbe_pcre_compile_exit;

llvm_cbe_cond_next477_i_i:
  llvm_cbe_tmp480_i_i = *llvm_cbe_tmp211_i_i;
  if (((llvm_cbe_tmp480_i_i & ((unsigned int )16)) == ((unsigned int )0))) {
    goto llvm_cbe_cond_true486_i_i;
  } else {
    goto llvm_cbe_cond_next577_i_i;
  }

llvm_cbe_cond_true486_i_i:
  *(&llvm_cbe_temp_options_i_i) = llvm_cbe_tmp480_i_i;
  llvm_cbe_tmp492_i_i = *llvm_cbe_tmp124_i_i;
  llvm_cbe_tmp494_i_i = is_anchored(llvm_cbe_tmp272_i_i, (&llvm_cbe_temp_options_i_i), ((unsigned int )0), llvm_cbe_tmp492_i_i);
  if ((llvm_cbe_tmp494_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_false505_i_i;
  } else {
    goto llvm_cbe_cond_true498_i_i;
  }

llvm_cbe_cond_true498_i_i:
  llvm_cbe_tmp501_i_i = *llvm_cbe_tmp211_i_i;
  *llvm_cbe_tmp211_i_i = (llvm_cbe_tmp501_i_i | ((unsigned int )16));
  llvm_cbe_tmp578724_i_i = *(&llvm_cbe_reqbyte_i_i);
  if ((((signed int )llvm_cbe_tmp578724_i_i) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true582_i_i;
  } else {
    llvm_cbe_tmp61_i__PHI_TEMPORARY = llvm_cbe_tmp194195_i_i;   /* for PHI node */
    goto llvm_cbe_pcre_compile_exit;
  }

llvm_cbe_cond_false505_i_i:
  llvm_cbe_tmp506_i_i = *(&llvm_cbe_firstbyte_i_i);
  if ((((signed int )llvm_cbe_tmp506_i_i) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true510_i_i;
  } else {
    goto llvm_cbe_cond_next513_i_i;
  }

llvm_cbe_cond_true510_i_i:
  llvm_cbe_tmp512_i_i = find_firstassertedchar(llvm_cbe_tmp272_i_i, (&llvm_cbe_temp_options_i_i), ((unsigned int )0));
  *(&llvm_cbe_firstbyte_i_i) = llvm_cbe_tmp512_i_i;
  goto llvm_cbe_cond_next513_i_i;

llvm_cbe_cond_next513_i_i:
  llvm_cbe_tmp514_i_i = *(&llvm_cbe_firstbyte_i_i);
  if ((((signed int )llvm_cbe_tmp514_i_i) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true518_i_i;
  } else {
    goto llvm_cbe_cond_false558_i_i;
  }

llvm_cbe_cond_true518_i_i:
  llvm_cbe_tmp520_i_i = llvm_cbe_tmp514_i_i & ((unsigned int )255);
  if (((llvm_cbe_tmp514_i_i & ((unsigned int )256)) == ((unsigned int )0))) {
    goto llvm_cbe_bb545_i_i;
  } else {
    goto llvm_cbe_cond_next529_i_i;
  }

llvm_cbe_cond_next529_i_i:
  llvm_cbe_tmp532_i_i = *llvm_cbe_tmp55_i_i;
  llvm_cbe_tmp535_i_i = *(&llvm_cbe_tmp532_i_i[llvm_cbe_tmp520_i_i]);
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp535_i_i)) == llvm_cbe_tmp520_i_i)) {
    llvm_cbe_iftmp_595_0_in_i_i__PHI_TEMPORARY = llvm_cbe_tmp520_i_i;   /* for PHI node */
    goto llvm_cbe_bb548_i_i;
  } else {
    goto llvm_cbe_bb545_i_i;
  }

llvm_cbe_bb545_i_i:
  llvm_cbe_iftmp_595_0_in_i_i__PHI_TEMPORARY = llvm_cbe_tmp514_i_i;   /* for PHI node */
  goto llvm_cbe_bb548_i_i;

llvm_cbe_bb548_i_i:
  llvm_cbe_iftmp_595_0_in_i_i = llvm_cbe_iftmp_595_0_in_i_i__PHI_TEMPORARY;
  *llvm_cbe_tmp215_i_i = (((unsigned short )llvm_cbe_iftmp_595_0_in_i_i));
  llvm_cbe_tmp554_i_i = *llvm_cbe_tmp211_i_i;
  *llvm_cbe_tmp211_i_i = (llvm_cbe_tmp554_i_i | ((unsigned int )1073741824));
  llvm_cbe_tmp578726_i_i = *(&llvm_cbe_reqbyte_i_i);
  if ((((signed int )llvm_cbe_tmp578726_i_i) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true582_i_i;
  } else {
    llvm_cbe_tmp61_i__PHI_TEMPORARY = llvm_cbe_tmp194195_i_i;   /* for PHI node */
    goto llvm_cbe_pcre_compile_exit;
  }

llvm_cbe_cond_false558_i_i:
  llvm_cbe_tmp561_i_i = *llvm_cbe_tmp124_i_i;
  llvm_cbe_tmp563_i_i = is_startline(llvm_cbe_tmp272_i_i, ((unsigned int )0), llvm_cbe_tmp561_i_i);
  if ((llvm_cbe_tmp563_i_i == ((unsigned int )0))) {
    goto llvm_cbe_cond_next577_i_i;
  } else {
    goto llvm_cbe_cond_true567_i_i;
  }

llvm_cbe_cond_true567_i_i:
  llvm_cbe_tmp570_i_i = *llvm_cbe_tmp211_i_i;
  *llvm_cbe_tmp211_i_i = (llvm_cbe_tmp570_i_i | ((unsigned int )268435456));
  goto llvm_cbe_cond_next577_i_i;

llvm_cbe_cond_next577_i_i:
  llvm_cbe_tmp578_i_i = *(&llvm_cbe_reqbyte_i_i);
  if ((((signed int )llvm_cbe_tmp578_i_i) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true582_i_i;
  } else {
    llvm_cbe_tmp61_i__PHI_TEMPORARY = llvm_cbe_tmp194195_i_i;   /* for PHI node */
    goto llvm_cbe_pcre_compile_exit;
  }

llvm_cbe_cond_true582_i_i:
  llvm_cbe_tmp585_i_i = *llvm_cbe_tmp211_i_i;
  if (((llvm_cbe_tmp585_i_i & ((unsigned int )16)) == ((unsigned int )0))) {
    goto llvm_cbe_bb598_i_i;
  } else {
    goto llvm_cbe_cond_next592_i_i;
  }

llvm_cbe_cond_next592_i_i:
  llvm_cbe_tmp593_i_i = *(&llvm_cbe_reqbyte_i_i);
  if (((llvm_cbe_tmp593_i_i & ((unsigned int )512)) == ((unsigned int )0))) {
    llvm_cbe_tmp61_i__PHI_TEMPORARY = llvm_cbe_tmp194195_i_i;   /* for PHI node */
    goto llvm_cbe_pcre_compile_exit;
  } else {
    goto llvm_cbe_bb598_i_i;
  }

llvm_cbe_bb598_i_i:
  llvm_cbe_tmp600_i_i = *(&llvm_cbe_reqbyte_i_i);
  llvm_cbe_tmp601_i_i = llvm_cbe_tmp600_i_i & ((unsigned int )255);
  if (((llvm_cbe_tmp600_i_i & ((unsigned int )256)) == ((unsigned int )0))) {
    goto llvm_cbe_bb627_i_i;
  } else {
    goto llvm_cbe_cond_next610_i_i;
  }

llvm_cbe_cond_next610_i_i:
  llvm_cbe_tmp613_i_i = *llvm_cbe_tmp55_i_i;
  llvm_cbe_tmp616_i_i = *(&llvm_cbe_tmp613_i_i[llvm_cbe_tmp601_i_i]);
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp616_i_i)) == llvm_cbe_tmp601_i_i)) {
    goto llvm_cbe_cond_next623_i_i;
  } else {
    goto llvm_cbe_bb627_i_i;
  }

llvm_cbe_cond_next623_i_i:
  llvm_cbe_tmp626_i_i = ((unsigned short )((((unsigned short )llvm_cbe_tmp600_i_i)) & ((unsigned short )-257)));
  llvm_cbe_iftmp_603_0_i_i__PHI_TEMPORARY = llvm_cbe_tmp626_i_i;   /* for PHI node */
  goto llvm_cbe_bb630_i_i;

llvm_cbe_bb627_i_i:
  llvm_cbe_tmp628629_i_i = ((unsigned short )llvm_cbe_tmp600_i_i);
  llvm_cbe_iftmp_603_0_i_i__PHI_TEMPORARY = llvm_cbe_tmp628629_i_i;   /* for PHI node */
  goto llvm_cbe_bb630_i_i;

llvm_cbe_bb630_i_i:
  llvm_cbe_iftmp_603_0_i_i = llvm_cbe_iftmp_603_0_i_i__PHI_TEMPORARY;
  *llvm_cbe_tmp217_i_i = llvm_cbe_iftmp_603_0_i_i;
  *llvm_cbe_tmp211_i_i = (llvm_cbe_tmp585_i_i | ((unsigned int )536870912));
  llvm_cbe_tmp61_i__PHI_TEMPORARY = llvm_cbe_tmp194195_i_i;   /* for PHI node */
  goto llvm_cbe_pcre_compile_exit;

llvm_cbe_pcre_compile_exit:
  llvm_cbe_tmp61_i = llvm_cbe_tmp61_i__PHI_TEMPORARY;
  if ((llvm_cbe_tmp61_i == ((struct l_struct_2E_pcre *)/*NULL*/0))) {
    goto llvm_cbe_cond_true94;
  } else {
    goto llvm_cbe_UnifiedReturnBlock;
  }

llvm_cbe_cond_true94:
  //  llvm_cbe_tmp96 = puts((&(_2E_str3[((unsigned int )0)])));
  return ((unsigned int )0);
llvm_cbe_UnifiedReturnBlock:
  return ((unsigned int )0);
}


static unsigned int check_escape(unsigned char **llvm_cbe_ptrptr, unsigned int *llvm_cbe_errorcodeptr, unsigned int llvm_cbe_bracount, unsigned int llvm_cbe_options, unsigned int llvm_cbe_isclass) {
  unsigned int llvm_cbe_tmp3;
  unsigned char *llvm_cbe_tmp5;
  unsigned char *llvm_cbe_tmp6;
  unsigned char llvm_cbe_tmp8;
  unsigned int llvm_cbe_tmp89;
  unsigned int llvm_cbe_tmp19;
  unsigned short llvm_cbe_tmp27;
  unsigned int llvm_cbe_tmp2728;
  unsigned char *llvm_cbe_tmp44;
  unsigned char llvm_cbe_tmp45;
  unsigned char *llvm_cbe_tmp51;
  unsigned char llvm_cbe_tmp76521;
  unsigned int llvm_cbe_p_3513_0_rec;
  unsigned int llvm_cbe_p_3513_0_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_p_3513_0;
  unsigned char llvm_cbe_tmp54;
  unsigned char llvm_cbe_tmp63;
  unsigned char *llvm_cbe_tmp73;
  unsigned char llvm_cbe_tmp76;
  unsigned int llvm_cbe_indvar_next573;
  unsigned char *llvm_cbe_p_3513_1;
  unsigned char *llvm_cbe_p_3513_1__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp91;
  bool llvm_cbe_braced_0;
  bool llvm_cbe_braced_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_0;
  unsigned char *llvm_cbe_ptr_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp110;
  unsigned char llvm_cbe_tmp111;
  unsigned char *llvm_cbe_ptr_4_ph;
  unsigned char llvm_cbe_tmp133546;
  unsigned char llvm_cbe_tmp136549;
  unsigned int llvm_cbe_ptr_4544_rec;
  unsigned int llvm_cbe_ptr_4544_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_2543;
  unsigned int llvm_cbe_c_2543__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp124_rec;
  unsigned char *llvm_cbe_tmp124;
  unsigned char llvm_cbe_tmp126;
  unsigned int llvm_cbe_tmp129;
  unsigned char llvm_cbe_tmp133;
  unsigned char llvm_cbe_tmp136;
  unsigned char *llvm_cbe_ptr_4_lcssa;
  unsigned char *llvm_cbe_ptr_4_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_2_lcssa;
  unsigned int llvm_cbe_c_2_lcssa__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp156;
  unsigned char llvm_cbe_tmp158;
  unsigned char *llvm_cbe_ptr_6;
  unsigned char *llvm_cbe_ptr_6__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp217557;
  unsigned char llvm_cbe_tmp220560;
  unsigned int llvm_cbe_ptr_7553_rec;
  unsigned int llvm_cbe_ptr_7553_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_6555;
  unsigned int llvm_cbe_c_6555__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp208;
  unsigned char llvm_cbe_tmp210;
  unsigned int llvm_cbe_tmp212;
  unsigned int llvm_cbe_c_6;
  unsigned char llvm_cbe_tmp217;
  unsigned char llvm_cbe_tmp220;
  unsigned int llvm_cbe_c_6_lcssa;
  unsigned int llvm_cbe_c_6_lcssa__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_7_lcssa;
  unsigned char *llvm_cbe_ptr_7_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_6_in_lcssa;
  unsigned int llvm_cbe_c_6_in_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp267;
  unsigned int llvm_cbe_ptr_10_rec;
  unsigned int llvm_cbe_ptr_10_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_7_in;
  unsigned int llvm_cbe_c_7_in__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp13_sum577;
  unsigned int llvm_cbe_c_7;
  unsigned char llvm_cbe_tmp282;
  unsigned char *llvm_cbe_ptr_10;
  unsigned char llvm_cbe_tmp312;
  unsigned char *llvm_cbe_tmp318;
  unsigned char *llvm_cbe_tmp324;
  unsigned char *llvm_cbe_tmp324_us;
  unsigned char *llvm_cbe_tmp324_lcssa_us_lcssa;
  unsigned char *llvm_cbe_tmp324_lcssa_us_lcssa__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp321322_lcssa_us_lcssa_in;
  unsigned char llvm_cbe_tmp321322_lcssa_us_lcssa_in__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp321322_lcssa_us_lcssa;
  unsigned int llvm_cbe_cc_4;
  unsigned int llvm_cbe_tmp360;
  unsigned int llvm_cbe_indvar_next571;
  unsigned int llvm_cbe_count_3_ph;
  unsigned int llvm_cbe_count_3_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_pt_3_ph;
  unsigned char *llvm_cbe_pt_3_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_9_ph;
  unsigned int llvm_cbe_c_9_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_pt_3_us_rec;
  unsigned int llvm_cbe_pt_3_us_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_pt_3_us;
  unsigned char llvm_cbe_tmp363_us;
  unsigned char llvm_cbe_tmp366_us;
  unsigned int llvm_cbe_tmp324_us_rec;
  unsigned char llvm_cbe_tmp363;
  unsigned char llvm_cbe_tmp366;
  unsigned char llvm_cbe_tmp321_lcssa_us_lcssa;
  unsigned char llvm_cbe_tmp321_lcssa_us_lcssa__PHI_TEMPORARY;
  unsigned char *llvm_cbe_pt_3_lcssa_us_lcssa;
  unsigned char *llvm_cbe_pt_3_lcssa_us_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_cc404_3;
  unsigned int llvm_cbe_tmp430;
  unsigned int llvm_cbe_ptr_11_rec;
  unsigned int llvm_cbe_ptr_11_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_10;
  unsigned int llvm_cbe_c_10__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp13_sum581;
  unsigned char llvm_cbe_tmp444;
  unsigned int llvm_cbe_tmp444445;
  unsigned char llvm_cbe_tmp447;
  unsigned char *llvm_cbe_tmp456;
  unsigned char llvm_cbe_tmp458;
  unsigned int llvm_cbe_tmp458459;
  unsigned char *llvm_cbe_ptr_11;
  unsigned int llvm_cbe_c_0;
  unsigned int llvm_cbe_c_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_1;
  unsigned char *llvm_cbe_ptr_1__PHI_TEMPORARY;

  llvm_cbe_tmp3 = (((unsigned int )(((unsigned int )llvm_cbe_options) >> ((unsigned int )((unsigned int )11))))) & ((unsigned int )1);
  llvm_cbe_tmp5 = *llvm_cbe_ptrptr;
  llvm_cbe_tmp6 = &llvm_cbe_tmp5[((unsigned int )1)];
  llvm_cbe_tmp8 = *llvm_cbe_tmp6;
  llvm_cbe_tmp89 = ((unsigned int )(unsigned char )llvm_cbe_tmp8);
  if ((llvm_cbe_tmp8 == ((unsigned char )0))) {
    goto llvm_cbe_cond_true;
  } else {
    goto llvm_cbe_cond_false;
  }

llvm_cbe_cond_true:
  *llvm_cbe_errorcodeptr = ((unsigned int )1);
  *llvm_cbe_ptrptr = llvm_cbe_tmp6;
  return llvm_cbe_tmp89;
llvm_cbe_cond_false:
  llvm_cbe_tmp19 = llvm_cbe_tmp89 + ((unsigned int )-48);
  if ((((unsigned int )llvm_cbe_tmp19) < ((unsigned int )((unsigned int )75)))) {
    goto llvm_cbe_cond_true23;
  } else {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_tmp89;   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp6;   /* for PHI node */
    goto llvm_cbe_cond_next494;
  }

llvm_cbe_cond_true23:
  llvm_cbe_tmp27 = *(&escapes[llvm_cbe_tmp19]);
  llvm_cbe_tmp2728 = ((signed int )(signed short )llvm_cbe_tmp27);
  if ((llvm_cbe_tmp27 == ((unsigned short )0))) {
    goto llvm_cbe_cond_false35;
  } else {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_tmp2728;   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp6;   /* for PHI node */
    goto llvm_cbe_cond_next494;
  }

llvm_cbe_cond_false35:
  switch (llvm_cbe_tmp89) {
  default:
    goto llvm_cbe_bb479;
;
  case ((unsigned int )48):
    llvm_cbe_ptr_10_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_c_7_in__PHI_TEMPORARY = llvm_cbe_tmp89;   /* for PHI node */
    goto llvm_cbe_bb269;
  case ((unsigned int )49):
    goto llvm_cbe_bb195;
  case ((unsigned int )50):
    goto llvm_cbe_bb195;
  case ((unsigned int )51):
    goto llvm_cbe_bb195;
  case ((unsigned int )52):
    goto llvm_cbe_bb195;
  case ((unsigned int )53):
    goto llvm_cbe_bb195;
  case ((unsigned int )54):
    goto llvm_cbe_bb195;
  case ((unsigned int )55):
    goto llvm_cbe_bb195;
  case ((unsigned int )56):
    goto llvm_cbe_bb195;
  case ((unsigned int )57):
    goto llvm_cbe_bb195;
  case ((unsigned int )76):
    goto llvm_cbe_bb40;
    break;
  case ((unsigned int )78):
    goto llvm_cbe_bb40;
    break;
  case ((unsigned int )85):
    goto llvm_cbe_bb40;
    break;
  case ((unsigned int )99):
    goto llvm_cbe_bb454;
  case ((unsigned int )103):
    goto llvm_cbe_bb42;
  case ((unsigned int )108):
    goto llvm_cbe_bb40;
    break;
  case ((unsigned int )117):
    goto llvm_cbe_bb40;
    break;
  case ((unsigned int )120):
    goto llvm_cbe_bb309;
  }
llvm_cbe_bb40:
  *llvm_cbe_errorcodeptr = ((unsigned int )37);
  *llvm_cbe_ptrptr = llvm_cbe_tmp6;
  return llvm_cbe_tmp89;
llvm_cbe_bb42:
  llvm_cbe_tmp44 = &llvm_cbe_tmp5[((unsigned int )2)];
  llvm_cbe_tmp45 = *llvm_cbe_tmp44;
  if ((llvm_cbe_tmp45 == ((unsigned char )123))) {
    goto llvm_cbe_cond_true49;
  } else {
    llvm_cbe_braced_0__PHI_TEMPORARY = 1;   /* for PHI node */
    llvm_cbe_ptr_0__PHI_TEMPORARY = llvm_cbe_tmp6;   /* for PHI node */
    goto llvm_cbe_cond_next108;
  }

llvm_cbe_cond_true49:
  llvm_cbe_tmp51 = &llvm_cbe_tmp5[((unsigned int )3)];
  llvm_cbe_tmp76521 = *llvm_cbe_tmp51;
  switch (llvm_cbe_tmp76521) {
  default:
    llvm_cbe_p_3513_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb52;
;
  case ((unsigned char )0):
    llvm_cbe_p_3513_1__PHI_TEMPORARY = llvm_cbe_tmp51;   /* for PHI node */
    goto llvm_cbe_bb88;
  case ((unsigned char )125):
    llvm_cbe_p_3513_1__PHI_TEMPORARY = llvm_cbe_tmp51;   /* for PHI node */
    goto llvm_cbe_bb88;
  }
  do {     /* Syntactic loop 'bb52' to make GCC happy */
llvm_cbe_bb52:
  llvm_cbe_p_3513_0_rec = llvm_cbe_p_3513_0_rec__PHI_TEMPORARY;
  llvm_cbe_p_3513_0 = &llvm_cbe_tmp5[(llvm_cbe_p_3513_0_rec + ((unsigned int )3))];
  llvm_cbe_tmp54 = *llvm_cbe_p_3513_0;
  if ((llvm_cbe_tmp54 == ((unsigned char )45))) {
    goto llvm_cbe_cond_next71;
  } else {
    goto llvm_cbe_cond_true58;
  }

llvm_cbe_cond_next71:
  llvm_cbe_tmp73 = &llvm_cbe_tmp5[(llvm_cbe_p_3513_0_rec + ((unsigned int )4))];
  llvm_cbe_tmp76 = *llvm_cbe_tmp73;
  llvm_cbe_indvar_next573 = llvm_cbe_p_3513_0_rec + ((unsigned int )1);
  switch (llvm_cbe_tmp76) {
  default:
    llvm_cbe_p_3513_0_rec__PHI_TEMPORARY = llvm_cbe_indvar_next573;   /* for PHI node */
    goto llvm_cbe_bb52;
;
  case ((unsigned char )0):
    llvm_cbe_p_3513_1__PHI_TEMPORARY = llvm_cbe_tmp73;   /* for PHI node */
    goto llvm_cbe_bb88;
    break;
  case ((unsigned char )125):
    llvm_cbe_p_3513_1__PHI_TEMPORARY = llvm_cbe_tmp73;   /* for PHI node */
    goto llvm_cbe_bb88;
    break;
  }
llvm_cbe_cond_true58:
  llvm_cbe_tmp63 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp54))]);
  if (((((unsigned char )(llvm_cbe_tmp63 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p_3513_1__PHI_TEMPORARY = llvm_cbe_p_3513_0;   /* for PHI node */
    goto llvm_cbe_bb88;
  } else {
    goto llvm_cbe_cond_next71;
  }

  } while (1); /* end of syntactic loop 'bb52' */
llvm_cbe_bb88:
  llvm_cbe_p_3513_1 = llvm_cbe_p_3513_1__PHI_TEMPORARY;
  llvm_cbe_tmp91 = *llvm_cbe_p_3513_1;
  switch (llvm_cbe_tmp91) {
  default:
    llvm_cbe_c_0__PHI_TEMPORARY = ((unsigned int )-26);   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp6;   /* for PHI node */
    goto llvm_cbe_cond_next494;
;
  case ((unsigned char )0):
    llvm_cbe_braced_0__PHI_TEMPORARY = 0;   /* for PHI node */
    llvm_cbe_ptr_0__PHI_TEMPORARY = llvm_cbe_tmp44;   /* for PHI node */
    goto llvm_cbe_cond_next108;
    break;
  case ((unsigned char )125):
    llvm_cbe_braced_0__PHI_TEMPORARY = 0;   /* for PHI node */
    llvm_cbe_ptr_0__PHI_TEMPORARY = llvm_cbe_tmp44;   /* for PHI node */
    goto llvm_cbe_cond_next108;
    break;
  }
llvm_cbe_cond_next108:
  llvm_cbe_braced_0 = llvm_cbe_braced_0__PHI_TEMPORARY;
  llvm_cbe_ptr_0 = llvm_cbe_ptr_0__PHI_TEMPORARY;
  llvm_cbe_tmp110 = &llvm_cbe_ptr_0[((unsigned int )1)];
  llvm_cbe_tmp111 = *llvm_cbe_tmp110;
  llvm_cbe_ptr_4_ph = (((llvm_cbe_tmp111 == ((unsigned char )45))) ? (llvm_cbe_tmp110) : (llvm_cbe_ptr_0));
  llvm_cbe_tmp133546 = *(&llvm_cbe_ptr_4_ph[((unsigned int )1)]);
  llvm_cbe_tmp136549 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp133546))]);
  if (((((unsigned char )(llvm_cbe_tmp136549 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_ptr_4_lcssa__PHI_TEMPORARY = llvm_cbe_ptr_4_ph;   /* for PHI node */
    llvm_cbe_c_2_lcssa__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb142;
  } else {
    llvm_cbe_ptr_4544_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_c_2543__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb120;
  }

  do {     /* Syntactic loop 'bb120' to make GCC happy */
llvm_cbe_bb120:
  llvm_cbe_ptr_4544_rec = llvm_cbe_ptr_4544_rec__PHI_TEMPORARY;
  llvm_cbe_c_2543 = llvm_cbe_c_2543__PHI_TEMPORARY;
  llvm_cbe_tmp124_rec = llvm_cbe_ptr_4544_rec + ((unsigned int )1);
  llvm_cbe_tmp124 = &llvm_cbe_ptr_4_ph[llvm_cbe_tmp124_rec];
  llvm_cbe_tmp126 = *llvm_cbe_tmp124;
  llvm_cbe_tmp129 = ((llvm_cbe_c_2543 * ((unsigned int )10)) + ((unsigned int )-48)) + (((unsigned int )(unsigned char )llvm_cbe_tmp126));
  llvm_cbe_tmp133 = *(&llvm_cbe_ptr_4_ph[(llvm_cbe_ptr_4544_rec + ((unsigned int )2))]);
  llvm_cbe_tmp136 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp133))]);
  if (((((unsigned char )(llvm_cbe_tmp136 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_ptr_4_lcssa__PHI_TEMPORARY = llvm_cbe_tmp124;   /* for PHI node */
    llvm_cbe_c_2_lcssa__PHI_TEMPORARY = llvm_cbe_tmp129;   /* for PHI node */
    goto llvm_cbe_bb142;
  } else {
    llvm_cbe_ptr_4544_rec__PHI_TEMPORARY = llvm_cbe_tmp124_rec;   /* for PHI node */
    llvm_cbe_c_2543__PHI_TEMPORARY = llvm_cbe_tmp129;   /* for PHI node */
    goto llvm_cbe_bb120;
  }

  } while (1); /* end of syntactic loop 'bb120' */
llvm_cbe_bb142:
  llvm_cbe_ptr_4_lcssa = llvm_cbe_ptr_4_lcssa__PHI_TEMPORARY;
  llvm_cbe_c_2_lcssa = llvm_cbe_c_2_lcssa__PHI_TEMPORARY;
  if ((llvm_cbe_c_2_lcssa == ((unsigned int )0))) {
    goto llvm_cbe_bb162;
  } else {
    goto llvm_cbe_cond_next148;
  }

llvm_cbe_cond_next148:
  if (llvm_cbe_braced_0) {
    llvm_cbe_ptr_6__PHI_TEMPORARY = llvm_cbe_ptr_4_lcssa;   /* for PHI node */
    goto llvm_cbe_bb165;
  } else {
    goto llvm_cbe_cond_next154;
  }

llvm_cbe_cond_next154:
  llvm_cbe_tmp156 = &llvm_cbe_ptr_4_lcssa[((unsigned int )1)];
  llvm_cbe_tmp158 = *llvm_cbe_tmp156;
  if ((llvm_cbe_tmp158 == ((unsigned char )125))) {
    llvm_cbe_ptr_6__PHI_TEMPORARY = llvm_cbe_tmp156;   /* for PHI node */
    goto llvm_cbe_bb165;
  } else {
    goto llvm_cbe_bb162;
  }

llvm_cbe_bb162:
  *llvm_cbe_errorcodeptr = ((unsigned int )57);
  return ((unsigned int )0);
llvm_cbe_bb165:
  llvm_cbe_ptr_6 = llvm_cbe_ptr_6__PHI_TEMPORARY;
  if ((llvm_cbe_tmp111 == ((unsigned char )45))) {
    goto llvm_cbe_cond_true170;
  } else {
    goto llvm_cbe_cond_next184;
  }

llvm_cbe_cond_true170:
  if ((((signed int )llvm_cbe_c_2_lcssa) > ((signed int )llvm_cbe_bracount))) {
    goto llvm_cbe_cond_true176;
  } else {
    goto llvm_cbe_cond_next179;
  }

llvm_cbe_cond_true176:
  *llvm_cbe_errorcodeptr = ((unsigned int )15);
  return ((unsigned int )0);
llvm_cbe_cond_next179:
  *llvm_cbe_ptrptr = llvm_cbe_ptr_6;
  return ((((unsigned int )-28) - llvm_cbe_bracount) + llvm_cbe_c_2_lcssa);
llvm_cbe_cond_next184:
  *llvm_cbe_ptrptr = llvm_cbe_ptr_6;
  return (((unsigned int )-27) - llvm_cbe_c_2_lcssa);
llvm_cbe_bb195:
  if ((llvm_cbe_isclass == ((unsigned int )0))) {
    goto llvm_cbe_bb214_preheader;
  } else {
    goto llvm_cbe_cond_next244;
  }

llvm_cbe_bb214_preheader:
  llvm_cbe_tmp217557 = *(&llvm_cbe_tmp5[((unsigned int )2)]);
  llvm_cbe_tmp220560 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp217557))]);
  if (((((unsigned char )(llvm_cbe_tmp220560 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_c_6_lcssa__PHI_TEMPORARY = llvm_cbe_tmp19;   /* for PHI node */
    llvm_cbe_ptr_7_lcssa__PHI_TEMPORARY = llvm_cbe_tmp6;   /* for PHI node */
    llvm_cbe_c_6_in_lcssa__PHI_TEMPORARY = llvm_cbe_tmp89;   /* for PHI node */
    goto llvm_cbe_bb226;
  } else {
    llvm_cbe_ptr_7553_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_c_6555__PHI_TEMPORARY = llvm_cbe_tmp19;   /* for PHI node */
    goto llvm_cbe_bb204;
  }

  do {     /* Syntactic loop 'bb204' to make GCC happy */
llvm_cbe_bb204:
  llvm_cbe_ptr_7553_rec = llvm_cbe_ptr_7553_rec__PHI_TEMPORARY;
  llvm_cbe_c_6555 = llvm_cbe_c_6555__PHI_TEMPORARY;
  llvm_cbe_tmp208 = &llvm_cbe_tmp5[(llvm_cbe_ptr_7553_rec + ((unsigned int )2))];
  llvm_cbe_tmp210 = *llvm_cbe_tmp208;
  llvm_cbe_tmp212 = (((unsigned int )(unsigned char )llvm_cbe_tmp210)) + (llvm_cbe_c_6555 * ((unsigned int )10));
  llvm_cbe_c_6 = llvm_cbe_tmp212 + ((unsigned int )-48);
  llvm_cbe_tmp217 = *(&llvm_cbe_tmp5[(llvm_cbe_ptr_7553_rec + ((unsigned int )3))]);
  llvm_cbe_tmp220 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp217))]);
  if (((((unsigned char )(llvm_cbe_tmp220 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_c_6_lcssa__PHI_TEMPORARY = llvm_cbe_c_6;   /* for PHI node */
    llvm_cbe_ptr_7_lcssa__PHI_TEMPORARY = llvm_cbe_tmp208;   /* for PHI node */
    llvm_cbe_c_6_in_lcssa__PHI_TEMPORARY = llvm_cbe_tmp212;   /* for PHI node */
    goto llvm_cbe_bb226;
  } else {
    llvm_cbe_ptr_7553_rec__PHI_TEMPORARY = (llvm_cbe_ptr_7553_rec + ((unsigned int )1));   /* for PHI node */
    llvm_cbe_c_6555__PHI_TEMPORARY = llvm_cbe_c_6;   /* for PHI node */
    goto llvm_cbe_bb204;
  }

  } while (1); /* end of syntactic loop 'bb204' */
llvm_cbe_bb226:
  llvm_cbe_c_6_lcssa = llvm_cbe_c_6_lcssa__PHI_TEMPORARY;
  llvm_cbe_ptr_7_lcssa = llvm_cbe_ptr_7_lcssa__PHI_TEMPORARY;
  llvm_cbe_c_6_in_lcssa = llvm_cbe_c_6_in_lcssa__PHI_TEMPORARY;
  if (((((signed int )llvm_cbe_c_6_lcssa) < ((signed int )((unsigned int )10))) | (((signed int )llvm_cbe_c_6_lcssa) <= ((signed int )llvm_cbe_bracount)))) {
    goto llvm_cbe_cond_true239;
  } else {
    goto llvm_cbe_cond_next244;
  }

llvm_cbe_cond_true239:
  *llvm_cbe_ptrptr = llvm_cbe_ptr_7_lcssa;
  return (((unsigned int )21) - llvm_cbe_c_6_in_lcssa);
llvm_cbe_cond_next244:
  if ((((signed int )llvm_cbe_tmp89) > ((signed int )((unsigned int )55)))) {
    llvm_cbe_c_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_cond_next494;
  } else {
    llvm_cbe_ptr_10_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_c_7_in__PHI_TEMPORARY = llvm_cbe_tmp89;   /* for PHI node */
    goto llvm_cbe_bb269;
  }

  do {     /* Syntactic loop 'bb269' to make GCC happy */
llvm_cbe_bb269:
  llvm_cbe_ptr_10_rec = llvm_cbe_ptr_10_rec__PHI_TEMPORARY;
  llvm_cbe_c_7_in = llvm_cbe_c_7_in__PHI_TEMPORARY;
  llvm_cbe_tmp13_sum577 = llvm_cbe_ptr_10_rec + ((unsigned int )1);
  llvm_cbe_c_7 = llvm_cbe_c_7_in + ((unsigned int )-48);
  if ((((signed int )(llvm_cbe_ptr_10_rec + llvm_cbe_tmp2728)) < ((signed int )((unsigned int )2)))) {
    goto llvm_cbe_cond_next279;
  } else {
    goto llvm_cbe_bb294;
  }

llvm_cbe_bb259:
  llvm_cbe_tmp267 = (((unsigned int )(unsigned char )llvm_cbe_tmp282)) + (llvm_cbe_c_7 << ((unsigned int )3));
  llvm_cbe_ptr_10_rec__PHI_TEMPORARY = llvm_cbe_tmp13_sum577;   /* for PHI node */
  llvm_cbe_c_7_in__PHI_TEMPORARY = llvm_cbe_tmp267;   /* for PHI node */
  goto llvm_cbe_bb269;

llvm_cbe_cond_next279:
  llvm_cbe_tmp282 = *(&llvm_cbe_tmp5[(llvm_cbe_ptr_10_rec + ((unsigned int )2))]);
  if ((((unsigned char )(((unsigned char )(llvm_cbe_tmp282 + ((unsigned char )-48))))) < ((unsigned char )((unsigned char )8)))) {
    goto llvm_cbe_bb259;
  } else {
    goto llvm_cbe_bb294;
  }

  } while (1); /* end of syntactic loop 'bb269' */
llvm_cbe_bb294:
  llvm_cbe_ptr_10 = &llvm_cbe_tmp5[llvm_cbe_tmp13_sum577];
  if (((((signed int )llvm_cbe_c_7) > ((signed int )((unsigned int )255))) & (llvm_cbe_tmp3 == ((unsigned int )0)))) {
    goto llvm_cbe_cond_true306;
  } else {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_c_7;   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_ptr_10;   /* for PHI node */
    goto llvm_cbe_cond_next494;
  }

llvm_cbe_cond_true306:
  *llvm_cbe_errorcodeptr = ((unsigned int )51);
  *llvm_cbe_ptrptr = llvm_cbe_ptr_10;
  return llvm_cbe_c_7;
llvm_cbe_bb309:
  llvm_cbe_tmp312 = *(&llvm_cbe_tmp5[((unsigned int )2)]);
  if ((llvm_cbe_tmp312 == ((unsigned char )123))) {
    goto llvm_cbe_cond_true316;
  } else {
    llvm_cbe_ptr_11_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_c_10__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb431;
  }

llvm_cbe_cond_true316:
  llvm_cbe_tmp318 = &llvm_cbe_tmp5[((unsigned int )3)];
  llvm_cbe_count_3_ph__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_pt_3_ph__PHI_TEMPORARY = llvm_cbe_tmp318;   /* for PHI node */
  llvm_cbe_c_9_ph__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb361_outer;

  do {     /* Syntactic loop 'bb361.outer' to make GCC happy */
llvm_cbe_bb361_outer:
  llvm_cbe_count_3_ph = llvm_cbe_count_3_ph__PHI_TEMPORARY;
  llvm_cbe_pt_3_ph = llvm_cbe_pt_3_ph__PHI_TEMPORARY;
  llvm_cbe_c_9_ph = llvm_cbe_c_9_ph__PHI_TEMPORARY;
  if ((llvm_cbe_c_9_ph == ((unsigned int )0))) {
    llvm_cbe_pt_3_us_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb361_us;
  } else {
    goto llvm_cbe_bb361;
  }

llvm_cbe_cond_next337_split:
  llvm_cbe_tmp324_lcssa_us_lcssa = llvm_cbe_tmp324_lcssa_us_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp321322_lcssa_us_lcssa_in = llvm_cbe_tmp321322_lcssa_us_lcssa_in__PHI_TEMPORARY;
  llvm_cbe_tmp321322_lcssa_us_lcssa = ((unsigned int )(unsigned char )llvm_cbe_tmp321322_lcssa_us_lcssa_in);
  llvm_cbe_cc_4 = ((((((signed int )llvm_cbe_tmp321322_lcssa_us_lcssa) > ((signed int )((unsigned int )96)))) ? (((unsigned int )-32)) : (((unsigned int )0)))) + llvm_cbe_tmp321322_lcssa_us_lcssa;
  llvm_cbe_tmp360 = (llvm_cbe_cc_4 + (llvm_cbe_c_9_ph << ((unsigned int )4))) - ((((((signed int )llvm_cbe_cc_4) < ((signed int )((unsigned int )65)))) ? (((unsigned int )48)) : (((unsigned int )55))));
  llvm_cbe_indvar_next571 = llvm_cbe_count_3_ph + ((unsigned int )1);
  llvm_cbe_count_3_ph__PHI_TEMPORARY = llvm_cbe_indvar_next571;   /* for PHI node */
  llvm_cbe_pt_3_ph__PHI_TEMPORARY = llvm_cbe_tmp324_lcssa_us_lcssa;   /* for PHI node */
  llvm_cbe_c_9_ph__PHI_TEMPORARY = llvm_cbe_tmp360;   /* for PHI node */
  goto llvm_cbe_bb361_outer;

llvm_cbe_bb319:
  llvm_cbe_tmp324 = &llvm_cbe_pt_3_ph[((unsigned int )1)];
  llvm_cbe_tmp324_lcssa_us_lcssa__PHI_TEMPORARY = llvm_cbe_tmp324;   /* for PHI node */
  llvm_cbe_tmp321322_lcssa_us_lcssa_in__PHI_TEMPORARY = llvm_cbe_tmp363;   /* for PHI node */
  goto llvm_cbe_cond_next337_split;

llvm_cbe_bb361:
  llvm_cbe_tmp363 = *llvm_cbe_pt_3_ph;
  llvm_cbe_tmp366 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp363))]);
  if (((((unsigned char )(llvm_cbe_tmp366 & ((unsigned char )8)))) == ((unsigned char )0))) {
    llvm_cbe_tmp321_lcssa_us_lcssa__PHI_TEMPORARY = llvm_cbe_tmp363;   /* for PHI node */
    llvm_cbe_pt_3_lcssa_us_lcssa__PHI_TEMPORARY = llvm_cbe_pt_3_ph;   /* for PHI node */
    goto llvm_cbe_bb372_split;
  } else {
    goto llvm_cbe_bb319;
  }

llvm_cbe_cond_next337_split_loopexit:
  llvm_cbe_tmp324_us = &llvm_cbe_pt_3_ph[llvm_cbe_tmp324_us_rec];
  llvm_cbe_tmp324_lcssa_us_lcssa__PHI_TEMPORARY = llvm_cbe_tmp324_us;   /* for PHI node */
  llvm_cbe_tmp321322_lcssa_us_lcssa_in__PHI_TEMPORARY = llvm_cbe_tmp363_us;   /* for PHI node */
  goto llvm_cbe_cond_next337_split;

  do {     /* Syntactic loop 'bb361.us' to make GCC happy */
llvm_cbe_bb361_us:
  llvm_cbe_pt_3_us_rec = llvm_cbe_pt_3_us_rec__PHI_TEMPORARY;
  llvm_cbe_pt_3_us = &llvm_cbe_pt_3_ph[llvm_cbe_pt_3_us_rec];
  llvm_cbe_tmp363_us = *llvm_cbe_pt_3_us;
  llvm_cbe_tmp366_us = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp363_us))]);
  if (((((unsigned char )(llvm_cbe_tmp366_us & ((unsigned char )8)))) == ((unsigned char )0))) {
    llvm_cbe_tmp321_lcssa_us_lcssa__PHI_TEMPORARY = llvm_cbe_tmp363_us;   /* for PHI node */
    llvm_cbe_pt_3_lcssa_us_lcssa__PHI_TEMPORARY = llvm_cbe_pt_3_us;   /* for PHI node */
    goto llvm_cbe_bb372_split;
  } else {
    goto llvm_cbe_bb319_us;
  }

llvm_cbe_bb319_us:
  llvm_cbe_tmp324_us_rec = llvm_cbe_pt_3_us_rec + ((unsigned int )1);
  if ((llvm_cbe_tmp363_us == ((unsigned char )48))) {
    llvm_cbe_pt_3_us_rec__PHI_TEMPORARY = llvm_cbe_tmp324_us_rec;   /* for PHI node */
    goto llvm_cbe_bb361_us;
  } else {
    goto llvm_cbe_cond_next337_split_loopexit;
  }

  } while (1); /* end of syntactic loop 'bb361.us' */
  } while (1); /* end of syntactic loop 'bb361.outer' */
llvm_cbe_bb372_split:
  llvm_cbe_tmp321_lcssa_us_lcssa = llvm_cbe_tmp321_lcssa_us_lcssa__PHI_TEMPORARY;
  llvm_cbe_pt_3_lcssa_us_lcssa = llvm_cbe_pt_3_lcssa_us_lcssa__PHI_TEMPORARY;
  if ((llvm_cbe_tmp321_lcssa_us_lcssa == ((unsigned char )125))) {
    goto llvm_cbe_cond_true378;
  } else {
    llvm_cbe_ptr_11_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_c_10__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb431;
  }

llvm_cbe_cond_true378:
  if ((((signed int )llvm_cbe_c_9_ph) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_bb397;
  } else {
    goto llvm_cbe_cond_next384;
  }

llvm_cbe_cond_next384:
  if ((((signed int )((((llvm_cbe_tmp3 == ((unsigned int )0))) ? (((unsigned int )2)) : (((unsigned int )8))))) < ((signed int )llvm_cbe_count_3_ph))) {
    goto llvm_cbe_bb397;
  } else {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_c_9_ph;   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_pt_3_lcssa_us_lcssa;   /* for PHI node */
    goto llvm_cbe_cond_next494;
  }

llvm_cbe_bb397:
  *llvm_cbe_errorcodeptr = ((unsigned int )34);
  *llvm_cbe_ptrptr = llvm_cbe_pt_3_lcssa_us_lcssa;
  return llvm_cbe_c_9_ph;
  do {     /* Syntactic loop 'bb431' to make GCC happy */
llvm_cbe_bb431:
  llvm_cbe_ptr_11_rec = llvm_cbe_ptr_11_rec__PHI_TEMPORARY;
  llvm_cbe_c_10 = llvm_cbe_c_10__PHI_TEMPORARY;
  llvm_cbe_tmp13_sum581 = llvm_cbe_ptr_11_rec + ((unsigned int )1);
  if ((((signed int )(llvm_cbe_ptr_11_rec + llvm_cbe_tmp2728)) < ((signed int )((unsigned int )2)))) {
    goto llvm_cbe_cond_next441;
  } else {
    goto llvm_cbe_cond_next494_loopexit;
  }

llvm_cbe_bb403:
  llvm_cbe_cc404_3 = ((((((signed int )llvm_cbe_tmp444445) > ((signed int )((unsigned int )96)))) ? (((unsigned int )-32)) : (((unsigned int )0)))) + llvm_cbe_tmp444445;
  llvm_cbe_tmp430 = (llvm_cbe_cc404_3 + (llvm_cbe_c_10 << ((unsigned int )4))) - ((((((signed int )llvm_cbe_cc404_3) < ((signed int )((unsigned int )65)))) ? (((unsigned int )48)) : (((unsigned int )55))));
  llvm_cbe_ptr_11_rec__PHI_TEMPORARY = llvm_cbe_tmp13_sum581;   /* for PHI node */
  llvm_cbe_c_10__PHI_TEMPORARY = llvm_cbe_tmp430;   /* for PHI node */
  goto llvm_cbe_bb431;

llvm_cbe_cond_next441:
  llvm_cbe_tmp444 = *(&llvm_cbe_tmp5[(llvm_cbe_ptr_11_rec + ((unsigned int )2))]);
  llvm_cbe_tmp444445 = ((unsigned int )(unsigned char )llvm_cbe_tmp444);
  llvm_cbe_tmp447 = *(&digitab[llvm_cbe_tmp444445]);
  if (((((unsigned char )(llvm_cbe_tmp447 & ((unsigned char )8)))) == ((unsigned char )0))) {
    goto llvm_cbe_cond_next494_loopexit;
  } else {
    goto llvm_cbe_bb403;
  }

  } while (1); /* end of syntactic loop 'bb431' */
llvm_cbe_bb454:
  llvm_cbe_tmp456 = &llvm_cbe_tmp5[((unsigned int )2)];
  llvm_cbe_tmp458 = *llvm_cbe_tmp456;
  llvm_cbe_tmp458459 = ((unsigned int )(unsigned char )llvm_cbe_tmp458);
  if ((llvm_cbe_tmp458 == ((unsigned char )0))) {
    goto llvm_cbe_cond_true464;
  } else {
    goto llvm_cbe_cond_next467;
  }

llvm_cbe_cond_true464:
  *llvm_cbe_errorcodeptr = ((unsigned int )2);
  return ((unsigned int )0);
llvm_cbe_cond_next467:
  *llvm_cbe_ptrptr = llvm_cbe_tmp456;
  return ((((((((unsigned int )(llvm_cbe_tmp458459 + ((unsigned int )-97))) < ((unsigned int )((unsigned int )26)))) ? (((unsigned int )-32)) : (((unsigned int )0)))) + llvm_cbe_tmp458459) ^ ((unsigned int )64));
llvm_cbe_bb479:
  if (((llvm_cbe_options & ((unsigned int )64)) == ((unsigned int )0))) {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_tmp89;   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp6;   /* for PHI node */
    goto llvm_cbe_cond_next494;
  } else {
    goto llvm_cbe_cond_true485;
  }

llvm_cbe_cond_true485:
  *llvm_cbe_errorcodeptr = ((unsigned int )3);
  *llvm_cbe_ptrptr = llvm_cbe_tmp6;
  return llvm_cbe_tmp89;
llvm_cbe_cond_next494_loopexit:
  llvm_cbe_ptr_11 = &llvm_cbe_tmp5[llvm_cbe_tmp13_sum581];
  llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_c_10;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_ptr_11;   /* for PHI node */
  goto llvm_cbe_cond_next494;

llvm_cbe_cond_next494:
  llvm_cbe_c_0 = llvm_cbe_c_0__PHI_TEMPORARY;
  llvm_cbe_ptr_1 = llvm_cbe_ptr_1__PHI_TEMPORARY;
  *llvm_cbe_ptrptr = llvm_cbe_ptr_1;
  return llvm_cbe_c_0;
}


static unsigned int find_parens(unsigned char *llvm_cbe_ptr, unsigned int llvm_cbe_count, unsigned char *llvm_cbe_name, unsigned int llvm_cbe_lorn, unsigned int llvm_cbe_xmode) {
  unsigned char llvm_cbe_tmp334386;
  unsigned int llvm_cbe_count_addr_2364_0;
  unsigned int llvm_cbe_count_addr_2364_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_addr_2372_0;
  unsigned char *llvm_cbe_ptr_addr_2372_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp3;
  unsigned char *llvm_cbe_tmp7;
  unsigned char llvm_cbe_tmp9;
  unsigned char *llvm_cbe_ptr_addr_3;
  unsigned char *llvm_cbe_ptr_addr_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp25;
  unsigned char llvm_cbe_tmp27;
  unsigned char *llvm_cbe_tmp49;
  unsigned char llvm_cbe_tmp51;
  unsigned char *llvm_cbe_tmp74;
  unsigned char llvm_cbe_tmp76;
  unsigned char *llvm_cbe_ptr_addr_5;
  unsigned char *llvm_cbe_ptr_addr_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp93;
  unsigned char llvm_cbe_tmp95;
  unsigned char *llvm_cbe_tmp116;
  unsigned char llvm_cbe_tmp118;
  unsigned char *llvm_cbe_ptr_addr_4;
  unsigned char *llvm_cbe_ptr_addr_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp129;
  unsigned char llvm_cbe_tmp131;
  unsigned int llvm_cbe_ptr_addr_6_rec;
  unsigned int llvm_cbe_ptr_addr_6_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp151_rec;
  unsigned char *llvm_cbe_tmp151;
  unsigned char llvm_cbe_tmp153;
  unsigned char llvm_cbe_tmp184;
  unsigned int llvm_cbe_tmp190;
  unsigned char *llvm_cbe_tmp209;
  unsigned char llvm_cbe_tmp211;
  unsigned char *llvm_cbe_tmp217;
  unsigned char *llvm_cbe_ptr_addr_7;
  unsigned char *llvm_cbe_ptr_addr_7__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp220;
  unsigned char llvm_cbe_tmp228;
  unsigned int llvm_cbe_tmp250;
  unsigned char *llvm_cbe_tmp271;
  unsigned int llvm_cbe_term_3;
  unsigned char llvm_cbe_tmp284395;
  unsigned int llvm_cbe_ptr_addr_8388_0_rec;
  unsigned int llvm_cbe_ptr_addr_8388_0_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp281;
  unsigned char llvm_cbe_tmp284;
  unsigned char *llvm_cbe_ptr_addr_8388_1;
  unsigned char *llvm_cbe_ptr_addr_8388_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp317;
  unsigned char *llvm_cbe_ptr_addr_0;
  unsigned char *llvm_cbe_ptr_addr_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_count_addr_0;
  unsigned int llvm_cbe_count_addr_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp331;
  unsigned char llvm_cbe_tmp334;
  unsigned int llvm_cbe_retval_0;
  unsigned int llvm_cbe_retval_0__PHI_TEMPORARY;

  llvm_cbe_tmp334386 = *llvm_cbe_ptr;
  if ((llvm_cbe_tmp334386 == ((unsigned char )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_bb_preheader;
  }

llvm_cbe_bb_preheader:
  llvm_cbe_count_addr_2364_0__PHI_TEMPORARY = llvm_cbe_count;   /* for PHI node */
  llvm_cbe_ptr_addr_2372_0__PHI_TEMPORARY = llvm_cbe_ptr;   /* for PHI node */
  goto llvm_cbe_bb;

  do {     /* Syntactic loop 'bb' to make GCC happy */
llvm_cbe_bb:
  llvm_cbe_count_addr_2364_0 = llvm_cbe_count_addr_2364_0__PHI_TEMPORARY;
  llvm_cbe_ptr_addr_2372_0 = llvm_cbe_ptr_addr_2372_0__PHI_TEMPORARY;
  llvm_cbe_tmp3 = *llvm_cbe_ptr_addr_2372_0;
  switch (llvm_cbe_tmp3) {
  default:
    goto llvm_cbe_cond_next136;
;
  case ((unsigned char )92):
    goto llvm_cbe_cond_true;
    break;
  case ((unsigned char )91):
    llvm_cbe_ptr_addr_4__PHI_TEMPORARY = llvm_cbe_ptr_addr_2372_0;   /* for PHI node */
    goto llvm_cbe_bb127;
  }
llvm_cbe_bb332:
  llvm_cbe_ptr_addr_0 = llvm_cbe_ptr_addr_0__PHI_TEMPORARY;
  llvm_cbe_count_addr_0 = llvm_cbe_count_addr_0__PHI_TEMPORARY;
  llvm_cbe_tmp331 = &llvm_cbe_ptr_addr_0[((unsigned int )1)];
  llvm_cbe_tmp334 = *llvm_cbe_tmp331;
  if ((llvm_cbe_tmp334 == ((unsigned char )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    llvm_cbe_count_addr_2364_0__PHI_TEMPORARY = llvm_cbe_count_addr_0;   /* for PHI node */
    llvm_cbe_ptr_addr_2372_0__PHI_TEMPORARY = llvm_cbe_tmp331;   /* for PHI node */
    goto llvm_cbe_bb;
  }

llvm_cbe_cond_true:
  llvm_cbe_tmp7 = &llvm_cbe_ptr_addr_2372_0[((unsigned int )1)];
  llvm_cbe_tmp9 = *llvm_cbe_tmp7;
  switch (llvm_cbe_tmp9) {
  default:
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_tmp7;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_count_addr_2364_0;   /* for PHI node */
    goto llvm_cbe_bb332;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )81):
    llvm_cbe_ptr_addr_3__PHI_TEMPORARY = llvm_cbe_tmp7;   /* for PHI node */
    goto llvm_cbe_bb23;
    break;
  }
  do {     /* Syntactic loop 'bb23' to make GCC happy */
llvm_cbe_bb23:
  llvm_cbe_ptr_addr_3 = llvm_cbe_ptr_addr_3__PHI_TEMPORARY;
  llvm_cbe_tmp25 = &llvm_cbe_ptr_addr_3[((unsigned int )1)];
  llvm_cbe_tmp27 = *llvm_cbe_tmp25;
  switch (llvm_cbe_tmp27) {
  default:
    llvm_cbe_ptr_addr_3__PHI_TEMPORARY = llvm_cbe_tmp25;   /* for PHI node */
    goto llvm_cbe_bb23;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )92):
    goto llvm_cbe_cond_next47;
    break;
  }
llvm_cbe_cond_next47:
  llvm_cbe_tmp49 = &llvm_cbe_ptr_addr_3[((unsigned int )2)];
  llvm_cbe_tmp51 = *llvm_cbe_tmp49;
  if ((llvm_cbe_tmp51 == ((unsigned char )69))) {
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_tmp49;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_count_addr_2364_0;   /* for PHI node */
    goto llvm_cbe_bb332;
  } else {
    llvm_cbe_ptr_addr_3__PHI_TEMPORARY = llvm_cbe_tmp49;   /* for PHI node */
    goto llvm_cbe_bb23;
  }

  } while (1); /* end of syntactic loop 'bb23' */
  do {     /* Syntactic loop 'bb127' to make GCC happy */
llvm_cbe_bb127:
  llvm_cbe_ptr_addr_4 = llvm_cbe_ptr_addr_4__PHI_TEMPORARY;
  llvm_cbe_tmp129 = &llvm_cbe_ptr_addr_4[((unsigned int )1)];
  llvm_cbe_tmp131 = *llvm_cbe_tmp129;
  switch (llvm_cbe_tmp131) {
  default:
    llvm_cbe_ptr_addr_4__PHI_TEMPORARY = llvm_cbe_tmp129;   /* for PHI node */
    goto llvm_cbe_bb127;
;
  case ((unsigned char )93):
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_tmp129;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_count_addr_2364_0;   /* for PHI node */
    goto llvm_cbe_bb332;
  case ((unsigned char )92):
    goto llvm_cbe_cond_true72;
  }
llvm_cbe_cond_true72:
  llvm_cbe_tmp74 = &llvm_cbe_ptr_addr_4[((unsigned int )2)];
  llvm_cbe_tmp76 = *llvm_cbe_tmp74;
  switch (llvm_cbe_tmp76) {
  default:
    llvm_cbe_ptr_addr_4__PHI_TEMPORARY = llvm_cbe_tmp74;   /* for PHI node */
    goto llvm_cbe_bb127;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )81):
    llvm_cbe_ptr_addr_5__PHI_TEMPORARY = llvm_cbe_tmp74;   /* for PHI node */
    goto llvm_cbe_bb91;
    break;
  }
  do {     /* Syntactic loop 'bb91' to make GCC happy */
llvm_cbe_bb91:
  llvm_cbe_ptr_addr_5 = llvm_cbe_ptr_addr_5__PHI_TEMPORARY;
  llvm_cbe_tmp93 = &llvm_cbe_ptr_addr_5[((unsigned int )1)];
  llvm_cbe_tmp95 = *llvm_cbe_tmp93;
  switch (llvm_cbe_tmp95) {
  default:
    llvm_cbe_ptr_addr_5__PHI_TEMPORARY = llvm_cbe_tmp93;   /* for PHI node */
    goto llvm_cbe_bb91;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )92):
    goto llvm_cbe_cond_next114;
    break;
  }
llvm_cbe_cond_next114:
  llvm_cbe_tmp116 = &llvm_cbe_ptr_addr_5[((unsigned int )2)];
  llvm_cbe_tmp118 = *llvm_cbe_tmp116;
  if ((llvm_cbe_tmp118 == ((unsigned char )69))) {
    llvm_cbe_ptr_addr_4__PHI_TEMPORARY = llvm_cbe_tmp116;   /* for PHI node */
    goto llvm_cbe_bb127;
  } else {
    llvm_cbe_ptr_addr_5__PHI_TEMPORARY = llvm_cbe_tmp116;   /* for PHI node */
    goto llvm_cbe_bb91;
  }

  } while (1); /* end of syntactic loop 'bb91' */
  } while (1); /* end of syntactic loop 'bb127' */
  do {     /* Syntactic loop 'bb149' to make GCC happy */
llvm_cbe_bb149:
  llvm_cbe_ptr_addr_6_rec = llvm_cbe_ptr_addr_6_rec__PHI_TEMPORARY;
  llvm_cbe_tmp151_rec = llvm_cbe_ptr_addr_6_rec + ((unsigned int )1);
  llvm_cbe_tmp151 = &llvm_cbe_ptr_addr_2372_0[llvm_cbe_tmp151_rec];
  llvm_cbe_tmp153 = *llvm_cbe_tmp151;
  switch (llvm_cbe_tmp153) {
  default:
    llvm_cbe_ptr_addr_6_rec__PHI_TEMPORARY = llvm_cbe_tmp151_rec;   /* for PHI node */
    goto llvm_cbe_bb149;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )10):
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_tmp151;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_count_addr_2364_0;   /* for PHI node */
    goto llvm_cbe_bb332;
  }
  } while (1); /* end of syntactic loop 'bb149' */
llvm_cbe_cond_next136:
  if (((llvm_cbe_tmp3 == ((unsigned char )35)) & (llvm_cbe_xmode != ((unsigned int )0)))) {
    llvm_cbe_ptr_addr_6_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb149;
  } else {
    goto llvm_cbe_cond_next174;
  }

llvm_cbe_cond_next174:
  if ((llvm_cbe_tmp3 == ((unsigned char )40))) {
    goto llvm_cbe_cond_next181;
  } else {
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_ptr_addr_2372_0;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_count_addr_2364_0;   /* for PHI node */
    goto llvm_cbe_bb332;
  }

llvm_cbe_cond_true188:
  llvm_cbe_tmp190 = llvm_cbe_count_addr_2364_0 + ((unsigned int )1);
  if (((llvm_cbe_tmp190 == llvm_cbe_lorn) & (llvm_cbe_name == ((unsigned char *)/*NULL*/0)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = llvm_cbe_tmp190;   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_ptr_addr_2372_0;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_tmp190;   /* for PHI node */
    goto llvm_cbe_bb332;
  }

llvm_cbe_cond_next181:
  llvm_cbe_tmp184 = *(&llvm_cbe_ptr_addr_2372_0[((unsigned int )1)]);
  if ((llvm_cbe_tmp184 == ((unsigned char )63))) {
    goto llvm_cbe_cond_next207;
  } else {
    goto llvm_cbe_cond_true188;
  }

llvm_cbe_bb240:
  if ((llvm_cbe_tmp220 == ((unsigned char )39))) {
    goto llvm_cbe_bb248;
  } else {
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_ptr_addr_7;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_count_addr_2364_0;   /* for PHI node */
    goto llvm_cbe_bb332;
  }

llvm_cbe_cond_next218:
  llvm_cbe_ptr_addr_7 = llvm_cbe_ptr_addr_7__PHI_TEMPORARY;
  llvm_cbe_tmp220 = *llvm_cbe_ptr_addr_7;
  if ((llvm_cbe_tmp220 == ((unsigned char )60))) {
    goto llvm_cbe_cond_next225;
  } else {
    goto llvm_cbe_bb240;
  }

llvm_cbe_cond_next207:
  llvm_cbe_tmp209 = &llvm_cbe_ptr_addr_2372_0[((unsigned int )2)];
  llvm_cbe_tmp211 = *llvm_cbe_tmp209;
  if ((llvm_cbe_tmp211 == ((unsigned char )80))) {
    goto llvm_cbe_cond_true215;
  } else {
    llvm_cbe_ptr_addr_7__PHI_TEMPORARY = llvm_cbe_tmp209;   /* for PHI node */
    goto llvm_cbe_cond_next218;
  }

llvm_cbe_cond_true215:
  llvm_cbe_tmp217 = &llvm_cbe_ptr_addr_2372_0[((unsigned int )3)];
  llvm_cbe_ptr_addr_7__PHI_TEMPORARY = llvm_cbe_tmp217;   /* for PHI node */
  goto llvm_cbe_cond_next218;

llvm_cbe_cond_next225:
  llvm_cbe_tmp228 = *(&llvm_cbe_ptr_addr_7[((unsigned int )1)]);
  switch (llvm_cbe_tmp228) {
  default:
    goto llvm_cbe_bb248;
;
  case ((unsigned char )33):
    goto llvm_cbe_bb240;
    break;
  case ((unsigned char )61):
    goto llvm_cbe_bb240;
    break;
  }
llvm_cbe_bb290:
  llvm_cbe_ptr_addr_8388_1 = llvm_cbe_ptr_addr_8388_1__PHI_TEMPORARY;
  if ((llvm_cbe_name == ((unsigned char *)/*NULL*/0))) {
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_ptr_addr_8388_1;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_tmp250;   /* for PHI node */
    goto llvm_cbe_bb332;
  } else {
    goto llvm_cbe_cond_true295;
  }

llvm_cbe_cond_next266:
  llvm_cbe_tmp271 = &llvm_cbe_ptr_addr_7[((unsigned int )1)];
  llvm_cbe_term_3 = (((llvm_cbe_tmp220 == ((unsigned char )60))) ? (((unsigned int )62)) : ((((unsigned int )(unsigned char )llvm_cbe_tmp220))));
  llvm_cbe_tmp284395 = *llvm_cbe_tmp271;
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp284395)) == llvm_cbe_term_3)) {
    llvm_cbe_ptr_addr_8388_1__PHI_TEMPORARY = llvm_cbe_tmp271;   /* for PHI node */
    goto llvm_cbe_bb290;
  } else {
    llvm_cbe_ptr_addr_8388_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb279;
  }

llvm_cbe_bb248:
  llvm_cbe_tmp250 = llvm_cbe_count_addr_2364_0 + ((unsigned int )1);
  if (((llvm_cbe_tmp250 == llvm_cbe_lorn) & (llvm_cbe_name == ((unsigned char *)/*NULL*/0)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = llvm_cbe_tmp250;   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next266;
  }

  do {     /* Syntactic loop 'bb279' to make GCC happy */
llvm_cbe_bb279:
  llvm_cbe_ptr_addr_8388_0_rec = llvm_cbe_ptr_addr_8388_0_rec__PHI_TEMPORARY;
  llvm_cbe_tmp281 = &llvm_cbe_ptr_addr_7[(llvm_cbe_ptr_addr_8388_0_rec + ((unsigned int )2))];
  llvm_cbe_tmp284 = *llvm_cbe_tmp281;
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp284)) == llvm_cbe_term_3)) {
    llvm_cbe_ptr_addr_8388_1__PHI_TEMPORARY = llvm_cbe_tmp281;   /* for PHI node */
    goto llvm_cbe_bb290;
  } else {
    llvm_cbe_ptr_addr_8388_0_rec__PHI_TEMPORARY = (llvm_cbe_ptr_addr_8388_0_rec + ((unsigned int )1));   /* for PHI node */
    goto llvm_cbe_bb279;
  }

  } while (1); /* end of syntactic loop 'bb279' */
llvm_cbe_cond_true295:
  if ((((((unsigned int )(unsigned long)llvm_cbe_ptr_addr_8388_1)) - (((unsigned int )(unsigned long)llvm_cbe_tmp271))) == llvm_cbe_lorn)) {
    goto llvm_cbe_cond_false;
  } else {
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_ptr_addr_8388_1;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_tmp250;   /* for PHI node */
    goto llvm_cbe_bb332;
  }

llvm_cbe_cond_false:
  llvm_cbe_tmp317 =  /*tail*/ strncmp(llvm_cbe_name, llvm_cbe_tmp271, llvm_cbe_lorn);
  if ((llvm_cbe_tmp317 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = llvm_cbe_tmp250;   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    llvm_cbe_ptr_addr_0__PHI_TEMPORARY = llvm_cbe_ptr_addr_8388_1;   /* for PHI node */
    llvm_cbe_count_addr_0__PHI_TEMPORARY = llvm_cbe_tmp250;   /* for PHI node */
    goto llvm_cbe_bb332;
  }

  } while (1); /* end of syntactic loop 'bb' */
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
}


static unsigned char *first_significant_code(unsigned char *llvm_cbe_code, unsigned int *llvm_cbe_options, unsigned int llvm_cbe_optbit, unsigned int llvm_cbe_skipassert) {
  unsigned char llvm_cbe_tmp2;
  unsigned char *llvm_cbe_code_addr_099_0_ph;
  unsigned char *llvm_cbe_code_addr_099_0_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_addr_099_0_us;
  unsigned char *llvm_cbe_code_addr_099_0_us__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp10_us;
  unsigned int llvm_cbe_tmp1011_us;
  unsigned int llvm_cbe_tmp15_us;
  unsigned char *llvm_cbe_tmp30_us;
  unsigned char llvm_cbe_tmp2112_us;
  unsigned char *llvm_cbe_tmp3098_us;
  unsigned char llvm_cbe_tmp2109_us;
  unsigned char *llvm_cbe_code_addr_099_0;
  unsigned char *llvm_cbe_code_addr_099_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp30;
  unsigned char llvm_cbe_tmp2112;
  unsigned char *llvm_cbe_code_addr_099_1_ph153;
  unsigned char *llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_addr_099_1_ph;
  unsigned char *llvm_cbe_code_addr_099_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_addr_1_rec;
  unsigned int llvm_cbe_code_addr_1_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp45;
  unsigned char llvm_cbe_tmp50;
  unsigned int llvm_cbe_tmp52;
  unsigned int llvm_cbe_tmp54_rec;
  unsigned char llvm_cbe_tmp56;
  unsigned char llvm_cbe_tmp65;
  unsigned char *llvm_cbe_tmp69;
  unsigned char llvm_cbe_tmp2115;
  unsigned char *llvm_cbe_code_addr_099_2;
  unsigned char *llvm_cbe_code_addr_099_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_addr_099_3_ph_ph;
  unsigned char *llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_addr_099_3_rec;
  unsigned int llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp85;
  unsigned char llvm_cbe_tmp88;
  unsigned int llvm_cbe_tmp91_rec;
  unsigned char *llvm_cbe_tmp91;
  unsigned char llvm_cbe_tmp2118;
  unsigned char *llvm_cbe_code_addr_099_4;
  unsigned char *llvm_cbe_code_addr_099_4__PHI_TEMPORARY;

  llvm_cbe_tmp2 = *llvm_cbe_code;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp2))) {
  default:
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )4):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )5):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )24):
    llvm_cbe_code_addr_099_0_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    goto llvm_cbe_bb4_preheader;
    break;
  case ((unsigned int )82):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )88):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )89):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )90):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )99):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )100):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )101):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  }
llvm_cbe_bb4_preheader:
  llvm_cbe_code_addr_099_0_ph = llvm_cbe_code_addr_099_0_ph__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_optbit) > ((signed int )((unsigned int )0)))) {
    llvm_cbe_code_addr_099_0_us__PHI_TEMPORARY = llvm_cbe_code_addr_099_0_ph;   /* for PHI node */
    goto llvm_cbe_bb4_us;
  } else {
    llvm_cbe_code_addr_099_0__PHI_TEMPORARY = llvm_cbe_code_addr_099_0_ph;   /* for PHI node */
    goto llvm_cbe_cond_next28;
  }

  do {     /* Syntactic loop 'bb4.us' to make GCC happy */
llvm_cbe_bb4_us:
  llvm_cbe_code_addr_099_0_us = llvm_cbe_code_addr_099_0_us__PHI_TEMPORARY;
  llvm_cbe_tmp10_us = *(&llvm_cbe_code_addr_099_0_us[((unsigned int )1)]);
  llvm_cbe_tmp1011_us = ((unsigned int )(unsigned char )llvm_cbe_tmp10_us);
  llvm_cbe_tmp15_us = *llvm_cbe_options;
  if ((((llvm_cbe_tmp1011_us ^ llvm_cbe_tmp15_us) & llvm_cbe_optbit) == ((unsigned int )0))) {
    goto llvm_cbe_cond_next28_us;
  } else {
    goto llvm_cbe_cond_true21_us;
  }

llvm_cbe_cond_next28_us:
  llvm_cbe_tmp30_us = &llvm_cbe_code_addr_099_0_us[((unsigned int )2)];
  llvm_cbe_tmp2112_us = *llvm_cbe_tmp30_us;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp2112_us))) {
  default:
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )4):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )5):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )24):
    llvm_cbe_code_addr_099_0_us__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    goto llvm_cbe_bb4_us;
  case ((unsigned int )82):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )88):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )89):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )90):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )99):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )100):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )101):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  }
llvm_cbe_cond_true21_us:
  *llvm_cbe_options = llvm_cbe_tmp1011_us;
  llvm_cbe_tmp3098_us = &llvm_cbe_code_addr_099_0_us[((unsigned int )2)];
  llvm_cbe_tmp2109_us = *llvm_cbe_tmp3098_us;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp2109_us))) {
  default:
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )4):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )5):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )24):
    llvm_cbe_code_addr_099_0_us__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    goto llvm_cbe_bb4_us;
  case ((unsigned int )82):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )88):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )89):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )90):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )99):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )100):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )101):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp3098_us;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  }
  } while (1); /* end of syntactic loop 'bb4.us' */
  do {     /* Syntactic loop 'cond_next28' to make GCC happy */
llvm_cbe_cond_next28:
  llvm_cbe_code_addr_099_0 = llvm_cbe_code_addr_099_0__PHI_TEMPORARY;
  llvm_cbe_tmp30 = &llvm_cbe_code_addr_099_0[((unsigned int )2)];
  llvm_cbe_tmp2112 = *llvm_cbe_tmp30;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp2112))) {
  default:
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )4):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )5):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )24):
    llvm_cbe_code_addr_099_0__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    goto llvm_cbe_cond_next28;
  case ((unsigned int )82):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )88):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
    break;
  case ((unsigned int )89):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
    break;
  case ((unsigned int )90):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
    break;
  case ((unsigned int )99):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )100):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )101):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  }
  } while (1); /* end of syntactic loop 'cond_next28' */
llvm_cbe_bb33_preheader:
  llvm_cbe_code_addr_099_1_ph153 = llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY;
  if ((llvm_cbe_skipassert == ((unsigned int )0))) {
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_code_addr_099_1_ph153;   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    llvm_cbe_code_addr_099_1_ph__PHI_TEMPORARY = llvm_cbe_code_addr_099_1_ph153;   /* for PHI node */
    llvm_cbe_code_addr_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb42;
  }

  do {     /* Syntactic loop 'bb42' to make GCC happy */
llvm_cbe_bb42:
  llvm_cbe_code_addr_099_1_ph = llvm_cbe_code_addr_099_1_ph__PHI_TEMPORARY;
  llvm_cbe_code_addr_1_rec = llvm_cbe_code_addr_1_rec__PHI_TEMPORARY;
  llvm_cbe_tmp45 = *(&llvm_cbe_code_addr_099_1_ph[(llvm_cbe_code_addr_1_rec + ((unsigned int )1))]);
  llvm_cbe_tmp50 = *(&llvm_cbe_code_addr_099_1_ph[(llvm_cbe_code_addr_1_rec + ((unsigned int )2))]);
  llvm_cbe_tmp52 = ((((unsigned int )(unsigned char )llvm_cbe_tmp45)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp50));
  llvm_cbe_tmp54_rec = llvm_cbe_code_addr_1_rec + llvm_cbe_tmp52;
  llvm_cbe_tmp56 = *(&llvm_cbe_code_addr_099_1_ph[llvm_cbe_tmp54_rec]);
  if ((llvm_cbe_tmp56 == ((unsigned char )83))) {
    llvm_cbe_code_addr_099_1_ph__PHI_TEMPORARY = llvm_cbe_code_addr_099_1_ph;   /* for PHI node */
    llvm_cbe_code_addr_1_rec__PHI_TEMPORARY = llvm_cbe_tmp54_rec;   /* for PHI node */
    goto llvm_cbe_bb42;
  } else {
    goto llvm_cbe_bb60;
  }

llvm_cbe_bb60:
  llvm_cbe_tmp65 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp56))]);
  llvm_cbe_tmp69 = &llvm_cbe_code_addr_099_1_ph[(llvm_cbe_code_addr_1_rec + ((((unsigned int )(unsigned char )llvm_cbe_tmp65)) + llvm_cbe_tmp52))];
  llvm_cbe_tmp2115 = *llvm_cbe_tmp69;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp2115))) {
  default:
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )4):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    goto llvm_cbe_bb71;
    break;
  case ((unsigned int )5):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    goto llvm_cbe_bb71;
    break;
  case ((unsigned int )24):
    llvm_cbe_code_addr_099_0_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    goto llvm_cbe_bb4_preheader;
  case ((unsigned int )82):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )88):
    llvm_cbe_code_addr_099_1_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    llvm_cbe_code_addr_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb42;
  case ((unsigned int )89):
    llvm_cbe_code_addr_099_1_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    llvm_cbe_code_addr_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb42;
  case ((unsigned int )90):
    llvm_cbe_code_addr_099_1_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    llvm_cbe_code_addr_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb42;
  case ((unsigned int )99):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )100):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )101):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp69;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  }
  } while (1); /* end of syntactic loop 'bb42' */
llvm_cbe_bb71:
  llvm_cbe_code_addr_099_2 = llvm_cbe_code_addr_099_2__PHI_TEMPORARY;
  if ((llvm_cbe_skipassert == ((unsigned int )0))) {
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_code_addr_099_2;   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code_addr_099_2;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb83;
  }

  do {     /* Syntactic loop 'bb83' to make GCC happy */
llvm_cbe_bb83:
  llvm_cbe_code_addr_099_3_ph_ph = llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY;
  llvm_cbe_code_addr_099_3_rec = llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY;
  llvm_cbe_tmp85 = *(&llvm_cbe_code_addr_099_3_ph_ph[llvm_cbe_code_addr_099_3_rec]);
  llvm_cbe_tmp88 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp85))]);
  llvm_cbe_tmp91_rec = llvm_cbe_code_addr_099_3_rec + (((unsigned int )(unsigned char )llvm_cbe_tmp88));
  llvm_cbe_tmp91 = &llvm_cbe_code_addr_099_3_ph_ph[llvm_cbe_tmp91_rec];
  llvm_cbe_tmp2118 = *llvm_cbe_tmp91;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp2118))) {
  default:
    llvm_cbe_code_addr_099_4__PHI_TEMPORARY = llvm_cbe_tmp91;   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )4):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp91;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )5):
    llvm_cbe_code_addr_099_2__PHI_TEMPORARY = llvm_cbe_tmp91;   /* for PHI node */
    goto llvm_cbe_bb71;
  case ((unsigned int )24):
    llvm_cbe_code_addr_099_0_ph__PHI_TEMPORARY = llvm_cbe_tmp91;   /* for PHI node */
    goto llvm_cbe_bb4_preheader;
  case ((unsigned int )82):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code_addr_099_3_ph_ph;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = llvm_cbe_tmp91_rec;   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )88):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp91;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )89):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp91;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )90):
    llvm_cbe_code_addr_099_1_ph153__PHI_TEMPORARY = llvm_cbe_tmp91;   /* for PHI node */
    goto llvm_cbe_bb33_preheader;
  case ((unsigned int )99):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code_addr_099_3_ph_ph;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = llvm_cbe_tmp91_rec;   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )100):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code_addr_099_3_ph_ph;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = llvm_cbe_tmp91_rec;   /* for PHI node */
    goto llvm_cbe_bb83;
  case ((unsigned int )101):
    llvm_cbe_code_addr_099_3_ph_ph__PHI_TEMPORARY = llvm_cbe_code_addr_099_3_ph_ph;   /* for PHI node */
    llvm_cbe_code_addr_099_3_rec__PHI_TEMPORARY = llvm_cbe_tmp91_rec;   /* for PHI node */
    goto llvm_cbe_bb83;
  }
  } while (1); /* end of syntactic loop 'bb83' */
llvm_cbe_return:
  llvm_cbe_code_addr_099_4 = llvm_cbe_code_addr_099_4__PHI_TEMPORARY;
  return llvm_cbe_code_addr_099_4;
}


static unsigned int find_fixedlength(unsigned char *llvm_cbe_code) {
  unsigned char *llvm_cbe_tmp3;
  unsigned char *llvm_cbe_cc_0;
  unsigned char *llvm_cbe_cc_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchlength_0;
  unsigned int llvm_cbe_branchlength_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_length_1;
  unsigned int llvm_cbe_length_1__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp5;
  unsigned int llvm_cbe_tmp19;
  unsigned int llvm_cbe_tmp30;
  unsigned int llvm_cbe_cc_1_rec;
  unsigned int llvm_cbe_cc_1_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp34;
  unsigned char llvm_cbe_tmp39;
  unsigned int llvm_cbe_tmp41;
  unsigned int llvm_cbe_tmp43_rec;
  unsigned char llvm_cbe_tmp45;
  unsigned char *llvm_cbe_tmp51;
  unsigned int llvm_cbe_length_0;
  unsigned int llvm_cbe_length_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp83;
  unsigned int llvm_cbe_cc_2_rec;
  unsigned int llvm_cbe_cc_2_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp91;
  unsigned char llvm_cbe_tmp96;
  unsigned int llvm_cbe_tmp100_rec;
  unsigned char *llvm_cbe_tmp100;
  unsigned char llvm_cbe_tmp102;
  unsigned char *llvm_cbe_cc_3;
  unsigned char *llvm_cbe_cc_3__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp122;
  unsigned char llvm_cbe_tmp125;
  unsigned char *llvm_cbe_tmp128;
  unsigned int llvm_cbe_tmp133;
  unsigned char *llvm_cbe_tmp135;
  unsigned char llvm_cbe_tmp139;
  unsigned char llvm_cbe_tmp144;
  unsigned int llvm_cbe_tmp148;
  unsigned char *llvm_cbe_tmp150;
  unsigned char llvm_cbe_tmp154;
  unsigned char llvm_cbe_tmp159;
  unsigned int llvm_cbe_tmp163;
  unsigned char *llvm_cbe_tmp165;
  unsigned int llvm_cbe_tmp1781;
  unsigned char *llvm_cbe_tmp1802;
  unsigned int llvm_cbe_tmp178;
  unsigned char *llvm_cbe_tmp180;
  unsigned char *llvm_cbe_tmp186;
  unsigned char llvm_cbe_tmp188;
  unsigned char llvm_cbe_tmp199;
  unsigned char llvm_cbe_tmp204;
  unsigned int llvm_cbe_tmp206;
  unsigned char llvm_cbe_tmp209;
  unsigned char llvm_cbe_tmp214;
  unsigned int llvm_cbe_tmp234;
  unsigned char *llvm_cbe_tmp236;
  unsigned int llvm_cbe_tmp239;
  unsigned int llvm_cbe_retval_0;
  unsigned int llvm_cbe_retval_0__PHI_TEMPORARY;

  llvm_cbe_tmp3 = &llvm_cbe_code[((unsigned int )3)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp3;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  goto llvm_cbe_bb;

  do {     /* Syntactic loop 'bb' to make GCC happy */
llvm_cbe_bb:
  llvm_cbe_cc_0 = llvm_cbe_cc_0__PHI_TEMPORARY;
  llvm_cbe_branchlength_0 = llvm_cbe_branchlength_0__PHI_TEMPORARY;
  llvm_cbe_length_1 = llvm_cbe_length_1__PHI_TEMPORARY;
  llvm_cbe_tmp5 = *llvm_cbe_cc_0;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp5))) {
  default:
    goto llvm_cbe_UnifiedReturnBlock;
;
  case ((unsigned int )0):
    goto llvm_cbe_bb56;
  case ((unsigned int )1):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )2):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )4):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )5):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )6):
    goto llvm_cbe_bb176;
  case ((unsigned int )7):
    goto llvm_cbe_bb176;
  case ((unsigned int )8):
    goto llvm_cbe_bb176;
  case ((unsigned int )9):
    goto llvm_cbe_bb176;
  case ((unsigned int )10):
    goto llvm_cbe_bb176;
  case ((unsigned int )11):
    goto llvm_cbe_bb176;
  case ((unsigned int )12):
    goto llvm_cbe_bb176;
  case ((unsigned int )13):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )14):
    goto llvm_cbe_bb167;
  case ((unsigned int )15):
    goto llvm_cbe_bb167;
  case ((unsigned int )22):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )23):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )24):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )25):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )26):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )27):
    goto llvm_cbe_bb131;
  case ((unsigned int )28):
    goto llvm_cbe_bb131;
  case ((unsigned int )29):
    goto llvm_cbe_bb131;
  case ((unsigned int )38):
    goto llvm_cbe_bb136;
  case ((unsigned int )64):
    goto llvm_cbe_bb151;
  case ((unsigned int )77):
    goto llvm_cbe_bb184;
  case ((unsigned int )78):
    goto llvm_cbe_bb184;
  case ((unsigned int )82):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )83):
    goto llvm_cbe_bb56;
  case ((unsigned int )84):
    goto llvm_cbe_bb56;
  case ((unsigned int )85):
    goto llvm_cbe_bb56;
  case ((unsigned int )86):
    goto llvm_cbe_bb56;
  case ((unsigned int )87):
    llvm_cbe_cc_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb88;
  case ((unsigned int )88):
    llvm_cbe_cc_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb88;
  case ((unsigned int )89):
    llvm_cbe_cc_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb88;
  case ((unsigned int )90):
    llvm_cbe_cc_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb88;
  case ((unsigned int )91):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )92):
    goto llvm_cbe_bb11;
    break;
  case ((unsigned int )93):
    goto llvm_cbe_bb11;
    break;
  case ((unsigned int )94):
    goto llvm_cbe_bb11;
    break;
  case ((unsigned int )95):
    goto llvm_cbe_bb11;
    break;
  case ((unsigned int )99):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )100):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  case ((unsigned int )101):
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_cc_0;   /* for PHI node */
    goto llvm_cbe_bb120;
  }
llvm_cbe_bb49:
  llvm_cbe_tmp51 = &llvm_cbe_cc_0[(llvm_cbe_cc_1_rec + (llvm_cbe_tmp41 + ((unsigned int )3)))];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp51;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp30;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

  do {     /* Syntactic loop 'bb31' to make GCC happy */
llvm_cbe_bb31:
  llvm_cbe_cc_1_rec = llvm_cbe_cc_1_rec__PHI_TEMPORARY;
  llvm_cbe_tmp34 = *(&llvm_cbe_cc_0[(llvm_cbe_cc_1_rec + ((unsigned int )1))]);
  llvm_cbe_tmp39 = *(&llvm_cbe_cc_0[(llvm_cbe_cc_1_rec + ((unsigned int )2))]);
  llvm_cbe_tmp41 = ((((unsigned int )(unsigned char )llvm_cbe_tmp34)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp39));
  llvm_cbe_tmp43_rec = llvm_cbe_cc_1_rec + llvm_cbe_tmp41;
  llvm_cbe_tmp45 = *(&llvm_cbe_cc_0[llvm_cbe_tmp43_rec]);
  if ((llvm_cbe_tmp45 == ((unsigned char )83))) {
    llvm_cbe_cc_1_rec__PHI_TEMPORARY = llvm_cbe_tmp43_rec;   /* for PHI node */
    goto llvm_cbe_bb31;
  } else {
    goto llvm_cbe_bb49;
  }

  } while (1); /* end of syntactic loop 'bb31' */
llvm_cbe_cond_next27:
  llvm_cbe_tmp30 = llvm_cbe_tmp19 + llvm_cbe_branchlength_0;
  llvm_cbe_cc_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb31;

llvm_cbe_bb11:
  llvm_cbe_tmp19 =  /*tail*/ find_fixedlength((&llvm_cbe_cc_0[((((llvm_cbe_tmp5 == ((unsigned char )94))) ? (((unsigned int )2)) : (((unsigned int )0))))]));
  if ((((signed int )llvm_cbe_tmp19) < ((signed int )((unsigned int )0)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = llvm_cbe_tmp19;   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next27;
  }

llvm_cbe_cond_next81:
  llvm_cbe_tmp83 = &llvm_cbe_cc_0[((unsigned int )3)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp83;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_0;   /* for PHI node */
  goto llvm_cbe_bb;

llvm_cbe_cond_next72:
  llvm_cbe_length_0 = llvm_cbe_length_0__PHI_TEMPORARY;
  if ((llvm_cbe_tmp5 == ((unsigned char )83))) {
    goto llvm_cbe_cond_next81;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = llvm_cbe_length_0;   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_bb56:
  if ((((signed int )llvm_cbe_length_1) < ((signed int )((unsigned int )0)))) {
    llvm_cbe_length_0__PHI_TEMPORARY = llvm_cbe_branchlength_0;   /* for PHI node */
    goto llvm_cbe_cond_next72;
  } else {
    goto llvm_cbe_cond_false63;
  }

llvm_cbe_cond_false63:
  if ((llvm_cbe_length_1 == llvm_cbe_branchlength_0)) {
    llvm_cbe_length_0__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
    goto llvm_cbe_cond_next72;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_bb120:
  llvm_cbe_cc_3 = llvm_cbe_cc_3__PHI_TEMPORARY;
  llvm_cbe_tmp122 = *llvm_cbe_cc_3;
  llvm_cbe_tmp125 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp122))]);
  llvm_cbe_tmp128 = &llvm_cbe_cc_3[(((unsigned int )(unsigned char )llvm_cbe_tmp125))];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp128;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_branchlength_0;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

  do {     /* Syntactic loop 'bb88' to make GCC happy */
llvm_cbe_bb88:
  llvm_cbe_cc_2_rec = llvm_cbe_cc_2_rec__PHI_TEMPORARY;
  llvm_cbe_tmp91 = *(&llvm_cbe_cc_0[(llvm_cbe_cc_2_rec + ((unsigned int )1))]);
  llvm_cbe_tmp96 = *(&llvm_cbe_cc_0[(llvm_cbe_cc_2_rec + ((unsigned int )2))]);
  llvm_cbe_tmp100_rec = llvm_cbe_cc_2_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp91)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp96)));
  llvm_cbe_tmp100 = &llvm_cbe_cc_0[llvm_cbe_tmp100_rec];
  llvm_cbe_tmp102 = *llvm_cbe_tmp100;
  if ((llvm_cbe_tmp102 == ((unsigned char )83))) {
    llvm_cbe_cc_2_rec__PHI_TEMPORARY = llvm_cbe_tmp100_rec;   /* for PHI node */
    goto llvm_cbe_bb88;
  } else {
    llvm_cbe_cc_3__PHI_TEMPORARY = llvm_cbe_tmp100;   /* for PHI node */
    goto llvm_cbe_bb120;
  }

  } while (1); /* end of syntactic loop 'bb88' */
llvm_cbe_bb131:
  llvm_cbe_tmp133 = llvm_cbe_branchlength_0 + ((unsigned int )1);
  llvm_cbe_tmp135 = &llvm_cbe_cc_0[((unsigned int )2)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp135;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp133;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

llvm_cbe_bb136:
  llvm_cbe_tmp139 = *(&llvm_cbe_cc_0[((unsigned int )1)]);
  llvm_cbe_tmp144 = *(&llvm_cbe_cc_0[((unsigned int )2)]);
  llvm_cbe_tmp148 = (((((unsigned int )(unsigned char )llvm_cbe_tmp139)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp144))) + llvm_cbe_branchlength_0;
  llvm_cbe_tmp150 = &llvm_cbe_cc_0[((unsigned int )4)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp150;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp148;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

llvm_cbe_bb151:
  llvm_cbe_tmp154 = *(&llvm_cbe_cc_0[((unsigned int )1)]);
  llvm_cbe_tmp159 = *(&llvm_cbe_cc_0[((unsigned int )2)]);
  llvm_cbe_tmp163 = (((((unsigned int )(unsigned char )llvm_cbe_tmp154)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp159))) + llvm_cbe_branchlength_0;
  llvm_cbe_tmp165 = &llvm_cbe_cc_0[((unsigned int )4)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp165;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp163;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

llvm_cbe_bb167:
  llvm_cbe_tmp1781 = llvm_cbe_branchlength_0 + ((unsigned int )1);
  llvm_cbe_tmp1802 = &llvm_cbe_cc_0[((unsigned int )3)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp1802;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp1781;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

llvm_cbe_bb176:
  llvm_cbe_tmp178 = llvm_cbe_branchlength_0 + ((unsigned int )1);
  llvm_cbe_tmp180 = &llvm_cbe_cc_0[((unsigned int )1)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp180;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp178;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

llvm_cbe_cond_next222:
  llvm_cbe_tmp234 = llvm_cbe_tmp206 + llvm_cbe_branchlength_0;
  llvm_cbe_tmp236 = &llvm_cbe_cc_0[((unsigned int )38)];
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp236;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp234;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

llvm_cbe_bb196:
  llvm_cbe_tmp199 = *(&llvm_cbe_cc_0[((unsigned int )34)]);
  llvm_cbe_tmp204 = *(&llvm_cbe_cc_0[((unsigned int )35)]);
  llvm_cbe_tmp206 = ((((unsigned int )(unsigned char )llvm_cbe_tmp199)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp204));
  llvm_cbe_tmp209 = *(&llvm_cbe_cc_0[((unsigned int )36)]);
  llvm_cbe_tmp214 = *(&llvm_cbe_cc_0[((unsigned int )37)]);
  if ((llvm_cbe_tmp206 == (((((unsigned int )(unsigned char )llvm_cbe_tmp209)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp214))))) {
    goto llvm_cbe_cond_next222;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_bb184:
  llvm_cbe_tmp186 = &llvm_cbe_cc_0[((unsigned int )33)];
  llvm_cbe_tmp188 = *llvm_cbe_tmp186;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp188))) {
  default:
    goto llvm_cbe_bb237;
;
  case ((unsigned int )69):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )70):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )73):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )74):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )75):
    goto llvm_cbe_bb196;
    break;
  case ((unsigned int )76):
    goto llvm_cbe_bb196;
    break;
  }
llvm_cbe_bb237:
  llvm_cbe_tmp239 = llvm_cbe_branchlength_0 + ((unsigned int )1);
  llvm_cbe_cc_0__PHI_TEMPORARY = llvm_cbe_tmp186;   /* for PHI node */
  llvm_cbe_branchlength_0__PHI_TEMPORARY = llvm_cbe_tmp239;   /* for PHI node */
  llvm_cbe_length_1__PHI_TEMPORARY = llvm_cbe_length_1;   /* for PHI node */
  goto llvm_cbe_bb;

  } while (1); /* end of syntactic loop 'bb' */
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
llvm_cbe_UnifiedReturnBlock:
  return ((unsigned int )-1);
}


static unsigned char *find_bracket(unsigned char *llvm_cbe_code, unsigned int llvm_cbe_number) {
  unsigned char llvm_cbe_tmp2;
  unsigned int llvm_cbe_tmp23;
  unsigned char *llvm_cbe_code_addr_01_0_ph_ph;
  unsigned char *llvm_cbe_code_addr_01_0_ph_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_addr_01_0_rec;
  unsigned int llvm_cbe_code_addr_01_0_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp15;
  unsigned char llvm_cbe_tmp20;
  unsigned int llvm_cbe_tmp25_rec;
  unsigned char *llvm_cbe_tmp25;
  unsigned char llvm_cbe_tmp213;
  unsigned int llvm_cbe_tmp2314_le6;
  unsigned char *llvm_cbe_code_addr_01_1_ph;
  unsigned char *llvm_cbe_code_addr_01_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2310_1_ph;
  unsigned int llvm_cbe_tmp2310_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_addr_01_1_rec;
  unsigned int llvm_cbe_code_addr_01_1_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2310_1;
  unsigned int llvm_cbe_tmp2310_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_addr_01_1;
  unsigned char llvm_cbe_tmp33;
  unsigned char llvm_cbe_tmp38;
  unsigned char llvm_cbe_tmp52;
  unsigned int llvm_cbe_tmp55_rec;
  unsigned char *llvm_cbe_tmp55;
  unsigned char llvm_cbe_tmp216;
  unsigned int llvm_cbe_tmp2317;
  unsigned int llvm_cbe_tmp2314;
  unsigned char *llvm_cbe_code_addr_01_2_ph_ph;
  unsigned char *llvm_cbe_code_addr_01_2_ph_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_addr_01_2_rec;
  unsigned int llvm_cbe_code_addr_01_2_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2310_2;
  unsigned int llvm_cbe_tmp2310_2__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp59;
  unsigned int llvm_cbe_tmp62_rec;
  unsigned char *llvm_cbe_tmp62;
  unsigned char llvm_cbe_tmp219;
  unsigned int llvm_cbe_tmp2320;
  unsigned char *llvm_cbe_retval_0;
  unsigned char *llvm_cbe_retval_0__PHI_TEMPORARY;

  llvm_cbe_tmp2 = *llvm_cbe_code;
  llvm_cbe_tmp23 = ((unsigned int )(unsigned char )llvm_cbe_tmp2);
  switch (llvm_cbe_tmp2) {
  default:
    llvm_cbe_code_addr_01_2_ph_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    llvm_cbe_code_addr_01_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_tmp2310_2__PHI_TEMPORARY = llvm_cbe_tmp23;   /* for PHI node */
    goto llvm_cbe_cond_false56;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )79):
    llvm_cbe_code_addr_01_0_ph_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    llvm_cbe_code_addr_01_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true12;
    break;
  case ((unsigned char )94):
    llvm_cbe_code_addr_01_1_ph__PHI_TEMPORARY = llvm_cbe_code;   /* for PHI node */
    llvm_cbe_tmp2310_1_ph__PHI_TEMPORARY = llvm_cbe_tmp23;   /* for PHI node */
    goto llvm_cbe_cond_true30_preheader;
  }
  do {     /* Syntactic loop 'cond_true12' to make GCC happy */
llvm_cbe_cond_true12:
  llvm_cbe_code_addr_01_0_ph_ph = llvm_cbe_code_addr_01_0_ph_ph__PHI_TEMPORARY;
  llvm_cbe_code_addr_01_0_rec = llvm_cbe_code_addr_01_0_rec__PHI_TEMPORARY;
  llvm_cbe_tmp15 = *(&llvm_cbe_code_addr_01_0_ph_ph[(llvm_cbe_code_addr_01_0_rec + ((unsigned int )1))]);
  llvm_cbe_tmp20 = *(&llvm_cbe_code_addr_01_0_ph_ph[(llvm_cbe_code_addr_01_0_rec + ((unsigned int )2))]);
  llvm_cbe_tmp25_rec = llvm_cbe_code_addr_01_0_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp15)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp20)));
  llvm_cbe_tmp25 = &llvm_cbe_code_addr_01_0_ph_ph[llvm_cbe_tmp25_rec];
  llvm_cbe_tmp213 = *llvm_cbe_tmp25;
  switch (llvm_cbe_tmp213) {
  default:
    goto llvm_cbe_cond_false56_preheader_loopexit;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )79):
    llvm_cbe_code_addr_01_0_ph_ph__PHI_TEMPORARY = llvm_cbe_code_addr_01_0_ph_ph;   /* for PHI node */
    llvm_cbe_code_addr_01_0_rec__PHI_TEMPORARY = llvm_cbe_tmp25_rec;   /* for PHI node */
    goto llvm_cbe_cond_true12;
  case ((unsigned char )94):
    goto llvm_cbe_cond_true30_preheader_loopexit2;
    break;
  }
  } while (1); /* end of syntactic loop 'cond_true12' */
llvm_cbe_cond_true30_preheader_loopexit2:
  llvm_cbe_tmp2314_le6 = ((unsigned int )(unsigned char )llvm_cbe_tmp213);
  llvm_cbe_code_addr_01_1_ph__PHI_TEMPORARY = llvm_cbe_tmp25;   /* for PHI node */
  llvm_cbe_tmp2310_1_ph__PHI_TEMPORARY = llvm_cbe_tmp2314_le6;   /* for PHI node */
  goto llvm_cbe_cond_true30_preheader;

llvm_cbe_cond_true30_preheader:
  llvm_cbe_code_addr_01_1_ph = llvm_cbe_code_addr_01_1_ph__PHI_TEMPORARY;
  llvm_cbe_tmp2310_1_ph = llvm_cbe_tmp2310_1_ph__PHI_TEMPORARY;
  llvm_cbe_code_addr_01_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_tmp2310_1__PHI_TEMPORARY = llvm_cbe_tmp2310_1_ph;   /* for PHI node */
  goto llvm_cbe_cond_true30;

  do {     /* Syntactic loop 'cond_true30' to make GCC happy */
llvm_cbe_cond_true30:
  llvm_cbe_code_addr_01_1_rec = llvm_cbe_code_addr_01_1_rec__PHI_TEMPORARY;
  llvm_cbe_tmp2310_1 = llvm_cbe_tmp2310_1__PHI_TEMPORARY;
  llvm_cbe_code_addr_01_1 = &llvm_cbe_code_addr_01_1_ph[llvm_cbe_code_addr_01_1_rec];
  llvm_cbe_tmp33 = *(&llvm_cbe_code_addr_01_1_ph[(llvm_cbe_code_addr_01_1_rec + ((unsigned int )3))]);
  llvm_cbe_tmp38 = *(&llvm_cbe_code_addr_01_1_ph[(llvm_cbe_code_addr_01_1_rec + ((unsigned int )4))]);
  if (((((((unsigned int )(unsigned char )llvm_cbe_tmp33)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp38))) == llvm_cbe_number)) {
    llvm_cbe_retval_0__PHI_TEMPORARY = llvm_cbe_code_addr_01_1;   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next49;
  }

llvm_cbe_cond_next49:
  llvm_cbe_tmp52 = *(&_pcre_OP_lengths[llvm_cbe_tmp2310_1]);
  llvm_cbe_tmp55_rec = llvm_cbe_code_addr_01_1_rec + (((unsigned int )(unsigned char )llvm_cbe_tmp52));
  llvm_cbe_tmp55 = &llvm_cbe_code_addr_01_1_ph[llvm_cbe_tmp55_rec];
  llvm_cbe_tmp216 = *llvm_cbe_tmp55;
  llvm_cbe_tmp2317 = ((unsigned int )(unsigned char )llvm_cbe_tmp216);
  switch (llvm_cbe_tmp216) {
  default:
    llvm_cbe_code_addr_01_2_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp55;   /* for PHI node */
    llvm_cbe_code_addr_01_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_tmp2310_2__PHI_TEMPORARY = llvm_cbe_tmp2317;   /* for PHI node */
    goto llvm_cbe_cond_false56;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )79):
    llvm_cbe_code_addr_01_0_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp55;   /* for PHI node */
    llvm_cbe_code_addr_01_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true12;
  case ((unsigned char )94):
    llvm_cbe_code_addr_01_1_rec__PHI_TEMPORARY = llvm_cbe_tmp55_rec;   /* for PHI node */
    llvm_cbe_tmp2310_1__PHI_TEMPORARY = llvm_cbe_tmp2317;   /* for PHI node */
    goto llvm_cbe_cond_true30;
  }
  } while (1); /* end of syntactic loop 'cond_true30' */
llvm_cbe_cond_false56_preheader_loopexit:
  llvm_cbe_tmp2314 = ((unsigned int )(unsigned char )llvm_cbe_tmp213);
  llvm_cbe_code_addr_01_2_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp25;   /* for PHI node */
  llvm_cbe_code_addr_01_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_tmp2310_2__PHI_TEMPORARY = llvm_cbe_tmp2314;   /* for PHI node */
  goto llvm_cbe_cond_false56;

  do {     /* Syntactic loop 'cond_false56' to make GCC happy */
llvm_cbe_cond_false56:
  llvm_cbe_code_addr_01_2_ph_ph = llvm_cbe_code_addr_01_2_ph_ph__PHI_TEMPORARY;
  llvm_cbe_code_addr_01_2_rec = llvm_cbe_code_addr_01_2_rec__PHI_TEMPORARY;
  llvm_cbe_tmp2310_2 = llvm_cbe_tmp2310_2__PHI_TEMPORARY;
  llvm_cbe_tmp59 = *(&_pcre_OP_lengths[llvm_cbe_tmp2310_2]);
  llvm_cbe_tmp62_rec = llvm_cbe_code_addr_01_2_rec + (((unsigned int )(unsigned char )llvm_cbe_tmp59));
  llvm_cbe_tmp62 = &llvm_cbe_code_addr_01_2_ph_ph[llvm_cbe_tmp62_rec];
  llvm_cbe_tmp219 = *llvm_cbe_tmp62;
  llvm_cbe_tmp2320 = ((unsigned int )(unsigned char )llvm_cbe_tmp219);
  switch (llvm_cbe_tmp219) {
  default:
    llvm_cbe_code_addr_01_2_ph_ph__PHI_TEMPORARY = llvm_cbe_code_addr_01_2_ph_ph;   /* for PHI node */
    llvm_cbe_code_addr_01_2_rec__PHI_TEMPORARY = llvm_cbe_tmp62_rec;   /* for PHI node */
    llvm_cbe_tmp2310_2__PHI_TEMPORARY = llvm_cbe_tmp2320;   /* for PHI node */
    goto llvm_cbe_cond_false56;
;
  case ((unsigned char )0):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    goto llvm_cbe_return;
    break;
  case ((unsigned char )79):
    llvm_cbe_code_addr_01_0_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp62;   /* for PHI node */
    llvm_cbe_code_addr_01_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true12;
  case ((unsigned char )94):
    llvm_cbe_code_addr_01_1_ph__PHI_TEMPORARY = llvm_cbe_tmp62;   /* for PHI node */
    llvm_cbe_tmp2310_1_ph__PHI_TEMPORARY = llvm_cbe_tmp2320;   /* for PHI node */
    goto llvm_cbe_cond_true30_preheader;
  }
  } while (1); /* end of syntactic loop 'cond_false56' */
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
}


static unsigned int could_be_empty_branch(unsigned char *llvm_cbe_code, unsigned char *llvm_cbe_endcode) {
  unsigned char llvm_cbe_tmp2;
  unsigned char llvm_cbe_tmp5;
  unsigned char *llvm_cbe_tmp9;
  unsigned char *llvm_cbe_code_addr_18_0;
  unsigned char *llvm_cbe_code_addr_18_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp11;
  unsigned int llvm_cbe_tmp1112;
  unsigned char llvm_cbe_tmp19;
  unsigned int llvm_cbe_tmp1920;
  unsigned int llvm_cbe_code_addr_2_rec;
  unsigned int llvm_cbe_code_addr_2_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp22_sum63;
  unsigned char llvm_cbe_tmp26;
  unsigned char llvm_cbe_tmp31;
  unsigned int llvm_cbe_tmp33;
  unsigned int llvm_cbe_tmp35_rec;
  unsigned char llvm_cbe_tmp37;
  unsigned char llvm_cbe_tmp18526;
  unsigned char *llvm_cbe_tmp18929;
  unsigned char llvm_cbe_tmp53;
  unsigned char llvm_cbe_tmp59;
  unsigned int llvm_cbe_empty_branch_4;
  unsigned int llvm_cbe_empty_branch_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_addr_3_rec;
  unsigned int llvm_cbe_code_addr_3_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp77;
  unsigned int llvm_cbe_empty_branch_3;
  unsigned int llvm_cbe_empty_branch_3__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp86;
  unsigned char llvm_cbe_tmp91;
  unsigned int llvm_cbe_tmp95_rec;
  unsigned char *llvm_cbe_tmp95;
  unsigned char llvm_cbe_tmp97;
  unsigned int llvm_cbe_tmp110111;
  unsigned char llvm_cbe_tmp119;
  unsigned char llvm_cbe_tmp133;
  unsigned char llvm_cbe_tmp138;
  unsigned int llvm_cbe_c_2;
  unsigned int llvm_cbe_c_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_addr_0;
  unsigned char *llvm_cbe_code_addr_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp185;
  unsigned char *llvm_cbe_tmp189;
  unsigned char *llvm_cbe_storemerge;
  unsigned char *llvm_cbe_storemerge__PHI_TEMPORARY;
  unsigned int llvm_cbe_retval_0;
  unsigned int llvm_cbe_retval_0__PHI_TEMPORARY;

  llvm_cbe_tmp2 = *llvm_cbe_code;
  llvm_cbe_tmp5 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp2))]);
  llvm_cbe_tmp9 =  /*tail*/ first_significant_code((&llvm_cbe_code[(((unsigned int )(unsigned char )llvm_cbe_tmp5))]), ((unsigned int *)/*NULL*/0), ((unsigned int )0), ((unsigned int )1));
  if ((((unsigned char *)llvm_cbe_tmp9) < ((unsigned char *)llvm_cbe_endcode))) {
    llvm_cbe_code_addr_18_0__PHI_TEMPORARY = llvm_cbe_tmp9;   /* for PHI node */
    goto llvm_cbe_bb;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }

  do {     /* Syntactic loop 'bb' to make GCC happy */
llvm_cbe_bb:
  llvm_cbe_code_addr_18_0 = llvm_cbe_code_addr_18_0__PHI_TEMPORARY;
  llvm_cbe_tmp11 = *llvm_cbe_code_addr_18_0;
  llvm_cbe_tmp1112 = ((unsigned int )(unsigned char )llvm_cbe_tmp11);
  if ((((unsigned int )(llvm_cbe_tmp1112 + ((unsigned int )-102))) < ((unsigned int )((unsigned int )2)))) {
    goto llvm_cbe_cond_true;
  } else {
    goto llvm_cbe_cond_next;
  }

llvm_cbe_bb190:
  llvm_cbe_storemerge = llvm_cbe_storemerge__PHI_TEMPORARY;
  if ((((unsigned char *)llvm_cbe_storemerge) < ((unsigned char *)llvm_cbe_endcode))) {
    llvm_cbe_code_addr_18_0__PHI_TEMPORARY = llvm_cbe_storemerge;   /* for PHI node */
    goto llvm_cbe_bb;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_bb41:
  llvm_cbe_tmp18526 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp37))]);
  llvm_cbe_tmp18929 =  /*tail*/ first_significant_code((&llvm_cbe_code_addr_18_0[(llvm_cbe_tmp22_sum63 + ((((unsigned int )(unsigned char )llvm_cbe_tmp18526)) + llvm_cbe_tmp33))]), ((unsigned int *)/*NULL*/0), ((unsigned int )0), ((unsigned int )1));
  llvm_cbe_storemerge__PHI_TEMPORARY = llvm_cbe_tmp18929;   /* for PHI node */
  goto llvm_cbe_bb190;

  do {     /* Syntactic loop 'bb23' to make GCC happy */
llvm_cbe_bb23:
  llvm_cbe_code_addr_2_rec = llvm_cbe_code_addr_2_rec__PHI_TEMPORARY;
  llvm_cbe_tmp22_sum63 = llvm_cbe_tmp1920 + llvm_cbe_code_addr_2_rec;
  llvm_cbe_tmp26 = *(&llvm_cbe_code_addr_18_0[(llvm_cbe_tmp22_sum63 + ((unsigned int )1))]);
  llvm_cbe_tmp31 = *(&llvm_cbe_code_addr_18_0[(llvm_cbe_tmp22_sum63 + ((unsigned int )2))]);
  llvm_cbe_tmp33 = ((((unsigned int )(unsigned char )llvm_cbe_tmp26)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp31));
  llvm_cbe_tmp35_rec = llvm_cbe_code_addr_2_rec + llvm_cbe_tmp33;
  llvm_cbe_tmp37 = *(&llvm_cbe_code_addr_18_0[(llvm_cbe_tmp1920 + llvm_cbe_tmp35_rec)]);
  if ((llvm_cbe_tmp37 == ((unsigned char )83))) {
    llvm_cbe_code_addr_2_rec__PHI_TEMPORARY = llvm_cbe_tmp35_rec;   /* for PHI node */
    goto llvm_cbe_bb23;
  } else {
    goto llvm_cbe_bb41;
  }

  } while (1); /* end of syntactic loop 'bb23' */
llvm_cbe_cond_true:
  llvm_cbe_tmp19 = *(&_pcre_OP_lengths[llvm_cbe_tmp1112]);
  llvm_cbe_tmp1920 = ((unsigned int )(unsigned char )llvm_cbe_tmp19);
  llvm_cbe_code_addr_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb23;

llvm_cbe_bb182:
  llvm_cbe_c_2 = llvm_cbe_c_2__PHI_TEMPORARY;
  llvm_cbe_code_addr_0 = llvm_cbe_code_addr_0__PHI_TEMPORARY;
  llvm_cbe_tmp185 = *(&_pcre_OP_lengths[llvm_cbe_c_2]);
  llvm_cbe_tmp189 =  /*tail*/ first_significant_code((&llvm_cbe_code_addr_0[(((unsigned int )(unsigned char )llvm_cbe_tmp185))]), ((unsigned int *)/*NULL*/0), ((unsigned int )0), ((unsigned int )1));
  llvm_cbe_storemerge__PHI_TEMPORARY = llvm_cbe_tmp189;   /* for PHI node */
  goto llvm_cbe_bb190;

llvm_cbe_cond_next108:
  llvm_cbe_tmp110111 = ((unsigned int )(unsigned char )llvm_cbe_tmp97);
  llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp110111;   /* for PHI node */
  llvm_cbe_code_addr_0__PHI_TEMPORARY = llvm_cbe_tmp95;   /* for PHI node */
  goto llvm_cbe_bb182;

llvm_cbe_bb101:
  if ((llvm_cbe_empty_branch_3 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next108;
  }

  do {     /* Syntactic loop 'bb68' to make GCC happy */
llvm_cbe_bb68:
  llvm_cbe_empty_branch_4 = llvm_cbe_empty_branch_4__PHI_TEMPORARY;
  llvm_cbe_code_addr_3_rec = llvm_cbe_code_addr_3_rec__PHI_TEMPORARY;
  if ((llvm_cbe_empty_branch_4 == ((unsigned int )0))) {
    goto llvm_cbe_cond_true73;
  } else {
    llvm_cbe_empty_branch_3__PHI_TEMPORARY = llvm_cbe_empty_branch_4;   /* for PHI node */
    goto llvm_cbe_cond_next83;
  }

llvm_cbe_cond_next83:
  llvm_cbe_empty_branch_3 = llvm_cbe_empty_branch_3__PHI_TEMPORARY;
  llvm_cbe_tmp86 = *(&llvm_cbe_code_addr_18_0[(llvm_cbe_code_addr_3_rec + ((unsigned int )1))]);
  llvm_cbe_tmp91 = *(&llvm_cbe_code_addr_18_0[(llvm_cbe_code_addr_3_rec + ((unsigned int )2))]);
  llvm_cbe_tmp95_rec = llvm_cbe_code_addr_3_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp86)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp91)));
  llvm_cbe_tmp95 = &llvm_cbe_code_addr_18_0[llvm_cbe_tmp95_rec];
  llvm_cbe_tmp97 = *llvm_cbe_tmp95;
  if ((llvm_cbe_tmp97 == ((unsigned char )83))) {
    llvm_cbe_empty_branch_4__PHI_TEMPORARY = llvm_cbe_empty_branch_3;   /* for PHI node */
    llvm_cbe_code_addr_3_rec__PHI_TEMPORARY = llvm_cbe_tmp95_rec;   /* for PHI node */
    goto llvm_cbe_bb68;
  } else {
    goto llvm_cbe_bb101;
  }

llvm_cbe_cond_true73:
  llvm_cbe_tmp77 =  /*tail*/ could_be_empty_branch((&llvm_cbe_code_addr_18_0[llvm_cbe_code_addr_3_rec]), llvm_cbe_endcode);
  if ((llvm_cbe_tmp77 == ((unsigned int )0))) {
    llvm_cbe_empty_branch_3__PHI_TEMPORARY = llvm_cbe_empty_branch_4;   /* for PHI node */
    goto llvm_cbe_cond_next83;
  } else {
    goto llvm_cbe_cond_true81;
  }

llvm_cbe_cond_true81:
  llvm_cbe_empty_branch_3__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  goto llvm_cbe_cond_next83;

  } while (1); /* end of syntactic loop 'bb68' */
llvm_cbe_cond_true50:
  llvm_cbe_tmp53 = *(&llvm_cbe_code_addr_18_0[((unsigned int )1)]);
  llvm_cbe_tmp59 = *(&llvm_cbe_code_addr_18_0[((unsigned int )2)]);
  if (((((((unsigned int )(unsigned char )llvm_cbe_tmp53)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp59))) == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    llvm_cbe_empty_branch_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_code_addr_3_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb68;
  }

llvm_cbe_cond_next:
  if ((((unsigned int )(llvm_cbe_tmp1112 + ((unsigned int )-92))) < ((unsigned int )((unsigned int )3)))) {
    goto llvm_cbe_cond_true50;
  } else {
    goto llvm_cbe_cond_next112;
  }

llvm_cbe_cond_next112:
  switch (llvm_cbe_tmp1112) {
  default:
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp1112;   /* for PHI node */
    llvm_cbe_code_addr_0__PHI_TEMPORARY = llvm_cbe_code_addr_18_0;   /* for PHI node */
    goto llvm_cbe_bb182;
;
  case ((unsigned int )6):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )7):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )8):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )9):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )10):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )11):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )12):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )13):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )14):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )15):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )21):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )27):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )28):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )29):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )32):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )33):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )38):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )40):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )45):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )46):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )51):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )53):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )58):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )59):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )64):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )66):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )77):
    goto llvm_cbe_bb115;
    break;
  case ((unsigned int )78):
    goto llvm_cbe_bb115;
    break;
  case ((unsigned int )83):
    goto llvm_cbe_UnifiedReturnBlock;
  case ((unsigned int )84):
    goto llvm_cbe_UnifiedReturnBlock;
  case ((unsigned int )85):
    goto llvm_cbe_UnifiedReturnBlock;
  case ((unsigned int )86):
    goto llvm_cbe_UnifiedReturnBlock;
  }
llvm_cbe_bb115:
  llvm_cbe_tmp119 = *(&llvm_cbe_code_addr_18_0[((unsigned int )33)]);
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp119))) {
  default:
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )69):
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp1112;   /* for PHI node */
    llvm_cbe_code_addr_0__PHI_TEMPORARY = llvm_cbe_code_addr_18_0;   /* for PHI node */
    goto llvm_cbe_bb182;
  case ((unsigned int )70):
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp1112;   /* for PHI node */
    llvm_cbe_code_addr_0__PHI_TEMPORARY = llvm_cbe_code_addr_18_0;   /* for PHI node */
    goto llvm_cbe_bb182;
  case ((unsigned int )73):
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp1112;   /* for PHI node */
    llvm_cbe_code_addr_0__PHI_TEMPORARY = llvm_cbe_code_addr_18_0;   /* for PHI node */
    goto llvm_cbe_bb182;
  case ((unsigned int )74):
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp1112;   /* for PHI node */
    llvm_cbe_code_addr_0__PHI_TEMPORARY = llvm_cbe_code_addr_18_0;   /* for PHI node */
    goto llvm_cbe_bb182;
  case ((unsigned int )75):
    goto llvm_cbe_bb130;
    break;
  case ((unsigned int )76):
    goto llvm_cbe_bb130;
    break;
  }
llvm_cbe_bb130:
  llvm_cbe_tmp133 = *(&llvm_cbe_code_addr_18_0[((unsigned int )34)]);
  llvm_cbe_tmp138 = *(&llvm_cbe_code_addr_18_0[((unsigned int )35)]);
  if ((((signed int )(((((unsigned int )(unsigned char )llvm_cbe_tmp133)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp138)))) > ((signed int )((unsigned int )0)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp1112;   /* for PHI node */
    llvm_cbe_code_addr_0__PHI_TEMPORARY = llvm_cbe_code_addr_18_0;   /* for PHI node */
    goto llvm_cbe_bb182;
  }

  } while (1); /* end of syntactic loop 'bb' */
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
llvm_cbe_UnifiedReturnBlock:
  return ((unsigned int )1);
}


static unsigned int check_posix_syntax(unsigned char *llvm_cbe_ptr, unsigned char **llvm_cbe_endptr, struct l_struct_2E_compile_data *llvm_cbe_cd) {
  unsigned char llvm_cbe_tmp4;
  unsigned char *llvm_cbe_tmp7;
  unsigned char llvm_cbe_tmp9;
  unsigned char *llvm_cbe_tmp13;
  unsigned char *llvm_cbe_ptr_addr_0_ph;
  unsigned char *llvm_cbe_ptr_addr_0_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1957;
  unsigned char llvm_cbe_tmp2158;
  unsigned char llvm_cbe_tmp2461;
  unsigned int llvm_cbe_ptr_addr_055_rec;
  unsigned int llvm_cbe_ptr_addr_055_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp15_rec;
  unsigned char *llvm_cbe_tmp15;
  unsigned char llvm_cbe_tmp21;
  unsigned char llvm_cbe_tmp24;
  unsigned char *llvm_cbe_ptr_addr_0_lcssa;
  unsigned char *llvm_cbe_ptr_addr_0_lcssa__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp32;
  unsigned char llvm_cbe_tmp41;

  llvm_cbe_tmp4 = *(&llvm_cbe_ptr[((unsigned int )1)]);
  llvm_cbe_tmp7 = &llvm_cbe_ptr[((unsigned int )2)];
  llvm_cbe_tmp9 = *llvm_cbe_tmp7;
  if ((llvm_cbe_tmp9 == ((unsigned char )94))) {
    goto llvm_cbe_cond_true;
  } else {
    llvm_cbe_ptr_addr_0_ph__PHI_TEMPORARY = llvm_cbe_tmp7;   /* for PHI node */
    goto llvm_cbe_bb16_preheader;
  }

llvm_cbe_cond_true:
  llvm_cbe_tmp13 = &llvm_cbe_ptr[((unsigned int )3)];
  llvm_cbe_ptr_addr_0_ph__PHI_TEMPORARY = llvm_cbe_tmp13;   /* for PHI node */
  goto llvm_cbe_bb16_preheader;

llvm_cbe_bb16_preheader:
  llvm_cbe_ptr_addr_0_ph = llvm_cbe_ptr_addr_0_ph__PHI_TEMPORARY;
  llvm_cbe_tmp1957 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp2158 = *llvm_cbe_ptr_addr_0_ph;
  llvm_cbe_tmp2461 = *(&llvm_cbe_tmp1957[(((unsigned int )(unsigned char )llvm_cbe_tmp2158))]);
  if (((((unsigned char )(llvm_cbe_tmp2461 & ((unsigned char )2)))) == ((unsigned char )0))) {
    llvm_cbe_ptr_addr_0_lcssa__PHI_TEMPORARY = llvm_cbe_ptr_addr_0_ph;   /* for PHI node */
    goto llvm_cbe_bb30;
  } else {
    llvm_cbe_ptr_addr_055_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb;
  }

  do {     /* Syntactic loop 'bb' to make GCC happy */
llvm_cbe_bb:
  llvm_cbe_ptr_addr_055_rec = llvm_cbe_ptr_addr_055_rec__PHI_TEMPORARY;
  llvm_cbe_tmp15_rec = llvm_cbe_ptr_addr_055_rec + ((unsigned int )1);
  llvm_cbe_tmp15 = &llvm_cbe_ptr_addr_0_ph[llvm_cbe_tmp15_rec];
  llvm_cbe_tmp21 = *llvm_cbe_tmp15;
  llvm_cbe_tmp24 = *(&llvm_cbe_tmp1957[(((unsigned int )(unsigned char )llvm_cbe_tmp21))]);
  if (((((unsigned char )(llvm_cbe_tmp24 & ((unsigned char )2)))) == ((unsigned char )0))) {
    llvm_cbe_ptr_addr_0_lcssa__PHI_TEMPORARY = llvm_cbe_tmp15;   /* for PHI node */
    goto llvm_cbe_bb30;
  } else {
    llvm_cbe_ptr_addr_055_rec__PHI_TEMPORARY = llvm_cbe_tmp15_rec;   /* for PHI node */
    goto llvm_cbe_bb;
  }

  } while (1); /* end of syntactic loop 'bb' */
llvm_cbe_bb30:
  llvm_cbe_ptr_addr_0_lcssa = llvm_cbe_ptr_addr_0_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp32 = *llvm_cbe_ptr_addr_0_lcssa;
  if ((llvm_cbe_tmp32 == llvm_cbe_tmp4)) {
    goto llvm_cbe_cond_true38;
  } else {
    goto llvm_cbe_UnifiedReturnBlock;
  }

llvm_cbe_cond_true38:
  llvm_cbe_tmp41 = *(&llvm_cbe_ptr_addr_0_lcssa[((unsigned int )1)]);
  if ((llvm_cbe_tmp41 == ((unsigned char )93))) {
    goto llvm_cbe_cond_true45;
  } else {
    goto llvm_cbe_UnifiedReturnBlock;
  }

llvm_cbe_cond_true45:
  *llvm_cbe_endptr = llvm_cbe_ptr_addr_0_lcssa;
  return ((unsigned int )1);
llvm_cbe_UnifiedReturnBlock:
  return ((unsigned int )0);
}


static void adjust_recurse(unsigned char *llvm_cbe_group, unsigned int llvm_cbe_adjust, struct l_struct_2E_compile_data *llvm_cbe_cd, unsigned char *llvm_cbe_save_hwm) {
  unsigned char llvm_cbe_tmp10410;
  unsigned char llvm_cbe_tmp6;
  unsigned char *llvm_cbe_tmp10;
  unsigned char llvm_cbe_tmp11;
  unsigned int llvm_cbe_tmp13;
  unsigned char *llvm_cbe_tmp16;
  unsigned char *llvm_cbe_tmp5113;
  unsigned int llvm_cbe_indvar_next;
  unsigned int llvm_cbe_indvar;
  unsigned int llvm_cbe_indvar__PHI_TEMPORARY;
  unsigned int llvm_cbe_hc_1_rec;
  unsigned char *llvm_cbe_hc_1;
  unsigned char llvm_cbe_tmp59;
  unsigned char *llvm_cbe_tmp63;
  unsigned char llvm_cbe_tmp64;
  unsigned int llvm_cbe_tmp66;
  unsigned char *llvm_cbe_tmp70;
  unsigned char *llvm_cbe_tmp96;
  unsigned char llvm_cbe_tmp104;
  unsigned int llvm_cbe_indvar_next49;
  unsigned char *llvm_cbe_code_11_0_ph;
  unsigned char *llvm_cbe_code_11_0_ph__PHI_TEMPORARY;
  unsigned char **llvm_cbe_tmp42;
  unsigned char **llvm_cbe_tmp15;
  unsigned char llvm_cbe_tmp3233;
  unsigned int llvm_cbe_indvar48;
  unsigned int llvm_cbe_indvar48__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_11_0_rec;
  unsigned char *llvm_cbe_tmp43;
  unsigned char *llvm_cbe_tmp20;
  unsigned char *llvm_cbe_code_11_1_ph_ph;
  unsigned char *llvm_cbe_code_11_1_ph_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_11_1_rec;
  unsigned int llvm_cbe_code_11_1_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp128;
  unsigned char llvm_cbe_tmp133;
  unsigned int llvm_cbe_tmp137_rec;
  unsigned char *llvm_cbe_tmp137;
  unsigned char llvm_cbe_tmp10425;
  unsigned char *llvm_cbe_code_11_2_ph_ph;
  unsigned char *llvm_cbe_code_11_2_ph_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_11_2_rec;
  unsigned int llvm_cbe_code_11_2_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp1047_2;
  unsigned char llvm_cbe_tmp1047_2__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp140;
  unsigned int llvm_cbe_tmp143_rec;
  unsigned char *llvm_cbe_tmp143;
  unsigned char llvm_cbe_tmp10428;

  llvm_cbe_tmp10410 = *llvm_cbe_group;
  switch (llvm_cbe_tmp10410) {
  default:
    llvm_cbe_code_11_2_ph_ph__PHI_TEMPORARY = llvm_cbe_group;   /* for PHI node */
    llvm_cbe_code_11_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_tmp1047_2__PHI_TEMPORARY = llvm_cbe_tmp10410;   /* for PHI node */
    goto llvm_cbe_cond_false;
;
  case ((unsigned char )0):
    goto llvm_cbe_UnifiedReturnBlock;
  case ((unsigned char )81):
    llvm_cbe_code_11_0_ph__PHI_TEMPORARY = llvm_cbe_group;   /* for PHI node */
    goto llvm_cbe_cond_true117_preheader;
  case ((unsigned char )79):
    llvm_cbe_code_11_1_ph_ph__PHI_TEMPORARY = llvm_cbe_group;   /* for PHI node */
    llvm_cbe_code_11_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true125;
  }
llvm_cbe_cond_true117_preheader:
  llvm_cbe_code_11_0_ph = llvm_cbe_code_11_0_ph__PHI_TEMPORARY;
  llvm_cbe_tmp42 = &llvm_cbe_cd->field8;
  llvm_cbe_tmp15 = &llvm_cbe_cd->field5;
  llvm_cbe_tmp3233 = ((unsigned char )llvm_cbe_adjust);
  llvm_cbe_indvar48__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_cond_true117;

  do {     /* Syntactic loop 'cond_true117' to make GCC happy */
llvm_cbe_cond_true117:
  llvm_cbe_indvar48 = llvm_cbe_indvar48__PHI_TEMPORARY;
  llvm_cbe_code_11_0_rec = llvm_cbe_indvar48 * ((unsigned int )3);
  if (((&llvm_cbe_code_11_0_ph[llvm_cbe_code_11_0_rec]) == ((unsigned char *)/*NULL*/0))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_bb40_preheader;
  }

llvm_cbe_cond_next94:
  llvm_cbe_tmp96 = &llvm_cbe_code_11_0_ph[(llvm_cbe_code_11_0_rec + ((unsigned int )3))];
  llvm_cbe_tmp104 = *llvm_cbe_tmp96;
  llvm_cbe_indvar_next49 = llvm_cbe_indvar48 + ((unsigned int )1);
  switch (llvm_cbe_tmp104) {
  default:
    llvm_cbe_code_11_2_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp96;   /* for PHI node */
    llvm_cbe_code_11_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_tmp1047_2__PHI_TEMPORARY = llvm_cbe_tmp104;   /* for PHI node */
    goto llvm_cbe_cond_false;
;
  case ((unsigned char )0):
    goto llvm_cbe_UnifiedReturnBlock;
  case ((unsigned char )81):
    llvm_cbe_indvar48__PHI_TEMPORARY = llvm_cbe_indvar_next49;   /* for PHI node */
    goto llvm_cbe_cond_true117;
  case ((unsigned char )79):
    llvm_cbe_code_11_1_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp96;   /* for PHI node */
    llvm_cbe_code_11_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true125;
  }
llvm_cbe_cond_true:
  *llvm_cbe_hc_1 = (((unsigned char )(((unsigned int )(((unsigned int )(llvm_cbe_tmp13 + llvm_cbe_adjust)) >> ((unsigned int )((unsigned int )8)))))));
  *llvm_cbe_tmp10 = (((unsigned char )((((unsigned char )llvm_cbe_tmp13)) + llvm_cbe_tmp3233)));
  llvm_cbe_tmp5113 = *llvm_cbe_tmp42;
  if ((((unsigned char *)llvm_cbe_tmp5113) > ((unsigned char *)llvm_cbe_hc_1))) {
    goto llvm_cbe_cond_next94;
  } else {
    goto llvm_cbe_cond_true56;
  }

  do {     /* Syntactic loop 'bb40' to make GCC happy */
llvm_cbe_bb40:
  llvm_cbe_indvar = llvm_cbe_indvar__PHI_TEMPORARY;
  llvm_cbe_hc_1_rec = llvm_cbe_indvar << ((unsigned int )1);
  llvm_cbe_hc_1 = &llvm_cbe_save_hwm[llvm_cbe_hc_1_rec];
  if ((((unsigned char *)llvm_cbe_tmp43) > ((unsigned char *)llvm_cbe_hc_1))) {
    goto llvm_cbe_bb3;
  } else {
    goto llvm_cbe_cond_true56;
  }

llvm_cbe_cond_next:
  llvm_cbe_indvar_next = llvm_cbe_indvar + ((unsigned int )1);
  llvm_cbe_indvar__PHI_TEMPORARY = llvm_cbe_indvar_next;   /* for PHI node */
  goto llvm_cbe_bb40;

llvm_cbe_bb3:
  llvm_cbe_tmp6 = *llvm_cbe_hc_1;
  llvm_cbe_tmp10 = &llvm_cbe_save_hwm[(llvm_cbe_hc_1_rec | ((unsigned int )1))];
  llvm_cbe_tmp11 = *llvm_cbe_tmp10;
  llvm_cbe_tmp13 = ((((unsigned int )(unsigned char )llvm_cbe_tmp6)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp11));
  llvm_cbe_tmp16 = *llvm_cbe_tmp15;
  if (((&llvm_cbe_tmp16[llvm_cbe_tmp13]) == llvm_cbe_tmp20)) {
    goto llvm_cbe_cond_true;
  } else {
    goto llvm_cbe_cond_next;
  }

  } while (1); /* end of syntactic loop 'bb40' */
llvm_cbe_bb40_preheader:
  llvm_cbe_tmp43 = *llvm_cbe_tmp42;
  llvm_cbe_tmp20 = &llvm_cbe_code_11_0_ph[(llvm_cbe_code_11_0_rec + ((unsigned int )1))];
  llvm_cbe_indvar__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb40;

llvm_cbe_cond_true56:
  llvm_cbe_tmp59 = *llvm_cbe_tmp20;
  llvm_cbe_tmp63 = &llvm_cbe_code_11_0_ph[(llvm_cbe_code_11_0_rec + ((unsigned int )2))];
  llvm_cbe_tmp64 = *llvm_cbe_tmp63;
  llvm_cbe_tmp66 = ((((unsigned int )(unsigned char )llvm_cbe_tmp59)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp64));
  llvm_cbe_tmp70 = *llvm_cbe_tmp15;
  if ((((unsigned char *)(&llvm_cbe_tmp70[llvm_cbe_tmp66])) < ((unsigned char *)llvm_cbe_group))) {
    goto llvm_cbe_cond_next94;
  } else {
    goto llvm_cbe_cond_true77;
  }

llvm_cbe_cond_true77:
  *llvm_cbe_tmp20 = (((unsigned char )(((unsigned int )(((unsigned int )(llvm_cbe_tmp66 + llvm_cbe_adjust)) >> ((unsigned int )((unsigned int )8)))))));
  *llvm_cbe_tmp63 = (((unsigned char )((((unsigned char )llvm_cbe_tmp66)) + llvm_cbe_tmp3233)));
  goto llvm_cbe_cond_next94;

  } while (1); /* end of syntactic loop 'cond_true117' */
  do {     /* Syntactic loop 'cond_true125' to make GCC happy */
llvm_cbe_cond_true125:
  llvm_cbe_code_11_1_ph_ph = llvm_cbe_code_11_1_ph_ph__PHI_TEMPORARY;
  llvm_cbe_code_11_1_rec = llvm_cbe_code_11_1_rec__PHI_TEMPORARY;
  llvm_cbe_tmp128 = *(&llvm_cbe_code_11_1_ph_ph[(llvm_cbe_code_11_1_rec + ((unsigned int )1))]);
  llvm_cbe_tmp133 = *(&llvm_cbe_code_11_1_ph_ph[(llvm_cbe_code_11_1_rec + ((unsigned int )2))]);
  llvm_cbe_tmp137_rec = llvm_cbe_code_11_1_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp128)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp133)));
  llvm_cbe_tmp137 = &llvm_cbe_code_11_1_ph_ph[llvm_cbe_tmp137_rec];
  llvm_cbe_tmp10425 = *llvm_cbe_tmp137;
  switch (llvm_cbe_tmp10425) {
  default:
    llvm_cbe_code_11_2_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp137;   /* for PHI node */
    llvm_cbe_code_11_2_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_tmp1047_2__PHI_TEMPORARY = llvm_cbe_tmp10425;   /* for PHI node */
    goto llvm_cbe_cond_false;
;
  case ((unsigned char )0):
    goto llvm_cbe_UnifiedReturnBlock;
  case ((unsigned char )81):
    llvm_cbe_code_11_0_ph__PHI_TEMPORARY = llvm_cbe_tmp137;   /* for PHI node */
    goto llvm_cbe_cond_true117_preheader;
  case ((unsigned char )79):
    llvm_cbe_code_11_1_ph_ph__PHI_TEMPORARY = llvm_cbe_code_11_1_ph_ph;   /* for PHI node */
    llvm_cbe_code_11_1_rec__PHI_TEMPORARY = llvm_cbe_tmp137_rec;   /* for PHI node */
    goto llvm_cbe_cond_true125;
  }
  } while (1); /* end of syntactic loop 'cond_true125' */
  do {     /* Syntactic loop 'cond_false' to make GCC happy */
llvm_cbe_cond_false:
  llvm_cbe_code_11_2_ph_ph = llvm_cbe_code_11_2_ph_ph__PHI_TEMPORARY;
  llvm_cbe_code_11_2_rec = llvm_cbe_code_11_2_rec__PHI_TEMPORARY;
  llvm_cbe_tmp1047_2 = llvm_cbe_tmp1047_2__PHI_TEMPORARY;
  llvm_cbe_tmp140 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp1047_2))]);
  llvm_cbe_tmp143_rec = llvm_cbe_code_11_2_rec + (((unsigned int )(unsigned char )llvm_cbe_tmp140));
  llvm_cbe_tmp143 = &llvm_cbe_code_11_2_ph_ph[llvm_cbe_tmp143_rec];
  llvm_cbe_tmp10428 = *llvm_cbe_tmp143;
  switch (llvm_cbe_tmp10428) {
  default:
    llvm_cbe_code_11_2_ph_ph__PHI_TEMPORARY = llvm_cbe_code_11_2_ph_ph;   /* for PHI node */
    llvm_cbe_code_11_2_rec__PHI_TEMPORARY = llvm_cbe_tmp143_rec;   /* for PHI node */
    llvm_cbe_tmp1047_2__PHI_TEMPORARY = llvm_cbe_tmp10428;   /* for PHI node */
    goto llvm_cbe_cond_false;
;
  case ((unsigned char )0):
    goto llvm_cbe_UnifiedReturnBlock;
    break;
  case ((unsigned char )81):
    llvm_cbe_code_11_0_ph__PHI_TEMPORARY = llvm_cbe_tmp143;   /* for PHI node */
    goto llvm_cbe_cond_true117_preheader;
  case ((unsigned char )79):
    llvm_cbe_code_11_1_ph_ph__PHI_TEMPORARY = llvm_cbe_tmp143;   /* for PHI node */
    llvm_cbe_code_11_1_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true125;
  }
  } while (1); /* end of syntactic loop 'cond_false' */
llvm_cbe_UnifiedReturnBlock:
  return;
}


static unsigned int check_auto_possessive(unsigned int llvm_cbe_op_code, unsigned int llvm_cbe_item, unsigned char *llvm_cbe_ptr, unsigned int llvm_cbe_options, struct l_struct_2E_compile_data *llvm_cbe_cd) {
  unsigned char *llvm_cbe_ptr_addr;    /* Address-exposed local */
  unsigned int llvm_cbe_temperrorcode;    /* Address-exposed local */
  unsigned char **llvm_cbe_tmp10161;
  unsigned int *llvm_cbe_tmp30;
  unsigned char **llvm_cbe_tmp37;
  unsigned int *llvm_cbe_tmp45;
  unsigned char *llvm_cbe_tmp83;
  unsigned char *llvm_cbe_tmp103;
  unsigned char *llvm_cbe_tmp6171;
  unsigned char *llvm_cbe_tmp6171__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp7;
  unsigned char llvm_cbe_tmp13;
  unsigned char llvm_cbe_tmp16;
  unsigned char *llvm_cbe_tmp11162;
  unsigned char *llvm_cbe_tmp12163;
  unsigned char llvm_cbe_tmp13164;
  unsigned char llvm_cbe_tmp16167;
  unsigned char *llvm_cbe_tmp6_lcssa;
  unsigned char *llvm_cbe_tmp6_lcssa__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp23;
  unsigned int llvm_cbe_tmp31;
  unsigned char *llvm_cbe_tmp38;
  unsigned int llvm_cbe_tmp54;
  unsigned int llvm_cbe_tmp69;
  unsigned char llvm_cbe_tmp84;
  unsigned char llvm_cbe_tmp100;
  unsigned char llvm_cbe_tmp104;
  unsigned char *llvm_cbe_tmp117;
  unsigned int llvm_cbe_tmp120;
  unsigned char *llvm_cbe_tmp124;
  unsigned char *llvm_cbe_tmp125;
  unsigned char llvm_cbe_tmp127;
  unsigned char *llvm_cbe_tmp136;
  unsigned char llvm_cbe_tmp137;
  unsigned int llvm_cbe_tmp144;
  unsigned int llvm_cbe_tmp146;
  unsigned int llvm_cbe_tmp147;
  unsigned char *llvm_cbe_tmp154;
  unsigned char *llvm_cbe_tmp159;
  unsigned int llvm_cbe_tmp161162;
  unsigned char llvm_cbe_tmp164;
  unsigned int llvm_cbe_next_018_0_ph;
  unsigned int llvm_cbe_next_018_0_ph__PHI_TEMPORARY;
  unsigned char **llvm_cbe_tmp189172;
  unsigned int *llvm_cbe_tmp209;
  unsigned char **llvm_cbe_tmp216;
  unsigned int *llvm_cbe_tmp225;
  unsigned char *llvm_cbe_tmp264;
  unsigned char *llvm_cbe_tmp284;
  unsigned char *llvm_cbe_tmp185182;
  unsigned char *llvm_cbe_tmp185182__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp186;
  unsigned char llvm_cbe_tmp192;
  unsigned char llvm_cbe_tmp195;
  unsigned char *llvm_cbe_tmp190173;
  unsigned char *llvm_cbe_tmp191174;
  unsigned char llvm_cbe_tmp192175;
  unsigned char llvm_cbe_tmp195178;
  unsigned char *llvm_cbe_tmp185_lcssa;
  unsigned char *llvm_cbe_tmp185_lcssa__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp202;
  unsigned int llvm_cbe_tmp210;
  unsigned char *llvm_cbe_tmp217;
  unsigned int llvm_cbe_tmp234;
  unsigned int llvm_cbe_tmp250;
  unsigned char llvm_cbe_tmp265;
  unsigned char llvm_cbe_tmp281;
  unsigned char llvm_cbe_tmp285;
  unsigned char *llvm_cbe_tmp298;
  unsigned int llvm_cbe_tmp301;
  unsigned char *llvm_cbe_tmp305;
  unsigned char *llvm_cbe_tmp306;
  unsigned char llvm_cbe_tmp308;
  unsigned int llvm_cbe_next_018_1;
  unsigned int llvm_cbe_next_018_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp317;
  unsigned char llvm_cbe_tmp318;
  unsigned int llvm_cbe_tmp334;
  unsigned char *llvm_cbe_tmp364;
  unsigned char llvm_cbe_tmp367;
  unsigned char *llvm_cbe_tmp399;
  unsigned char llvm_cbe_tmp402;
  unsigned char *llvm_cbe_tmp417;
  unsigned char llvm_cbe_tmp420;
  unsigned char *llvm_cbe_tmp441;
  unsigned char llvm_cbe_tmp444;
  unsigned char *llvm_cbe_tmp467;
  unsigned char llvm_cbe_tmp470;
  unsigned char *llvm_cbe_tmp490;
  unsigned char llvm_cbe_tmp493;
  unsigned char *llvm_cbe_tmp515;
  unsigned char llvm_cbe_tmp518;
  unsigned char *llvm_cbe_tmp539;
  unsigned char llvm_cbe_tmp542;
  unsigned char *llvm_cbe_tmp623;
  unsigned char llvm_cbe_tmp626;
  unsigned char *llvm_cbe_tmp647;
  unsigned char llvm_cbe_tmp650;
  unsigned char *llvm_cbe_tmp673;
  unsigned char llvm_cbe_tmp676;
  unsigned char *llvm_cbe_tmp696;
  unsigned char llvm_cbe_tmp699;
  unsigned char *llvm_cbe_tmp721;
  unsigned char llvm_cbe_tmp724;
  unsigned char *llvm_cbe_tmp745;
  unsigned char llvm_cbe_tmp748;
  unsigned int llvm_cbe_retval_0;
  unsigned int llvm_cbe_retval_0__PHI_TEMPORARY;

  *(&llvm_cbe_ptr_addr) = llvm_cbe_ptr;
  if (((llvm_cbe_options & ((unsigned int )8)) == ((unsigned int )0))) {
    goto llvm_cbe_cond_next135;
  } else {
    goto llvm_cbe_bb8_preheader;
  }

llvm_cbe_bb8_preheader:
  llvm_cbe_tmp10161 = &llvm_cbe_cd->field3;
  llvm_cbe_tmp30 = &llvm_cbe_cd->field18;
  llvm_cbe_tmp37 = &llvm_cbe_cd->field7;
  llvm_cbe_tmp45 = &llvm_cbe_cd->field19;
  llvm_cbe_tmp83 = &llvm_cbe_cd->field20[((unsigned int )0)];
  llvm_cbe_tmp103 = &llvm_cbe_cd->field20[((unsigned int )1)];
  goto llvm_cbe_bb8_outer;

  do {     /* Syntactic loop 'bb8.outer' to make GCC happy */
llvm_cbe_bb8_outer:
  llvm_cbe_tmp11162 = *llvm_cbe_tmp10161;
  llvm_cbe_tmp12163 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp13164 = *llvm_cbe_tmp12163;
  llvm_cbe_tmp16167 = *(&llvm_cbe_tmp11162[(((unsigned int )(unsigned char )llvm_cbe_tmp13164))]);
  if (((((unsigned char )(llvm_cbe_tmp16167 & ((unsigned char )1)))) == ((unsigned char )0))) {
    llvm_cbe_tmp6_lcssa__PHI_TEMPORARY = llvm_cbe_tmp12163;   /* for PHI node */
    goto llvm_cbe_bb21;
  } else {
    llvm_cbe_tmp6171__PHI_TEMPORARY = llvm_cbe_tmp12163;   /* for PHI node */
    goto llvm_cbe_bb5;
  }

llvm_cbe_cond_true116:
  llvm_cbe_tmp117 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp120 = *llvm_cbe_tmp45;
  *(&llvm_cbe_ptr_addr) = (&llvm_cbe_tmp117[llvm_cbe_tmp120]);
  goto llvm_cbe_bb8_outer;

  do {     /* Syntactic loop 'bb123' to make GCC happy */
llvm_cbe_bb123:
  llvm_cbe_tmp124 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp125 = &llvm_cbe_tmp124[((unsigned int )1)];
  *(&llvm_cbe_ptr_addr) = llvm_cbe_tmp125;
  llvm_cbe_tmp127 = *llvm_cbe_tmp125;
  if ((llvm_cbe_tmp127 == ((unsigned char )0))) {
    goto llvm_cbe_bb8_outer;
  } else {
    goto llvm_cbe_bb28;
  }

llvm_cbe_cond_true35:
  if ((((unsigned char *)llvm_cbe_tmp38) > ((unsigned char *)llvm_cbe_tmp125))) {
    goto llvm_cbe_cond_next;
  } else {
    goto llvm_cbe_bb123;
  }

llvm_cbe_bb28:
  llvm_cbe_tmp31 = *llvm_cbe_tmp30;
  llvm_cbe_tmp38 = *llvm_cbe_tmp37;
  if ((llvm_cbe_tmp31 == ((unsigned int )0))) {
    goto llvm_cbe_cond_false;
  } else {
    goto llvm_cbe_cond_true35;
  }

llvm_cbe_cond_next:
  llvm_cbe_tmp54 = _pcre_is_newline(llvm_cbe_tmp125, llvm_cbe_tmp31, llvm_cbe_tmp38, llvm_cbe_tmp45);
  if ((llvm_cbe_tmp54 == ((unsigned int )0))) {
    goto llvm_cbe_bb123;
  } else {
    goto llvm_cbe_cond_true116;
  }

llvm_cbe_cond_false:
  llvm_cbe_tmp69 = *llvm_cbe_tmp45;
  if ((((unsigned char *)(&llvm_cbe_tmp38[(-(llvm_cbe_tmp69))])) < ((unsigned char *)llvm_cbe_tmp125))) {
    goto llvm_cbe_bb123;
  } else {
    goto llvm_cbe_cond_next77;
  }

llvm_cbe_cond_next77:
  llvm_cbe_tmp84 = *llvm_cbe_tmp83;
  if ((llvm_cbe_tmp127 == llvm_cbe_tmp84)) {
    goto llvm_cbe_cond_next89;
  } else {
    goto llvm_cbe_bb123;
  }

llvm_cbe_cond_next97:
  llvm_cbe_tmp100 = *(&llvm_cbe_tmp124[((unsigned int )2)]);
  llvm_cbe_tmp104 = *llvm_cbe_tmp103;
  if ((llvm_cbe_tmp100 == llvm_cbe_tmp104)) {
    goto llvm_cbe_cond_true116;
  } else {
    goto llvm_cbe_bb123;
  }

llvm_cbe_cond_next89:
  if ((llvm_cbe_tmp69 == ((unsigned int )1))) {
    goto llvm_cbe_cond_true116;
  } else {
    goto llvm_cbe_cond_next97;
  }

  } while (1); /* end of syntactic loop 'bb123' */
llvm_cbe_bb21:
  llvm_cbe_tmp6_lcssa = llvm_cbe_tmp6_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp23 = *llvm_cbe_tmp6_lcssa;
  if ((llvm_cbe_tmp23 == ((unsigned char )35))) {
    goto llvm_cbe_bb123;
  } else {
    goto llvm_cbe_cond_next135;
  }

  do {     /* Syntactic loop 'bb5' to make GCC happy */
llvm_cbe_bb5:
  llvm_cbe_tmp6171 = llvm_cbe_tmp6171__PHI_TEMPORARY;
  llvm_cbe_tmp7 = &llvm_cbe_tmp6171[((unsigned int )1)];
  *(&llvm_cbe_ptr_addr) = llvm_cbe_tmp7;
  llvm_cbe_tmp13 = *llvm_cbe_tmp7;
  llvm_cbe_tmp16 = *(&llvm_cbe_tmp11162[(((unsigned int )(unsigned char )llvm_cbe_tmp13))]);
  if (((((unsigned char )(llvm_cbe_tmp16 & ((unsigned char )1)))) == ((unsigned char )0))) {
    llvm_cbe_tmp6_lcssa__PHI_TEMPORARY = llvm_cbe_tmp7;   /* for PHI node */
    goto llvm_cbe_bb21;
  } else {
    llvm_cbe_tmp6171__PHI_TEMPORARY = llvm_cbe_tmp7;   /* for PHI node */
    goto llvm_cbe_bb5;
  }

  } while (1); /* end of syntactic loop 'bb5' */
  } while (1); /* end of syntactic loop 'bb8.outer' */
llvm_cbe_cond_next135:
  llvm_cbe_tmp136 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp137 = *llvm_cbe_tmp136;
  if ((llvm_cbe_tmp137 == ((unsigned char )92))) {
    goto llvm_cbe_cond_true141;
  } else {
    goto llvm_cbe_cond_false156;
  }

llvm_cbe_cond_true141:
  *(&llvm_cbe_temperrorcode) = ((unsigned int )0);
  llvm_cbe_tmp144 = *(&llvm_cbe_cd->field12);
  llvm_cbe_tmp146 = check_escape((&llvm_cbe_ptr_addr), (&llvm_cbe_temperrorcode), llvm_cbe_tmp144, llvm_cbe_options, ((unsigned int )0));
  llvm_cbe_tmp147 = *(&llvm_cbe_temperrorcode);
  if ((llvm_cbe_tmp147 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next153;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_next153:
  llvm_cbe_tmp154 = *(&llvm_cbe_ptr_addr);
  *(&llvm_cbe_ptr_addr) = (&llvm_cbe_tmp154[((unsigned int )1)]);
  if (((llvm_cbe_options & ((unsigned int )8)) == ((unsigned int )0))) {
    llvm_cbe_next_018_1__PHI_TEMPORARY = llvm_cbe_tmp146;   /* for PHI node */
    goto llvm_cbe_cond_next316;
  } else {
    llvm_cbe_next_018_0_ph__PHI_TEMPORARY = llvm_cbe_tmp146;   /* for PHI node */
    goto llvm_cbe_bb187_preheader;
  }

llvm_cbe_cond_false156:
  llvm_cbe_tmp159 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp161162 = ((unsigned int )(unsigned char )llvm_cbe_tmp137);
  llvm_cbe_tmp164 = *(&llvm_cbe_tmp159[llvm_cbe_tmp161162]);
  if ((((signed char )llvm_cbe_tmp164) > ((signed char )((unsigned char )-1)))) {
    goto llvm_cbe_cond_true168;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_true168:
  *(&llvm_cbe_ptr_addr) = (&llvm_cbe_tmp136[((unsigned int )1)]);
  if (((llvm_cbe_options & ((unsigned int )8)) == ((unsigned int )0))) {
    llvm_cbe_next_018_1__PHI_TEMPORARY = llvm_cbe_tmp161162;   /* for PHI node */
    goto llvm_cbe_cond_next316;
  } else {
    llvm_cbe_next_018_0_ph__PHI_TEMPORARY = llvm_cbe_tmp161162;   /* for PHI node */
    goto llvm_cbe_bb187_preheader;
  }

llvm_cbe_bb187_preheader:
  llvm_cbe_next_018_0_ph = llvm_cbe_next_018_0_ph__PHI_TEMPORARY;
  llvm_cbe_tmp189172 = &llvm_cbe_cd->field3;
  llvm_cbe_tmp209 = &llvm_cbe_cd->field18;
  llvm_cbe_tmp216 = &llvm_cbe_cd->field7;
  llvm_cbe_tmp225 = &llvm_cbe_cd->field19;
  llvm_cbe_tmp264 = &llvm_cbe_cd->field20[((unsigned int )0)];
  llvm_cbe_tmp284 = &llvm_cbe_cd->field20[((unsigned int )1)];
  goto llvm_cbe_bb187_outer;

  do {     /* Syntactic loop 'bb187.outer' to make GCC happy */
llvm_cbe_bb187_outer:
  llvm_cbe_tmp190173 = *llvm_cbe_tmp189172;
  llvm_cbe_tmp191174 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp192175 = *llvm_cbe_tmp191174;
  llvm_cbe_tmp195178 = *(&llvm_cbe_tmp190173[(((unsigned int )(unsigned char )llvm_cbe_tmp192175))]);
  if (((((unsigned char )(llvm_cbe_tmp195178 & ((unsigned char )1)))) == ((unsigned char )0))) {
    llvm_cbe_tmp185_lcssa__PHI_TEMPORARY = llvm_cbe_tmp191174;   /* for PHI node */
    goto llvm_cbe_bb200;
  } else {
    llvm_cbe_tmp185182__PHI_TEMPORARY = llvm_cbe_tmp191174;   /* for PHI node */
    goto llvm_cbe_bb184;
  }

llvm_cbe_cond_true297:
  llvm_cbe_tmp298 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp301 = *llvm_cbe_tmp225;
  *(&llvm_cbe_ptr_addr) = (&llvm_cbe_tmp298[llvm_cbe_tmp301]);
  goto llvm_cbe_bb187_outer;

  do {     /* Syntactic loop 'bb304' to make GCC happy */
llvm_cbe_bb304:
  llvm_cbe_tmp305 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp306 = &llvm_cbe_tmp305[((unsigned int )1)];
  *(&llvm_cbe_ptr_addr) = llvm_cbe_tmp306;
  llvm_cbe_tmp308 = *llvm_cbe_tmp306;
  if ((llvm_cbe_tmp308 == ((unsigned char )0))) {
    goto llvm_cbe_bb187_outer;
  } else {
    goto llvm_cbe_bb207;
  }

llvm_cbe_cond_true214:
  if ((((unsigned char *)llvm_cbe_tmp217) > ((unsigned char *)llvm_cbe_tmp306))) {
    goto llvm_cbe_cond_next223;
  } else {
    goto llvm_cbe_bb304;
  }

llvm_cbe_bb207:
  llvm_cbe_tmp210 = *llvm_cbe_tmp209;
  llvm_cbe_tmp217 = *llvm_cbe_tmp216;
  if ((llvm_cbe_tmp210 == ((unsigned int )0))) {
    goto llvm_cbe_cond_false244;
  } else {
    goto llvm_cbe_cond_true214;
  }

llvm_cbe_cond_next223:
  llvm_cbe_tmp234 = _pcre_is_newline(llvm_cbe_tmp306, llvm_cbe_tmp210, llvm_cbe_tmp217, llvm_cbe_tmp225);
  if ((llvm_cbe_tmp234 == ((unsigned int )0))) {
    goto llvm_cbe_bb304;
  } else {
    goto llvm_cbe_cond_true297;
  }

llvm_cbe_cond_false244:
  llvm_cbe_tmp250 = *llvm_cbe_tmp225;
  if ((((unsigned char *)(&llvm_cbe_tmp217[(-(llvm_cbe_tmp250))])) < ((unsigned char *)llvm_cbe_tmp306))) {
    goto llvm_cbe_bb304;
  } else {
    goto llvm_cbe_cond_next258;
  }

llvm_cbe_cond_next258:
  llvm_cbe_tmp265 = *llvm_cbe_tmp264;
  if ((llvm_cbe_tmp308 == llvm_cbe_tmp265)) {
    goto llvm_cbe_cond_next270;
  } else {
    goto llvm_cbe_bb304;
  }

llvm_cbe_cond_next278:
  llvm_cbe_tmp281 = *(&llvm_cbe_tmp305[((unsigned int )2)]);
  llvm_cbe_tmp285 = *llvm_cbe_tmp284;
  if ((llvm_cbe_tmp281 == llvm_cbe_tmp285)) {
    goto llvm_cbe_cond_true297;
  } else {
    goto llvm_cbe_bb304;
  }

llvm_cbe_cond_next270:
  if ((llvm_cbe_tmp250 == ((unsigned int )1))) {
    goto llvm_cbe_cond_true297;
  } else {
    goto llvm_cbe_cond_next278;
  }

  } while (1); /* end of syntactic loop 'bb304' */
llvm_cbe_bb200:
  llvm_cbe_tmp185_lcssa = llvm_cbe_tmp185_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp202 = *llvm_cbe_tmp185_lcssa;
  if ((llvm_cbe_tmp202 == ((unsigned char )35))) {
    goto llvm_cbe_bb304;
  } else {
    llvm_cbe_next_018_1__PHI_TEMPORARY = llvm_cbe_next_018_0_ph;   /* for PHI node */
    goto llvm_cbe_cond_next316;
  }

  do {     /* Syntactic loop 'bb184' to make GCC happy */
llvm_cbe_bb184:
  llvm_cbe_tmp185182 = llvm_cbe_tmp185182__PHI_TEMPORARY;
  llvm_cbe_tmp186 = &llvm_cbe_tmp185182[((unsigned int )1)];
  *(&llvm_cbe_ptr_addr) = llvm_cbe_tmp186;
  llvm_cbe_tmp192 = *llvm_cbe_tmp186;
  llvm_cbe_tmp195 = *(&llvm_cbe_tmp190173[(((unsigned int )(unsigned char )llvm_cbe_tmp192))]);
  if (((((unsigned char )(llvm_cbe_tmp195 & ((unsigned char )1)))) == ((unsigned char )0))) {
    llvm_cbe_tmp185_lcssa__PHI_TEMPORARY = llvm_cbe_tmp186;   /* for PHI node */
    goto llvm_cbe_bb200;
  } else {
    llvm_cbe_tmp185182__PHI_TEMPORARY = llvm_cbe_tmp186;   /* for PHI node */
    goto llvm_cbe_bb184;
  }

  } while (1); /* end of syntactic loop 'bb184' */
  } while (1); /* end of syntactic loop 'bb187.outer' */
llvm_cbe_cond_next316:
  llvm_cbe_next_018_1 = llvm_cbe_next_018_1__PHI_TEMPORARY;
  llvm_cbe_tmp317 = *(&llvm_cbe_ptr_addr);
  llvm_cbe_tmp318 = *llvm_cbe_tmp317;
  switch (llvm_cbe_tmp318) {
  default:
    goto llvm_cbe_cond_next331;
;
  case ((unsigned char )42):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned char )63):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  }
llvm_cbe_cond_next331:
  llvm_cbe_tmp334 = strncmp(llvm_cbe_tmp317, (&(_2E_str73[((unsigned int )0)])), ((unsigned int )3));
  if ((llvm_cbe_tmp334 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_bb340;
  }

llvm_cbe_bb340:
  if ((((signed int )llvm_cbe_next_018_1) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true345;
  } else {
    goto llvm_cbe_cond_next608;
  }

llvm_cbe_cond_true345:
  switch (llvm_cbe_op_code) {
  default:
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )6):
    goto llvm_cbe_bb432;
  case ((unsigned int )7):
    goto llvm_cbe_bb408;
  case ((unsigned int )8):
    goto llvm_cbe_bb481;
  case ((unsigned int )9):
    goto llvm_cbe_bb458;
  case ((unsigned int )10):
    goto llvm_cbe_bb530;
  case ((unsigned int )11):
    goto llvm_cbe_bb506;
  case ((unsigned int )17):
    goto llvm_cbe_bb557;
  case ((unsigned int )18):
    goto llvm_cbe_bb557;
  case ((unsigned int )19):
    goto llvm_cbe_bb588;
  case ((unsigned int )20):
    goto llvm_cbe_bb588;
  case ((unsigned int )27):
    goto llvm_cbe_bb347;
    break;
  case ((unsigned int )28):
    goto llvm_cbe_bb353;
  case ((unsigned int )29):
    goto llvm_cbe_bb373;
  }
llvm_cbe_bb347:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 != llvm_cbe_item)));
llvm_cbe_bb353:
  if ((llvm_cbe_next_018_1 == llvm_cbe_item)) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next361;
  }

llvm_cbe_cond_next361:
  llvm_cbe_tmp364 = *(&llvm_cbe_cd->field1);
  llvm_cbe_tmp367 = *(&llvm_cbe_tmp364[llvm_cbe_next_018_1]);
  return (((unsigned int )(bool )((((unsigned int )(unsigned char )llvm_cbe_tmp367)) != llvm_cbe_item)));
llvm_cbe_bb373:
  if ((((signed int )llvm_cbe_next_018_1) < ((signed int )((unsigned int )0)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next380;
  }

llvm_cbe_cond_next380:
  if ((llvm_cbe_next_018_1 == llvm_cbe_item)) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next388;
  }

llvm_cbe_cond_next388:
  if (((llvm_cbe_options & ((unsigned int )1)) == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next396;
  }

llvm_cbe_cond_next396:
  llvm_cbe_tmp399 = *(&llvm_cbe_cd->field1);
  llvm_cbe_tmp402 = *(&llvm_cbe_tmp399[llvm_cbe_next_018_1]);
  return (((unsigned int )(bool )((((unsigned int )(unsigned char )llvm_cbe_tmp402)) == llvm_cbe_item)));
llvm_cbe_bb408:
  if ((((signed int )llvm_cbe_next_018_1) > ((signed int )((unsigned int )127)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next414;
  }

llvm_cbe_cond_next414:
  llvm_cbe_tmp417 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp420 = *(&llvm_cbe_tmp417[llvm_cbe_next_018_1]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp420) >> ((unsigned char )((unsigned char )2))))))) & ((unsigned char )1)))) ^ ((unsigned char )1))))));
llvm_cbe_bb432:
  if ((((signed int )llvm_cbe_next_018_1) > ((signed int )((unsigned int )127)))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_cond_next438;
  }

llvm_cbe_cond_next438:
  llvm_cbe_tmp441 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp444 = *(&llvm_cbe_tmp441[llvm_cbe_next_018_1]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp444) >> ((unsigned char )((unsigned char )2))))))) & ((unsigned char )1))))));
llvm_cbe_bb458:
  if ((((signed int )llvm_cbe_next_018_1) > ((signed int )((unsigned int )127)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next464;
  }

llvm_cbe_cond_next464:
  llvm_cbe_tmp467 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp470 = *(&llvm_cbe_tmp467[llvm_cbe_next_018_1]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )(llvm_cbe_tmp470 & ((unsigned char )1)))) ^ ((unsigned char )1))))));
llvm_cbe_bb481:
  if ((((signed int )llvm_cbe_next_018_1) > ((signed int )((unsigned int )127)))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_cond_next487;
  }

llvm_cbe_cond_next487:
  llvm_cbe_tmp490 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp493 = *(&llvm_cbe_tmp490[llvm_cbe_next_018_1]);
  return (((unsigned int )(unsigned char )(((unsigned char )(llvm_cbe_tmp493 & ((unsigned char )1))))));
llvm_cbe_bb506:
  if ((((signed int )llvm_cbe_next_018_1) > ((signed int )((unsigned int )127)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next512;
  }

llvm_cbe_cond_next512:
  llvm_cbe_tmp515 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp518 = *(&llvm_cbe_tmp515[llvm_cbe_next_018_1]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp518) >> ((unsigned char )((unsigned char )4))))))) & ((unsigned char )1)))) ^ ((unsigned char )1))))));
llvm_cbe_bb530:
  if ((((signed int )llvm_cbe_next_018_1) > ((signed int )((unsigned int )127)))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_cond_next536;
  }

llvm_cbe_cond_next536:
  llvm_cbe_tmp539 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp542 = *(&llvm_cbe_tmp539[llvm_cbe_next_018_1]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp542) >> ((unsigned char )((unsigned char )4))))))) & ((unsigned char )1))))));
llvm_cbe_bb557:
  switch (llvm_cbe_next_018_1) {
  default:
    goto llvm_cbe_bb582;
;
  case ((unsigned int )9):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )32):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )160):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )5760):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )6158):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8192):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8193):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8194):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8195):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8196):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8197):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8198):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8199):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8200):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8201):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8202):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8239):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )8287):
    goto llvm_cbe_bb577;
    break;
  case ((unsigned int )12288):
    goto llvm_cbe_bb577;
    break;
  }
llvm_cbe_bb577:
  return (((unsigned int )(bool )(llvm_cbe_op_code != ((unsigned int )18))));
llvm_cbe_bb582:
  return (((unsigned int )(bool )(llvm_cbe_op_code == ((unsigned int )18))));
llvm_cbe_bb588:
  switch (llvm_cbe_next_018_1) {
  default:
    goto llvm_cbe_bb601;
;
  case ((unsigned int )10):
    goto llvm_cbe_bb596;
    break;
  case ((unsigned int )11):
    goto llvm_cbe_bb596;
    break;
  case ((unsigned int )12):
    goto llvm_cbe_bb596;
    break;
  case ((unsigned int )13):
    goto llvm_cbe_bb596;
    break;
  case ((unsigned int )133):
    goto llvm_cbe_bb596;
    break;
  case ((unsigned int )8232):
    goto llvm_cbe_bb596;
    break;
  case ((unsigned int )8233):
    goto llvm_cbe_bb596;
    break;
  }
llvm_cbe_bb596:
  return (((unsigned int )(bool )(llvm_cbe_op_code != ((unsigned int )20))));
llvm_cbe_bb601:
  return (((unsigned int )(bool )(llvm_cbe_op_code == ((unsigned int )20))));
llvm_cbe_cond_next608:
  switch (llvm_cbe_op_code) {
  default:
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )6):
    goto llvm_cbe_bb850;
  case ((unsigned int )7):
    goto llvm_cbe_bb814;
  case ((unsigned int )8):
    goto llvm_cbe_bb868;
  case ((unsigned int )9):
    goto llvm_cbe_bb855;
  case ((unsigned int )10):
    goto llvm_cbe_bb978;
  case ((unsigned int )11):
    goto llvm_cbe_bb954;
  case ((unsigned int )17):
    goto llvm_cbe_bb921;
  case ((unsigned int )18):
    goto llvm_cbe_bb891;
  case ((unsigned int )19):
    goto llvm_cbe_bb949;
  case ((unsigned int )20):
    goto llvm_cbe_bb926;
  case ((unsigned int )27):
    goto llvm_cbe_bb611;
    break;
  case ((unsigned int )28):
    goto llvm_cbe_bb611;
    break;
  }
llvm_cbe_bb611:
  switch ((-(llvm_cbe_next_018_1))) {
  default:
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )6):
    goto llvm_cbe_bb638;
  case ((unsigned int )7):
    goto llvm_cbe_bb614;
    break;
  case ((unsigned int )8):
    goto llvm_cbe_bb687;
  case ((unsigned int )9):
    goto llvm_cbe_bb664;
  case ((unsigned int )10):
    goto llvm_cbe_bb736;
  case ((unsigned int )11):
    goto llvm_cbe_bb712;
  case ((unsigned int )17):
    goto llvm_cbe_bb763;
  case ((unsigned int )18):
    goto llvm_cbe_bb763;
  case ((unsigned int )19):
    goto llvm_cbe_bb794;
  case ((unsigned int )20):
    goto llvm_cbe_bb794;
  }
llvm_cbe_bb614:
  if ((((signed int )llvm_cbe_item) > ((signed int )((unsigned int )127)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next620;
  }

llvm_cbe_cond_next620:
  llvm_cbe_tmp623 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp626 = *(&llvm_cbe_tmp623[llvm_cbe_item]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp626) >> ((unsigned char )((unsigned char )2))))))) & ((unsigned char )1)))) ^ ((unsigned char )1))))));
llvm_cbe_bb638:
  if ((((signed int )llvm_cbe_item) > ((signed int )((unsigned int )127)))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_cond_next644;
  }

llvm_cbe_cond_next644:
  llvm_cbe_tmp647 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp650 = *(&llvm_cbe_tmp647[llvm_cbe_item]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp650) >> ((unsigned char )((unsigned char )2))))))) & ((unsigned char )1))))));
llvm_cbe_bb664:
  if ((((signed int )llvm_cbe_item) > ((signed int )((unsigned int )127)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next670;
  }

llvm_cbe_cond_next670:
  llvm_cbe_tmp673 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp676 = *(&llvm_cbe_tmp673[llvm_cbe_item]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )(llvm_cbe_tmp676 & ((unsigned char )1)))) ^ ((unsigned char )1))))));
llvm_cbe_bb687:
  if ((((signed int )llvm_cbe_item) > ((signed int )((unsigned int )127)))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_cond_next693;
  }

llvm_cbe_cond_next693:
  llvm_cbe_tmp696 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp699 = *(&llvm_cbe_tmp696[llvm_cbe_item]);
  return (((unsigned int )(unsigned char )(((unsigned char )(llvm_cbe_tmp699 & ((unsigned char )1))))));
llvm_cbe_bb712:
  if ((((signed int )llvm_cbe_item) > ((signed int )((unsigned int )127)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next718;
  }

llvm_cbe_cond_next718:
  llvm_cbe_tmp721 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp724 = *(&llvm_cbe_tmp721[llvm_cbe_item]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp724) >> ((unsigned char )((unsigned char )4))))))) & ((unsigned char )1)))) ^ ((unsigned char )1))))));
llvm_cbe_bb736:
  if ((((signed int )llvm_cbe_item) > ((signed int )((unsigned int )127)))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_cond_next742;
  }

llvm_cbe_cond_next742:
  llvm_cbe_tmp745 = *(&llvm_cbe_cd->field3);
  llvm_cbe_tmp748 = *(&llvm_cbe_tmp745[llvm_cbe_item]);
  return (((unsigned int )(unsigned char )(((unsigned char )((((unsigned char )(((unsigned char )(((unsigned char )llvm_cbe_tmp748) >> ((unsigned char )((unsigned char )4))))))) & ((unsigned char )1))))));
llvm_cbe_bb763:
  switch (llvm_cbe_item) {
  default:
    goto llvm_cbe_bb788;
;
  case ((unsigned int )9):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )32):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )160):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )5760):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )6158):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8192):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8193):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8194):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8195):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8196):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8197):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8198):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8199):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8200):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8201):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8202):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8239):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )8287):
    goto llvm_cbe_bb783;
    break;
  case ((unsigned int )12288):
    goto llvm_cbe_bb783;
    break;
  }
llvm_cbe_bb783:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 != ((unsigned int )-18))));
llvm_cbe_bb788:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 == ((unsigned int )-18))));
llvm_cbe_bb794:
  switch (llvm_cbe_item) {
  default:
    goto llvm_cbe_bb807;
;
  case ((unsigned int )10):
    goto llvm_cbe_bb802;
    break;
  case ((unsigned int )11):
    goto llvm_cbe_bb802;
    break;
  case ((unsigned int )12):
    goto llvm_cbe_bb802;
    break;
  case ((unsigned int )13):
    goto llvm_cbe_bb802;
    break;
  case ((unsigned int )133):
    goto llvm_cbe_bb802;
    break;
  case ((unsigned int )8232):
    goto llvm_cbe_bb802;
    break;
  case ((unsigned int )8233):
    goto llvm_cbe_bb802;
    break;
  }
llvm_cbe_bb802:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 != ((unsigned int )-20))));
llvm_cbe_bb807:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 == ((unsigned int )-20))));
llvm_cbe_bb814:
  switch (llvm_cbe_next_018_1) {
  default:
    goto llvm_cbe_UnifiedReturnBlock;
;
  case ((unsigned int )-9):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-6):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-20):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-18):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-10):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }
llvm_cbe_bb850:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 == ((unsigned int )-7))));
llvm_cbe_bb855:
  return (((unsigned int )(bool )((((unsigned int )(llvm_cbe_next_018_1 + ((unsigned int )8))) < ((unsigned int )((unsigned int )2))) | (llvm_cbe_next_018_1 == ((unsigned int )-11)))));
llvm_cbe_bb868:
  switch (llvm_cbe_next_018_1) {
  default:
    goto llvm_cbe_UnifiedReturnBlock;
;
  case ((unsigned int )-20):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-18):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-9):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }
llvm_cbe_bb891:
  switch (llvm_cbe_next_018_1) {
  default:
    goto llvm_cbe_UnifiedReturnBlock;
;
  case ((unsigned int )-17):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-8):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-11):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-7):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }
llvm_cbe_bb921:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 == ((unsigned int )-18))));
llvm_cbe_bb926:
  switch (llvm_cbe_next_018_1) {
  default:
    goto llvm_cbe_UnifiedReturnBlock;
;
  case ((unsigned int )-19):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-11):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  case ((unsigned int )-7):
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }
llvm_cbe_bb949:
  return (((unsigned int )(bool )(llvm_cbe_next_018_1 == ((unsigned int )-20))));
llvm_cbe_bb954:
  return (((unsigned int )(bool )(((llvm_cbe_next_018_1 == ((unsigned int )-18)) | (llvm_cbe_next_018_1 == ((unsigned int )-20))) | (((unsigned int )(llvm_cbe_next_018_1 + ((unsigned int )10))) < ((unsigned int )((unsigned int )2))))));
llvm_cbe_bb978:
  return (((unsigned int )(bool )((llvm_cbe_next_018_1 == ((unsigned int )-11)) | (llvm_cbe_next_018_1 == ((unsigned int )-7)))));
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
llvm_cbe_UnifiedReturnBlock:
  return ((unsigned int )0);
}


static unsigned int _pcre_is_newline(unsigned char *llvm_cbe_ptr, unsigned int llvm_cbe_type, unsigned char *llvm_cbe_endptr, unsigned int *llvm_cbe_lenptr) {
  unsigned char llvm_cbe_tmp5;
  unsigned int llvm_cbe_tmp56;
  unsigned char llvm_cbe_tmp29;
  unsigned char llvm_cbe_tmp60;

  llvm_cbe_tmp5 = *llvm_cbe_ptr;
  llvm_cbe_tmp56 = ((unsigned int )(unsigned char )llvm_cbe_tmp5);
  if ((llvm_cbe_type == ((unsigned int )2))) {
    goto llvm_cbe_cond_true14;
  } else {
    goto llvm_cbe_cond_false42;
  }

llvm_cbe_cond_true14:
  switch (llvm_cbe_tmp56) {
  default:
    goto llvm_cbe_UnifiedReturnBlock;
;
  case ((unsigned int )10):
    goto llvm_cbe_bb;
    break;
  case ((unsigned int )13):
    goto llvm_cbe_bb18;
  }
llvm_cbe_bb:
  *llvm_cbe_lenptr = ((unsigned int )1);
  return ((unsigned int )1);
llvm_cbe_bb18:
  if ((((unsigned char *)(&llvm_cbe_endptr[((unsigned int )-1)])) > ((unsigned char *)llvm_cbe_ptr))) {
    goto llvm_cbe_cond_next26;
  } else {
    goto llvm_cbe_bb35;
  }

llvm_cbe_cond_next26:
  llvm_cbe_tmp29 = *(&llvm_cbe_ptr[((unsigned int )1)]);
  if ((llvm_cbe_tmp29 == ((unsigned char )10))) {
    goto llvm_cbe_bb36;
  } else {
    goto llvm_cbe_bb35;
  }

llvm_cbe_bb35:
  *llvm_cbe_lenptr = ((unsigned int )1);
  return ((unsigned int )1);
llvm_cbe_bb36:
  *llvm_cbe_lenptr = ((unsigned int )2);
  return ((unsigned int )1);
llvm_cbe_cond_false42:
  switch (llvm_cbe_tmp56) {
  default:
    goto llvm_cbe_UnifiedReturnBlock;
;
  case ((unsigned int )10):
    goto llvm_cbe_bb46;
    break;
  case ((unsigned int )11):
    goto llvm_cbe_bb46;
    break;
  case ((unsigned int )12):
    goto llvm_cbe_bb46;
    break;
  case ((unsigned int )13):
    goto llvm_cbe_bb49;
  case ((unsigned int )133):
    goto llvm_cbe_bb71;
  case ((unsigned int )8232):
    goto llvm_cbe_bb83;
  case ((unsigned int )8233):
    goto llvm_cbe_bb83;
  }
llvm_cbe_bb46:
  *llvm_cbe_lenptr = ((unsigned int )1);
  return ((unsigned int )1);
llvm_cbe_bb49:
  if ((((unsigned char *)(&llvm_cbe_endptr[((unsigned int )-1)])) > ((unsigned char *)llvm_cbe_ptr))) {
    goto llvm_cbe_cond_next57;
  } else {
    goto llvm_cbe_bb66;
  }

llvm_cbe_cond_next57:
  llvm_cbe_tmp60 = *(&llvm_cbe_ptr[((unsigned int )1)]);
  if ((llvm_cbe_tmp60 == ((unsigned char )10))) {
    goto llvm_cbe_bb67;
  } else {
    goto llvm_cbe_bb66;
  }

llvm_cbe_bb66:
  *llvm_cbe_lenptr = ((unsigned int )1);
  return ((unsigned int )1);
llvm_cbe_bb67:
  *llvm_cbe_lenptr = ((unsigned int )2);
  return ((unsigned int )1);
llvm_cbe_bb71:
  *llvm_cbe_lenptr = ((unsigned int )1);
  return ((unsigned int )1);
llvm_cbe_bb83:
  *llvm_cbe_lenptr = ((unsigned int )3);
  return ((unsigned int )1);
llvm_cbe_UnifiedReturnBlock:
  return ((unsigned int )0);
}


static unsigned int compile_regex(unsigned int llvm_cbe_options, unsigned int llvm_cbe_oldims, unsigned char **llvm_cbe_codeptr, unsigned char **llvm_cbe_ptrptr, unsigned int *llvm_cbe_errorcodeptr, unsigned int llvm_cbe_lookbehind, unsigned int llvm_cbe_reset_bracount, unsigned int llvm_cbe_skipbytes, unsigned int *llvm_cbe_firstbyteptr, unsigned int *llvm_cbe_reqbyteptr, struct l_struct_2E_branch_chain *llvm_cbe_bcptr, struct l_struct_2E_compile_data *llvm_cbe_cd, unsigned int *llvm_cbe_lengthptr) {
  unsigned int llvm_cbe_length;    /* Address-exposed local */
  struct l_struct_2E_branch_chain llvm_cbe_bc;    /* Address-exposed local */
  unsigned int llvm_cbe_length_prevgroup;    /* Address-exposed local */
  unsigned char *llvm_cbe_tempcode;    /* Address-exposed local */
  unsigned char *llvm_cbe_ptr106;    /* Address-exposed local */
  unsigned char *llvm_cbe_tempptr;    /* Address-exposed local */
  unsigned char llvm_cbe_classbits[32];    /* Address-exposed local */
  unsigned int llvm_cbe_subreqbyte;    /* Address-exposed local */
  unsigned int llvm_cbe_subfirstbyte;    /* Address-exposed local */
  unsigned char llvm_cbe_mcbuffer[8];    /* Address-exposed local */
  unsigned char llvm_cbe_pbits[32];    /* Address-exposed local */
  unsigned int llvm_cbe_set;    /* Address-exposed local */
  unsigned int llvm_cbe_unset;    /* Address-exposed local */
  unsigned char *llvm_cbe_tmp3;
  unsigned char *llvm_cbe_tmp5;
  unsigned char **llvm_cbe_tmp11;
  unsigned int *llvm_cbe_tmp24;
  unsigned int llvm_cbe_tmp25;
  unsigned int llvm_cbe_tmp21;
  unsigned char *llvm_cbe_tmp22;
  unsigned char *llvm_cbe_slot_913437_0_us61_8;
  unsigned char *llvm_cbe_slot_913437_0_us61_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_8;
  unsigned char *llvm_cbe_slot_913437_059_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchreqbyte_413514_0;
  unsigned int llvm_cbe_branchreqbyte_413514_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchfirstbyte_113557_0;
  unsigned int llvm_cbe_branchfirstbyte_113557_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_113600_0;
  unsigned char *llvm_cbe_code_113600_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_013604_0;
  unsigned char *llvm_cbe_ptr_013604_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_313605_0;
  unsigned int llvm_cbe_options_addr_313605_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_max_bracount_113905_0;
  unsigned int llvm_cbe_max_bracount_113905_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte_313907_0;
  unsigned int llvm_cbe_reqbyte_313907_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte_313911_0;
  unsigned int llvm_cbe_firstbyte_313911_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_reverse_count_113916_0;
  unsigned char *llvm_cbe_reverse_count_113916_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_branch_413917_0;
  unsigned char *llvm_cbe_last_branch_413917_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_0_us61_11;
  unsigned char *llvm_cbe_slot_913437_0_us61_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_11;
  unsigned char *llvm_cbe_slot_913437_059_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchreqbyte_413514_1_ph;
  unsigned int llvm_cbe_branchreqbyte_413514_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchfirstbyte_113557_1_ph;
  unsigned int llvm_cbe_branchfirstbyte_113557_1_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_113600_1_ph;
  unsigned char *llvm_cbe_code_113600_1_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_013604_1_ph;
  unsigned char *llvm_cbe_ptr_013604_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_313605_1_ph;
  unsigned int llvm_cbe_options_addr_313605_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_max_bracount_113905_1_ph;
  unsigned int llvm_cbe_max_bracount_113905_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte_313907_1_ph;
  unsigned int llvm_cbe_reqbyte_313907_1_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte_313911_1_ph;
  unsigned int llvm_cbe_firstbyte_313911_1_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_reverse_count_113916_1_ph;
  unsigned char *llvm_cbe_reverse_count_113916_1_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_branch_413917_1_ph;
  unsigned char *llvm_cbe_last_branch_413917_1_ph__PHI_TEMPORARY;
  unsigned int *llvm_cbe_iftmp_509_0;
  unsigned char **llvm_cbe_tmp143;
  unsigned char **llvm_cbe_tmp203;
  unsigned char **llvm_cbe_tmp435;
  unsigned int *llvm_cbe_tmp453;
  unsigned char **llvm_cbe_tmp460;
  unsigned int *llvm_cbe_tmp469;
  unsigned char *llvm_cbe_tmp508;
  unsigned char *llvm_cbe_tmp528;
  unsigned char *llvm_cbe_classbits711;
  unsigned char **llvm_cbe_tmp776;
  unsigned char *llvm_cbe_pbits885;
  unsigned char *llvm_cbe_tmp962;
  unsigned char *llvm_cbe_tmp972;
  unsigned char *llvm_cbe_tmp1237;
  unsigned char *llvm_cbe_tmp1288;
  unsigned char *llvm_cbe_tmp1292;
  unsigned char *llvm_cbe_tmp1353;
  unsigned char **llvm_cbe_tmp1623;
  unsigned char *llvm_cbe_tmp1755;
  unsigned int *llvm_cbe_tmp1993;
  unsigned int *llvm_cbe_tmp2377;
  unsigned char **llvm_cbe_tmp3478;
  unsigned int *llvm_cbe_tmp351413356;
  unsigned int *llvm_cbe_tmp3506;
  unsigned char **llvm_cbe_tmp3783;
  unsigned int *llvm_cbe_tmp4496;
  unsigned char **llvm_cbe_tmp4563;
  unsigned int *llvm_cbe_iftmp_468_0;
  unsigned int *llvm_cbe_tmp5026;
  unsigned int *llvm_cbe_tmp5042;
  bool llvm_cbe_tmp138_not;
  unsigned char *llvm_cbe_slot_913437_0_us61_9;
  unsigned char *llvm_cbe_slot_913437_0_us61_9__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_9;
  unsigned char *llvm_cbe_slot_913437_059_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchreqbyte_413514_1;
  unsigned int llvm_cbe_branchreqbyte_413514_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchfirstbyte_113557_1;
  unsigned int llvm_cbe_branchfirstbyte_113557_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_113600_1;
  unsigned char *llvm_cbe_code_113600_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_013604_1;
  unsigned char *llvm_cbe_ptr_013604_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_313605_1;
  unsigned int llvm_cbe_options_addr_313605_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_max_bracount_113905_1;
  unsigned int llvm_cbe_max_bracount_113905_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte_313907_1;
  unsigned int llvm_cbe_reqbyte_313907_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte_313911_1;
  unsigned int llvm_cbe_firstbyte_313911_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_reverse_count_113916_1;
  unsigned char *llvm_cbe_reverse_count_113916_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_branch_413917_1;
  unsigned char *llvm_cbe_last_branch_413917_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp46;
  unsigned int llvm_cbe_tmp47;
  unsigned char *llvm_cbe_code_0;
  unsigned char *llvm_cbe_code_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp56;
  unsigned char *llvm_cbe_tmp63;
  unsigned int llvm_cbe_tmp64;
  unsigned char *llvm_cbe_code_2;
  unsigned char *llvm_cbe_code_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_reverse_count_0;
  unsigned char *llvm_cbe_reverse_count_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp117;
  unsigned int llvm_cbe_tmp119;
  unsigned int llvm_cbe_req_caseopt_3_ph;
  unsigned int llvm_cbe_tmp188189;
  unsigned char *llvm_cbe_slot_913437_0_us61_13;
  unsigned char *llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_13;
  unsigned char *llvm_cbe_slot_913437_059_13__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_max_10;
  unsigned int llvm_cbe_repeat_max_10__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_min_10;
  unsigned int llvm_cbe_repeat_min_10__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_2;
  unsigned int llvm_cbe_options_addr_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_7;
  unsigned char *llvm_cbe_save_hwm_7__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_5;
  unsigned char *llvm_cbe_previous_callout_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_3;
  unsigned char *llvm_cbe_previous_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_2;
  unsigned int llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_2;
  unsigned int llvm_cbe_inescq_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_2;
  unsigned char *llvm_cbe_last_code_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_3;
  unsigned char *llvm_cbe_code105_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_7;
  unsigned int llvm_cbe_after_manual_callout_7__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_3;
  unsigned int llvm_cbe_options104_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_3;
  unsigned int llvm_cbe_req_caseopt_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_2;
  unsigned int llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_2;
  unsigned int llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_5;
  unsigned int llvm_cbe_reqbyte103_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_3;
  unsigned int llvm_cbe_firstbyte102_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_3;
  unsigned int llvm_cbe_greedy_non_default_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_3;
  unsigned int llvm_cbe_greedy_default_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp134;
  unsigned char llvm_cbe_tmp135;
  unsigned int llvm_cbe_tmp135136;
  unsigned int llvm_cbe_options_addr_25789_3;
  unsigned int llvm_cbe_options_addr_25789_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_3;
  unsigned char *llvm_cbe_save_hwm_79347_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_59381_3;
  unsigned char *llvm_cbe_previous_callout_59381_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_39389_3;
  unsigned char *llvm_cbe_previous_39389_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_3;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_29422_3;
  unsigned int llvm_cbe_inescq_29422_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_29452_3;
  unsigned char *llvm_cbe_last_code_29452_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_39456_3;
  unsigned char *llvm_cbe_code105_39456_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_79460_3;
  unsigned int llvm_cbe_after_manual_callout_79460_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_3;
  unsigned int llvm_cbe_options104_39466_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_3;
  unsigned int llvm_cbe_req_caseopt_39708_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_29740_3;
  unsigned int llvm_cbe_zerofirstbyte_29740_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_29762_3;
  unsigned int llvm_cbe_zeroreqbyte_29762_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_3;
  unsigned int llvm_cbe_reqbyte103_59784_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_3;
  unsigned int llvm_cbe_firstbyte102_39829_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_3;
  unsigned int llvm_cbe_greedy_non_default_39879_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_3;
  unsigned int llvm_cbe_greedy_default_39909_3__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp13512351_3;
  unsigned char llvm_cbe_tmp13512351_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp13513612358_3;
  unsigned int llvm_cbe_tmp13513612358_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp144;
  unsigned char *llvm_cbe_tmp51985596;
  unsigned char *llvm_cbe_code105_1;
  unsigned int llvm_cbe_tmp162;
  unsigned int llvm_cbe_tmp163164;
  unsigned int llvm_cbe_tmp183184;
  unsigned char *ltmp_4_1;
  unsigned char *llvm_cbe_tmp194;
  unsigned int llvm_cbe_options_addr_25789_4;
  unsigned int llvm_cbe_options_addr_25789_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_4;
  unsigned char *llvm_cbe_save_hwm_79347_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_59381_4;
  unsigned char *llvm_cbe_previous_callout_59381_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_39389_4;
  unsigned char *llvm_cbe_previous_39389_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_4;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_29422_4;
  unsigned int llvm_cbe_inescq_29422_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_29452_4;
  unsigned char *llvm_cbe_last_code_29452_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_39456_4;
  unsigned char *llvm_cbe_code105_39456_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_79460_4;
  unsigned int llvm_cbe_after_manual_callout_79460_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_4;
  unsigned int llvm_cbe_options104_39466_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_4;
  unsigned int llvm_cbe_req_caseopt_39708_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_29740_4;
  unsigned int llvm_cbe_zerofirstbyte_29740_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_29762_4;
  unsigned int llvm_cbe_zeroreqbyte_29762_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_4;
  unsigned int llvm_cbe_reqbyte103_59784_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_4;
  unsigned int llvm_cbe_firstbyte102_39829_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_4;
  unsigned int llvm_cbe_greedy_non_default_39879_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_4;
  unsigned int llvm_cbe_greedy_default_39909_4__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp13512351_4;
  unsigned char llvm_cbe_tmp13512351_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp13513612358_4;
  unsigned int llvm_cbe_tmp13513612358_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp204;
  unsigned char *llvm_cbe_tmp207;
  unsigned char *llvm_cbe_tmp51985785;
  unsigned int llvm_cbe_options_addr_25789_5;
  unsigned int llvm_cbe_options_addr_25789_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_5;
  unsigned char *llvm_cbe_save_hwm_79347_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_59381_5;
  unsigned char *llvm_cbe_previous_callout_59381_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_39389_5;
  unsigned char *llvm_cbe_previous_39389_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_5;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_29422_5;
  unsigned int llvm_cbe_inescq_29422_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_79460_5;
  unsigned int llvm_cbe_after_manual_callout_79460_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_5;
  unsigned int llvm_cbe_options104_39466_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_5;
  unsigned int llvm_cbe_req_caseopt_39708_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_29740_5;
  unsigned int llvm_cbe_zerofirstbyte_29740_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_29762_5;
  unsigned int llvm_cbe_zeroreqbyte_29762_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_5;
  unsigned int llvm_cbe_reqbyte103_59784_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_5;
  unsigned int llvm_cbe_firstbyte102_39829_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_5;
  unsigned int llvm_cbe_greedy_non_default_39879_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_5;
  unsigned int llvm_cbe_greedy_default_39909_5__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp13512351_5;
  unsigned char llvm_cbe_tmp13512351_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp13513612358_5;
  unsigned int llvm_cbe_tmp13513612358_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_1;
  unsigned char *llvm_cbe_last_code_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_2;
  unsigned char *llvm_cbe_code105_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_25708_0;
  unsigned char *llvm_cbe_previous_25708_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_15771_0;
  unsigned char *llvm_cbe_last_code_15771_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_25773_0;
  unsigned char *llvm_cbe_code105_25773_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_25789_8;
  unsigned int llvm_cbe_options_addr_25789_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_8;
  unsigned char *llvm_cbe_save_hwm_79347_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_59381_8;
  unsigned char *llvm_cbe_previous_callout_59381_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_8;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_29422_8;
  unsigned int llvm_cbe_inescq_29422_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_79460_8;
  unsigned int llvm_cbe_after_manual_callout_79460_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_8;
  unsigned int llvm_cbe_options104_39466_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_8;
  unsigned int llvm_cbe_req_caseopt_39708_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_29740_8;
  unsigned int llvm_cbe_zerofirstbyte_29740_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_29762_8;
  unsigned int llvm_cbe_zeroreqbyte_29762_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_8;
  unsigned int llvm_cbe_reqbyte103_59784_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_8;
  unsigned int llvm_cbe_firstbyte102_39829_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_8;
  unsigned int llvm_cbe_greedy_non_default_39879_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_8;
  unsigned int llvm_cbe_greedy_default_39909_8__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp13512351_8;
  unsigned char llvm_cbe_tmp13512351_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp13513612358_8;
  unsigned int llvm_cbe_tmp13513612358_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp234;
  unsigned char llvm_cbe_tmp236;
  unsigned char *llvm_cbe_tmp51975787;
  unsigned char llvm_cbe_tmp13512365;
  unsigned int llvm_cbe_tmp13513612367;
  unsigned char *llvm_cbe_tmp255;
  unsigned char *llvm_cbe_tmp4_i;
  unsigned char llvm_cbe_tmp9_i;
  unsigned char llvm_cbe_tmp14_i;
  unsigned int llvm_cbe_tmp17_i;
  unsigned char *llvm_cbe_tmp267;
  unsigned char *llvm_cbe_tmp11_i4;
  unsigned char *llvm_cbe_tmp23_i;
  unsigned char *llvm_cbe_tmp35_i;
  unsigned char *llvm_cbe_previous_25708_1;
  unsigned char *llvm_cbe_previous_25708_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_15771_1;
  unsigned char *llvm_cbe_last_code_15771_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_25773_1;
  unsigned char *llvm_cbe_code105_25773_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_25789_9;
  unsigned int llvm_cbe_options_addr_25789_9__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_9;
  unsigned char *llvm_cbe_save_hwm_79347_9__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_59381_9;
  unsigned char *llvm_cbe_previous_callout_59381_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_9;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_29422_9;
  unsigned int llvm_cbe_inescq_29422_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_79460_9;
  unsigned int llvm_cbe_after_manual_callout_79460_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_9;
  unsigned int llvm_cbe_options104_39466_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_9;
  unsigned int llvm_cbe_req_caseopt_39708_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_29740_9;
  unsigned int llvm_cbe_zerofirstbyte_29740_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_29762_9;
  unsigned int llvm_cbe_zeroreqbyte_29762_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_9;
  unsigned int llvm_cbe_reqbyte103_59784_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_9;
  unsigned int llvm_cbe_firstbyte102_39829_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_9;
  unsigned int llvm_cbe_greedy_non_default_39879_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_9;
  unsigned int llvm_cbe_greedy_default_39909_9__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp13512351_9;
  unsigned char llvm_cbe_tmp13512351_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp13513612358_9;
  unsigned int llvm_cbe_tmp13513612358_9__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp293;
  unsigned char llvm_cbe_tmp297;
  unsigned char llvm_cbe_tmp300;
  unsigned char *llvm_cbe_tmp307;
  unsigned char llvm_cbe_tmp31726562;
  unsigned char llvm_cbe_tmp32026565;
  unsigned int llvm_cbe_p_1026561_rec;
  unsigned int llvm_cbe_p_1026561_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp314;
  unsigned char llvm_cbe_tmp317;
  unsigned char llvm_cbe_tmp320;
  unsigned char *llvm_cbe_p_10_lcssa;
  unsigned char *llvm_cbe_p_10_lcssa__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp328;
  unsigned char llvm_cbe_tmp346;
  unsigned char llvm_cbe_tmp357;
  unsigned char *llvm_cbe_tmp364;
  unsigned char llvm_cbe_tmp37426570;
  unsigned char llvm_cbe_tmp37726573;
  unsigned int llvm_cbe_p_1226569_rec;
  unsigned int llvm_cbe_p_1226569_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp371;
  unsigned char llvm_cbe_tmp374;
  unsigned char llvm_cbe_tmp377;
  unsigned char *llvm_cbe_p_12_lcssa;
  unsigned char *llvm_cbe_p_12_lcssa__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp385;
  bool llvm_cbe_iftmp_136_0;
  bool llvm_cbe_iftmp_136_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp414;
  unsigned char *llvm_cbe_previous_callout_59381_9_mux;
  unsigned char *llvm_cbe_tmp422;
  unsigned char *llvm_cbe_tmp4_i9;
  unsigned char llvm_cbe_tmp9_i13;
  unsigned char llvm_cbe_tmp14_i17;
  unsigned int llvm_cbe_tmp17_i20;
  unsigned char *llvm_cbe_previous_callout_6;
  unsigned char *llvm_cbe_previous_callout_6__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_1;
  unsigned int llvm_cbe_after_manual_callout_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp436;
  unsigned char llvm_cbe_tmp439;
  unsigned int llvm_cbe_tmp454;
  unsigned char *llvm_cbe_tmp461;
  unsigned int llvm_cbe_tmp478;
  unsigned int llvm_cbe_tmp494;
  unsigned char llvm_cbe_tmp509;
  unsigned char llvm_cbe_tmp525;
  unsigned char llvm_cbe_tmp529;
  unsigned char *llvm_cbe_tmp542;
  unsigned int llvm_cbe_tmp545;
  unsigned char *llvm_cbe_tmp550;
  unsigned char *llvm_cbe_tmp551;
  unsigned char llvm_cbe_tmp553;
  unsigned char *llvm_cbe_tmp558;
  unsigned char llvm_cbe_tmp559;
  unsigned int llvm_cbe_c_2;
  unsigned int llvm_cbe_c_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp582;
  unsigned char *llvm_cbe_tmp11_i30;
  unsigned char *llvm_cbe_tmp23_i36;
  unsigned char *llvm_cbe_tmp35_i42;
  unsigned char *llvm_cbe_previous_callout_9;
  unsigned char *llvm_cbe_previous_callout_9__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_10;
  unsigned char *llvm_cbe_code105_10__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp597;
  unsigned int llvm_cbe_tmp605;
  unsigned int llvm_cbe_firstbyte102_1;
  unsigned char *llvm_cbe_tmp631;
  unsigned char *llvm_cbe_tmp519612381;
  unsigned char *llvm_cbe_tmp635;
  unsigned char *llvm_cbe_tmp519612384;
  unsigned int llvm_cbe_firstbyte102_4;
  unsigned char *llvm_cbe_tmp648;
  unsigned char *llvm_cbe_tmp519612387;
  unsigned char *llvm_cbe_tmp651;
  unsigned char llvm_cbe_tmp653;
  unsigned int llvm_cbe_tmp676;
  unsigned char *llvm_cbe_tmp681;
  unsigned char llvm_cbe_tmp683;
  unsigned char *llvm_cbe_tmp694;
  unsigned char *llvm_cbe_tmp695;
  unsigned char llvm_cbe_tmp697;
  unsigned char *llvm_cbe_tmp705;
  unsigned char llvm_cbe_tmp707;
  unsigned int llvm_cbe_negate_class_0;
  unsigned int llvm_cbe_negate_class_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_c_8_in;
  unsigned char llvm_cbe_c_8_in__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_8;
  unsigned char *ltmp_5_1;
  unsigned int llvm_cbe_tmp865;
  unsigned int llvm_cbe_class_lastchar_7;
  unsigned int llvm_cbe_class_lastchar_7__PHI_TEMPORARY;
  unsigned int llvm_cbe_class_charcount_7;
  unsigned int llvm_cbe_class_charcount_7__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_4;
  unsigned int llvm_cbe_inescq_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_11;
  unsigned int llvm_cbe_c_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp728;
  unsigned char *llvm_cbe_tmp729;
  unsigned char llvm_cbe_tmp730;
  unsigned char *llvm_cbe_tmp745;
  unsigned char llvm_cbe_tmp747;
  unsigned int llvm_cbe_tmp770;
  unsigned char *llvm_cbe_tmp777;
  unsigned char *llvm_cbe_tmp778;
  unsigned char llvm_cbe_tmp780;
  unsigned char *llvm_cbe_tmp788;
  unsigned char llvm_cbe_tmp790;
  bool llvm_cbe_local_negate_9;
  bool llvm_cbe_local_negate_9__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp798;
  unsigned char *llvm_cbe_tmp800;
  unsigned int llvm_cbe_tmp802;
  unsigned int llvm_cbe_yield_912392_0;
  unsigned int llvm_cbe_yield_912392_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp810;
  unsigned char *llvm_cbe_tmp831;
  unsigned int llvm_cbe_tmp833;
  unsigned int llvm_cbe_tmp845;
  unsigned char llvm_cbe_tmp849;
  unsigned int llvm_cbe_tmp88_0;
  unsigned int llvm_cbe_tmp88_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp879;
  unsigned int llvm_cbe_tmp882;
  unsigned char *ltmp_6_1;
  unsigned int llvm_cbe_tmp889;
  unsigned int llvm_cbe_tmp893;
  unsigned int llvm_cbe_c_1312408_13;
  unsigned int llvm_cbe_c_1312408_13__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp907;
  unsigned char llvm_cbe_tmp908;
  unsigned char llvm_cbe_tmp914;
  unsigned int llvm_cbe_indvar_next26678;
  unsigned int llvm_cbe_c_1426586;
  unsigned int llvm_cbe_c_1426586__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp929;
  unsigned char llvm_cbe_tmp930;
  unsigned char llvm_cbe_tmp936;
  unsigned int llvm_cbe_tmp940;
  unsigned char llvm_cbe_tmp963;
  unsigned char llvm_cbe_tmp973;
  unsigned int llvm_cbe_c_1512416_13;
  unsigned int llvm_cbe_c_1512416_13__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp986;
  unsigned char llvm_cbe_tmp987;
  unsigned char llvm_cbe_tmp990;
  unsigned int llvm_cbe_indvar_next26683;
  unsigned int llvm_cbe_c_1726583;
  unsigned int llvm_cbe_c_1726583__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1005;
  unsigned char llvm_cbe_tmp1006;
  unsigned char llvm_cbe_tmp1009;
  unsigned int llvm_cbe_tmp1013;
  unsigned char *llvm_cbe_tmp1021;
  unsigned int llvm_cbe_tmp1033;
  unsigned int llvm_cbe_tmp1036;
  unsigned int llvm_cbe_tmp1038;
  unsigned char *llvm_cbe_tmp1067;
  unsigned char llvm_cbe_tmp1069;
  unsigned char *llvm_cbe_tmp1076;
  unsigned char llvm_cbe_tmp1077;
  unsigned int llvm_cbe_c_19;
  unsigned int llvm_cbe_c_19__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1099;
  unsigned int llvm_cbe_tmp1101;
  unsigned int llvm_cbe_c_2012437_12;
  unsigned int llvm_cbe_c_2012437_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1113;
  unsigned char llvm_cbe_tmp1114;
  unsigned char llvm_cbe_tmp1119;
  unsigned int llvm_cbe_indvar_next26667;
  unsigned int llvm_cbe_c_2112444_12;
  unsigned int llvm_cbe_c_2112444_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1134;
  unsigned char llvm_cbe_tmp1135;
  unsigned char llvm_cbe_tmp1140;
  unsigned int llvm_cbe_indvar_next26664;
  unsigned int llvm_cbe_c_2212451_12;
  unsigned int llvm_cbe_c_2212451_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1155;
  unsigned char llvm_cbe_tmp1156;
  unsigned char llvm_cbe_tmp1161;
  unsigned int llvm_cbe_indvar_next26661;
  unsigned int llvm_cbe_c_2312458_12;
  unsigned int llvm_cbe_c_2312458_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1176;
  unsigned char llvm_cbe_tmp1177;
  unsigned char llvm_cbe_tmp1182;
  unsigned int llvm_cbe_indvar_next26658;
  unsigned int llvm_cbe_c_2426580;
  unsigned int llvm_cbe_c_2426580__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1197;
  unsigned char llvm_cbe_tmp1198;
  unsigned char llvm_cbe_tmp1202;
  unsigned int llvm_cbe_tmp1206;
  unsigned char llvm_cbe_tmp1214;
  unsigned int llvm_cbe_c_2526577;
  unsigned int llvm_cbe_c_2526577__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1221;
  unsigned char llvm_cbe_tmp1222;
  unsigned char llvm_cbe_tmp1226;
  unsigned int llvm_cbe_tmp1230;
  unsigned char llvm_cbe_tmp1238;
  unsigned char llvm_cbe_tmp1285;
  unsigned char llvm_cbe_tmp1289;
  unsigned char llvm_cbe_tmp1293;
  unsigned int llvm_cbe_c_2612465_12;
  unsigned int llvm_cbe_c_2612465_12__PHI_TEMPORARY;
  unsigned char llvm_cbe_x_10;
  unsigned char llvm_cbe_x_10__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1317;
  unsigned char llvm_cbe_tmp1318;
  unsigned int llvm_cbe_indvar_next26655;
  unsigned char llvm_cbe_tmp1338;
  unsigned char llvm_cbe_tmp1354;
  unsigned int llvm_cbe_c_2712472_12;
  unsigned int llvm_cbe_c_2712472_12__PHI_TEMPORARY;
  unsigned char llvm_cbe_x1364_10;
  unsigned char llvm_cbe_x1364_10__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1382;
  unsigned char llvm_cbe_tmp1383;
  unsigned int llvm_cbe_indvar_next26652;
  unsigned char *llvm_cbe_tmp519812479;
  unsigned char *llvm_cbe_tmp1407;
  unsigned char llvm_cbe_tmp1408;
  unsigned int llvm_cbe_tmp14081409;
  unsigned char llvm_cbe_tmp141812502;
  unsigned int llvm_cbe_inescq_9;
  unsigned int llvm_cbe_inescq_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_18;
  unsigned int llvm_cbe_c_18__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1416;
  unsigned char llvm_cbe_tmp1418;
  unsigned int llvm_cbe_inescq_912486_0;
  unsigned int llvm_cbe_inescq_912486_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_1812491_0;
  unsigned int llvm_cbe_c_1812491_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1424;
  unsigned char *llvm_cbe_tmp1425;
  unsigned char llvm_cbe_tmp1426;
  unsigned int llvm_cbe_inescq_912486_1;
  unsigned int llvm_cbe_inescq_912486_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_1812491_1;
  unsigned int llvm_cbe_c_1812491_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1431;
  unsigned char llvm_cbe_tmp1439;
  unsigned char *llvm_cbe_tmp1447_pn;
  unsigned char *llvm_cbe_storemerge5533;
  unsigned char llvm_cbe_tmp1451;
  unsigned char llvm_cbe_tmp1459;
  unsigned char *llvm_cbe_tmp1466;
  unsigned char llvm_cbe_tmp1468;
  unsigned char llvm_cbe_tmp1475;
  unsigned char *llvm_cbe_tmp1485;
  unsigned char llvm_cbe_tmp1486;
  unsigned char llvm_cbe_tmp1494;
  unsigned int llvm_cbe_inescq_10;
  unsigned int llvm_cbe_inescq_10__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1499;
  unsigned char llvm_cbe_tmp1500;
  unsigned int llvm_cbe_tmp15211522;
  unsigned int llvm_cbe_tmp1537;
  unsigned int llvm_cbe_tmp1540;
  unsigned int llvm_cbe_tmp1542;
  unsigned int llvm_cbe_d_10;
  unsigned int llvm_cbe_d_10__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp519812520;
  unsigned int llvm_cbe_d_1012504_1;
  unsigned int llvm_cbe_d_1012504_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp1597;
  unsigned int llvm_cbe_indvar26648;
  unsigned int llvm_cbe_indvar26648__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_2812521_12;
  unsigned char *llvm_cbe_tmp1609;
  unsigned char llvm_cbe_tmp1610;
  unsigned char *llvm_cbe_tmp1624;
  unsigned char llvm_cbe_tmp1627;
  unsigned int llvm_cbe_tmp16271628;
  unsigned char *llvm_cbe_tmp1633;
  unsigned char llvm_cbe_tmp1634;
  unsigned int llvm_cbe_indvar26645;
  unsigned int llvm_cbe_indvar26645__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_2812521_12_us;
  unsigned char *llvm_cbe_tmp1609_us;
  unsigned char llvm_cbe_tmp1610_us;
  unsigned int llvm_cbe_inescq_11;
  unsigned int llvm_cbe_inescq_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1658;
  unsigned char llvm_cbe_tmp1659;
  unsigned char *llvm_cbe_tmp1673;
  unsigned char llvm_cbe_tmp1676;
  unsigned int llvm_cbe_tmp16761677;
  unsigned char *llvm_cbe_tmp1682;
  unsigned char llvm_cbe_tmp1683;
  unsigned int llvm_cbe_c_29;
  unsigned int llvm_cbe_c_29__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp1692;
  unsigned int llvm_cbe_class_lastchar_6;
  unsigned int llvm_cbe_class_lastchar_6__PHI_TEMPORARY;
  unsigned int llvm_cbe_class_charcount_6;
  unsigned int llvm_cbe_class_charcount_6__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_3;
  unsigned int llvm_cbe_inescq_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1695;
  unsigned char *llvm_cbe_tmp1696;
  unsigned char llvm_cbe_tmp1698;
  unsigned int llvm_cbe_tmp16981699;
  unsigned int llvm_cbe_class_lastchar_8;
  unsigned int llvm_cbe_class_lastchar_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_class_charcount_8;
  unsigned int llvm_cbe_class_charcount_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_5;
  unsigned int llvm_cbe_inescq_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_12;
  unsigned int llvm_cbe_c_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp519812532;
  unsigned int llvm_cbe_firstbyte102_5;
  unsigned char *llvm_cbe_tmp1751;
  unsigned char *llvm_cbe_tmp519612533;
  unsigned char *llvm_cbe_code105_2712620;
  unsigned int llvm_cbe_firstbyte102_6;
  unsigned char *llvm_cbe_tmp1772;
  unsigned int llvm_cbe_c_3112623_4;
  unsigned int llvm_cbe_c_3112623_4__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp1781;
  unsigned int llvm_cbe_tmp1772_sum;
  unsigned char *llvm_cbe_tmp1797;
  unsigned char *ltmp_7_1;
  unsigned char *llvm_cbe_code105_11;
  unsigned char *llvm_cbe_code105_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1802;
  unsigned char *llvm_cbe_tmp519612629;
  unsigned char *llvm_cbe_tmp1810;
  unsigned char *llvm_cbe_p1813_626504;
  unsigned char llvm_cbe_tmp182826506;
  unsigned char llvm_cbe_tmp183126509;
  unsigned char *llvm_cbe_p1813_626505;
  unsigned char *llvm_cbe_p1813_626505__PHI_TEMPORARY;
  unsigned int llvm_cbe_min_626502;
  unsigned int llvm_cbe_min_626502__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp1820;
  unsigned int llvm_cbe_tmp1823;
  unsigned char *llvm_cbe_p1813_6;
  unsigned char llvm_cbe_tmp1828;
  unsigned char llvm_cbe_tmp1831;
  unsigned char *llvm_cbe_p1813_6_lcssa;
  unsigned char *llvm_cbe_p1813_6_lcssa__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp1810_pn_lcssa;
  unsigned char *llvm_cbe_tmp1810_pn_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_min_6_lcssa;
  unsigned int llvm_cbe_min_6_lcssa__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp1848;
  unsigned char *llvm_cbe_tmp1856;
  unsigned char llvm_cbe_tmp1858;
  unsigned char llvm_cbe_tmp187826498;
  unsigned int llvm_cbe_p1813_926494_rec;
  unsigned int llvm_cbe_p1813_926494_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_max_826493;
  unsigned int llvm_cbe_max_826493__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp1867;
  unsigned int llvm_cbe_tmp1870;
  unsigned char *llvm_cbe_tmp1872;
  unsigned char llvm_cbe_tmp1875;
  unsigned char llvm_cbe_tmp1878;
  unsigned char *llvm_cbe_p1813_9_lcssa;
  unsigned char *llvm_cbe_p1813_9_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_max_8_lcssa;
  unsigned int llvm_cbe_max_8_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_max_3;
  unsigned int llvm_cbe_repeat_max_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_min_3;
  unsigned int llvm_cbe_repeat_min_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp93_0;
  unsigned char *llvm_cbe_tmp93_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp1915;
  unsigned int llvm_cbe_repeat_max_4;
  unsigned int llvm_cbe_repeat_max_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_min_4;
  unsigned int llvm_cbe_repeat_min_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp519812674;
  unsigned int llvm_cbe_repeat_max_9;
  unsigned int llvm_cbe_repeat_max_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_min_9;
  unsigned int llvm_cbe_repeat_min_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_1;
  unsigned int llvm_cbe_firstbyte102_7;
  unsigned int llvm_cbe_iftmp_236_0;
  unsigned char *llvm_cbe_tmp1949;
  unsigned char *llvm_cbe_tmp1950;
  unsigned char llvm_cbe_tmp1951;
  unsigned char llvm_cbe_tmp197412733;
  unsigned int llvm_cbe_repeat_type_0;
  unsigned int llvm_cbe_repeat_type_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp1974;
  unsigned int llvm_cbe_possessive_quantifier_512675_0;
  unsigned int llvm_cbe_possessive_quantifier_512675_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_type_012701_0;
  unsigned int llvm_cbe_repeat_type_012701_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp1982;
  unsigned int llvm_cbe_tmp19821983;
  unsigned int llvm_cbe_tmp1994;
  unsigned int llvm_cbe_tmp1995;
  unsigned int llvm_cbe_reqbyte103_6;
  unsigned int llvm_cbe_reqbyte103_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp2007;
  unsigned char llvm_cbe_tmp2010;
  unsigned int llvm_cbe_tmp2017;
  unsigned char llvm_cbe_tmp209512753;
  unsigned int llvm_cbe_possessive_quantifier_512675_1;
  unsigned int llvm_cbe_possessive_quantifier_512675_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_type_012701_1;
  unsigned int llvm_cbe_repeat_type_012701_1__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp2027;
  unsigned char llvm_cbe_tmp2034;
  unsigned int llvm_cbe_tmp20342035;
  unsigned char *llvm_cbe_tmp2046;
  unsigned int llvm_cbe_tmp2052;
  unsigned int llvm_cbe_tmp20682069;
  unsigned char *llvm_cbe_tmp2080;
  unsigned int llvm_cbe_tmp2086;
  unsigned int llvm_cbe_possessive_quantifier_6;
  unsigned int llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_32;
  unsigned int llvm_cbe_c_32__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_7;
  unsigned int llvm_cbe_reqbyte103_7__PHI_TEMPORARY;
  unsigned int llvm_cbe_op_type_5;
  unsigned int llvm_cbe_op_type_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_type_6;
  unsigned int llvm_cbe_repeat_type_6__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp2095;
  unsigned int llvm_cbe_possessive_quantifier_612736_0;
  unsigned int llvm_cbe_possessive_quantifier_612736_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_3212739_0;
  unsigned int llvm_cbe_c_3212739_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_712742_0;
  unsigned int llvm_cbe_reqbyte103_712742_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_op_type_512745_0;
  unsigned int llvm_cbe_op_type_512745_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_type_612750_0;
  unsigned int llvm_cbe_repeat_type_612750_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp2103;
  unsigned int llvm_cbe_tmp21032104;
  unsigned char llvm_cbe_tmp2107;
  unsigned int llvm_cbe_tmp21072108;
  unsigned int llvm_cbe_possessive_quantifier_612736_1;
  unsigned int llvm_cbe_possessive_quantifier_612736_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_3212739_1;
  unsigned int llvm_cbe_c_3212739_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_712742_1;
  unsigned int llvm_cbe_reqbyte103_712742_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_op_type_512745_1;
  unsigned int llvm_cbe_op_type_512745_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_type_612750_1;
  unsigned int llvm_cbe_repeat_type_612750_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_prop_value_0;
  unsigned int llvm_cbe_prop_value_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_prop_type_0;
  unsigned int llvm_cbe_prop_type_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2130;
  unsigned char llvm_cbe_tmp21412142;
  unsigned char *llvm_cbe_tmp2146;
  unsigned char *llvm_cbe_tmp2158;
  unsigned char *llvm_cbe_tmp2177;
  unsigned char *llvm_cbe_tmp2196;
  unsigned char *llvm_cbe_tmp2224;
  unsigned char *llvm_cbe_tmp2244;
  unsigned char *llvm_cbe_tmp2254;
  unsigned char llvm_cbe_tmp2273;
  unsigned char *llvm_cbe_tmp227612756;
  unsigned char *llvm_cbe_tmp2276;
  unsigned char *llvm_cbe_tmp2288;
  unsigned char *llvm_cbe_tmp2303;
  unsigned char *llvm_cbe_code105_16;
  unsigned char *llvm_cbe_code105_16__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2307;
  unsigned char llvm_cbe_tmp23132314;
  unsigned char *llvm_cbe_tmp2318;
  unsigned char *llvm_cbe_tmp2337;
  unsigned int llvm_cbe_repeat_max_11;
  unsigned int llvm_cbe_repeat_max_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_14;
  unsigned char *llvm_cbe_code105_14__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp2347;
  unsigned char *llvm_cbe_tmp2396;
  unsigned char *llvm_cbe_tmp2415;
  unsigned char llvm_cbe_tmp24362437;
  unsigned char *llvm_cbe_tmp2434;
  unsigned int llvm_cbe_repeat_max_14;
  unsigned char *llvm_cbe_tmp2471;
  unsigned int llvm_cbe_tmp24842485;
  unsigned int llvm_cbe_tmp2488;
  unsigned char llvm_cbe_tmp2497;
  unsigned char *llvm_cbe_tmp519813229;
  unsigned char *llvm_cbe_tmp519813230;
  unsigned int llvm_cbe_ket_6_rec;
  unsigned int llvm_cbe_ket_6_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp2521;
  unsigned char llvm_cbe_tmp2526;
  unsigned int llvm_cbe_tmp2530_rec;
  unsigned char *llvm_cbe_tmp2530;
  unsigned char llvm_cbe_tmp2532;
  unsigned int llvm_cbe_tmp2541;
  unsigned int llvm_cbe_ketoffset_7;
  unsigned int llvm_cbe_ketoffset_7__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp2567;
  unsigned char *ltmp_8_1;
  unsigned char *llvm_cbe_tmp2570;
  unsigned char *llvm_cbe_tmp2586;
  unsigned char *ltmp_9_1;
  unsigned char *llvm_cbe_tmp2589;
  unsigned char *llvm_cbe_tmp2598;
  unsigned char *llvm_cbe_bralink_7;
  unsigned char *llvm_cbe_bralink_7__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_8;
  unsigned char *llvm_cbe_previous_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_17;
  unsigned char *llvm_cbe_code105_17__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2627;
  unsigned int llvm_cbe_tmp2640;
  unsigned int llvm_cbe_tmp2643;
  unsigned int llvm_cbe_reqbyte103_10;
  unsigned char llvm_cbe_tmp27022703;
  unsigned int llvm_cbe_indvar26622;
  unsigned int llvm_cbe_indvar26622__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_113294_0;
  unsigned char *llvm_cbe_save_hwm_113294_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_code105_1913296_0_rec;
  unsigned char *llvm_cbe_tmp2665;
  unsigned char *ltmp_10_1;
  unsigned int llvm_cbe_indvar26619;
  unsigned int llvm_cbe_indvar26619__PHI_TEMPORARY;
  unsigned int llvm_cbe_hc_813305_0_rec;
  unsigned char *llvm_cbe_tmp2673;
  unsigned char llvm_cbe_tmp2676;
  unsigned char *llvm_cbe_tmp2680;
  unsigned char llvm_cbe_tmp2681;
  unsigned char *llvm_cbe_tmp2691;
  unsigned char llvm_cbe_tmp2700;
  unsigned char *llvm_cbe_tmp2709;
  unsigned int llvm_cbe_indvar_next26623;
  unsigned char *llvm_cbe_tmp2725;
  unsigned char *llvm_cbe_save_hwm_2;
  unsigned char *llvm_cbe_save_hwm_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_20;
  unsigned char *llvm_cbe_code105_20__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_11;
  unsigned int llvm_cbe_reqbyte103_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_213284_0;
  unsigned char *llvm_cbe_save_hwm_213284_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_2013285_0;
  unsigned char *llvm_cbe_code105_2013285_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_1113286_0;
  unsigned int llvm_cbe_reqbyte103_1113286_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2744;
  unsigned int llvm_cbe_repeat_max_18;
  unsigned int llvm_cbe_repeat_max_18__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_213284_1;
  unsigned char *llvm_cbe_save_hwm_213284_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_2013285_1;
  unsigned char *llvm_cbe_code105_2013285_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_1113286_1;
  unsigned int llvm_cbe_reqbyte103_1113286_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_max_16;
  unsigned int llvm_cbe_repeat_max_16__PHI_TEMPORARY;
  unsigned char *llvm_cbe_bralink_813241_0;
  unsigned char *llvm_cbe_bralink_813241_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_313252_0;
  unsigned char *llvm_cbe_save_hwm_313252_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_913258_0;
  unsigned char *llvm_cbe_previous_913258_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_1813259_0;
  unsigned char *llvm_cbe_code105_1813259_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_1213269_0;
  unsigned int llvm_cbe_reqbyte103_1213269_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2765;
  unsigned int llvm_cbe_tmp2766;
  unsigned char llvm_cbe_tmp2784;
  unsigned char llvm_cbe_tmp28702871;
  unsigned char *llvm_cbe_tmp2781;
  unsigned char *llvm_cbe_tmp2787;
  unsigned char *llvm_cbe_tmp2796;
  unsigned int llvm_cbe_tmp2806;
  unsigned int llvm_cbe_iftmp_312_0;
  unsigned int llvm_cbe_iftmp_312_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp2822;
  unsigned char *ltmp_11_1;
  unsigned char *ltmp_12_1;
  unsigned char *llvm_cbe_bralink_913313_0_ph;
  unsigned char *llvm_cbe_bralink_913313_0_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_2313314_0_ph;
  unsigned char *llvm_cbe_code105_2313314_0_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2844;
  unsigned char llvm_cbe_tmp2872;
  unsigned int llvm_cbe_indvar;
  unsigned int llvm_cbe_indvar__PHI_TEMPORARY;
  unsigned int llvm_cbe_hc2777_713319_0_rec;
  unsigned char *llvm_cbe_tmp2832;
  unsigned char llvm_cbe_tmp2835;
  unsigned char *llvm_cbe_tmp2839;
  unsigned char llvm_cbe_tmp2840;
  unsigned char *llvm_cbe_tmp2859;
  unsigned char llvm_cbe_tmp2868;
  unsigned char *llvm_cbe_tmp2886;
  unsigned char *llvm_cbe_bralink_913313_1;
  unsigned char *llvm_cbe_bralink_913313_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_2313314_1;
  unsigned char *llvm_cbe_code105_2313314_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp2902;
  unsigned int llvm_cbe_indvar_next26614;
  unsigned int llvm_cbe_indvar26613;
  unsigned int llvm_cbe_indvar26613__PHI_TEMPORARY;
  unsigned char *llvm_cbe_bralink_10;
  unsigned char *llvm_cbe_bralink_10__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_8;
  unsigned char *llvm_cbe_save_hwm_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_21;
  unsigned char *llvm_cbe_code105_21__PHI_TEMPORARY;
  unsigned int llvm_cbe_i_9;
  unsigned char *llvm_cbe_bralink_11_ph;
  unsigned char *llvm_cbe_bralink_11_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_9_ph;
  unsigned char *llvm_cbe_save_hwm_9_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_22_ph;
  unsigned char *llvm_cbe_code105_22_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_indvar26616;
  unsigned int llvm_cbe_indvar26616__PHI_TEMPORARY;
  unsigned char *llvm_cbe_bralink_1126488;
  unsigned char *llvm_cbe_bralink_1126488__PHI_TEMPORARY;
  unsigned int llvm_cbe_code105_2226489_rec;
  unsigned char *llvm_cbe_code105_2226489;
  unsigned int llvm_cbe_tmp2917;
  unsigned int llvm_cbe_tmp2918;
  unsigned char *llvm_cbe_tmp2924;
  unsigned char llvm_cbe_tmp2925;
  unsigned char *llvm_cbe_tmp2929;
  unsigned char llvm_cbe_tmp2930;
  unsigned int llvm_cbe_tmp2932;
  unsigned char *llvm_cbe_tmp2941;
  unsigned char *llvm_cbe_iftmp_320_0;
  unsigned char *llvm_cbe_iftmp_320_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp29492950;
  unsigned char llvm_cbe_tmp29532954;
  unsigned int llvm_cbe_indvar_next26617;
  unsigned int llvm_cbe_repeat_max_17;
  unsigned int llvm_cbe_repeat_max_17__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_313252_1;
  unsigned char *llvm_cbe_save_hwm_313252_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_1813259_1;
  unsigned char *llvm_cbe_code105_1813259_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_1213269_1;
  unsigned int llvm_cbe_reqbyte103_1213269_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp2980;
  unsigned char llvm_cbe_tmp2983;
  unsigned char llvm_cbe_tmp2988;
  unsigned int llvm_cbe_tmp2980_sum;
  unsigned char *llvm_cbe_tmp2993;
  unsigned char llvm_cbe_tmp3004;
  unsigned int llvm_cbe_scode_5_rec;
  unsigned int llvm_cbe_scode_5_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp2993_sum26701;
  unsigned int llvm_cbe_tmp3014;
  unsigned char llvm_cbe_tmp3020;
  unsigned char llvm_cbe_tmp3026;
  unsigned char llvm_cbe_tmp3031;
  unsigned int llvm_cbe_tmp3035_rec;
  unsigned char llvm_cbe_tmp3037;
  unsigned char *llvm_cbe_tmp519813331;
  unsigned char *llvm_cbe_tmp2959;
  unsigned int llvm_cbe_repeat_max_15;
  unsigned int llvm_cbe_repeat_max_15__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_4;
  unsigned char *llvm_cbe_save_hwm_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_12;
  unsigned char *llvm_cbe_code105_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_8;
  unsigned int llvm_cbe_reqbyte103_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_repeat_max_12;
  unsigned int llvm_cbe_repeat_max_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_413043_0;
  unsigned char *llvm_cbe_save_hwm_413043_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_1213058_0;
  unsigned char *llvm_cbe_code105_1213058_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_813077_0;
  unsigned int llvm_cbe_reqbyte103_813077_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3058;
  unsigned char llvm_cbe_tmp3059;
  unsigned char llvm_cbe_tmp3084;
  unsigned char *llvm_cbe_tmp3090;
  unsigned int llvm_cbe_tmp3092;
  unsigned char llvm_cbe_tmp3099;
  unsigned char *ltmp_13_1;
  unsigned int llvm_cbe_tmp3134;
  unsigned char *llvm_cbe_tmp3135;
  unsigned char llvm_cbe_tmp31413142;
  unsigned char llvm_cbe_tmp31453146;
  unsigned char *llvm_cbe_tmp3151;
  unsigned char *llvm_cbe_tmp3152;
  unsigned char *llvm_cbe_tmp3157;
  unsigned int llvm_cbe_repeat_max_13;
  unsigned int llvm_cbe_repeat_max_13__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_5;
  unsigned char *llvm_cbe_save_hwm_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_13;
  unsigned char *llvm_cbe_code105_13__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_9;
  unsigned int llvm_cbe_reqbyte103_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp3167;
  unsigned char *llvm_cbe_tmp519613332;
  unsigned char *llvm_cbe_tmp3176;
  unsigned char *llvm_cbe_tmp3177;
  unsigned char *llvm_cbe_tmp3178;
  unsigned char llvm_cbe_tmp3180;
  unsigned char *llvm_cbe_tmp3187;
  unsigned char llvm_cbe_tmp3189;
  unsigned char *llvm_cbe_tmp3195_pn;
  unsigned char *llvm_cbe_tmp3195_pn__PHI_TEMPORARY;
  unsigned char *llvm_cbe_storemerge;
  unsigned char llvm_cbe_tmp3199;
  unsigned int llvm_cbe_reset_bracount132_8;
  unsigned int llvm_cbe_reset_bracount132_8__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp3227;
  unsigned char llvm_cbe_tmp3234;
  unsigned char *llvm_cbe_tmp3258;
  unsigned char *llvm_cbe_tmp3259;
  unsigned char *llvm_cbe_tmp3260;
  unsigned char llvm_cbe_tmp3261;
  unsigned char *llvm_cbe_tmp3268;
  unsigned char llvm_cbe_tmp3269;
  unsigned char llvm_cbe_tmp3318;
  unsigned int llvm_cbe_tmp33183319;
  unsigned int llvm_cbe_terminator_0;
  unsigned int llvm_cbe_terminator_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_refsign_6;
  unsigned int llvm_cbe_refsign_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3326;
  unsigned char *llvm_cbe_tmp3327;
  unsigned char *llvm_cbe_tmp3328;
  unsigned char llvm_cbe_tmp3329;
  unsigned char llvm_cbe_tmp3332;
  unsigned char llvm_cbe_tmp338226532;
  unsigned char llvm_cbe_tmp338526535;
  unsigned int llvm_cbe_recno_726527;
  unsigned int llvm_cbe_recno_726527__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3353;
  unsigned char llvm_cbe_tmp3354;
  unsigned int llvm_cbe_tmp33543355;
  unsigned char llvm_cbe_tmp3357;
  unsigned int llvm_cbe_tmp3370;
  unsigned int llvm_cbe_recno_6;
  unsigned int llvm_cbe_recno_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3375;
  unsigned char *llvm_cbe_tmp3376;
  unsigned char llvm_cbe_tmp3382;
  unsigned char llvm_cbe_tmp3385;
  unsigned int llvm_cbe_recno_7_lcssa;
  unsigned int llvm_cbe_recno_7_lcssa__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3392;
  unsigned int llvm_cbe_tmp3396;
  unsigned char llvm_cbe_tmp3404;
  unsigned char *llvm_cbe_tmp3412;
  unsigned char llvm_cbe_tmp3413;
  unsigned char *llvm_cbe_tmp3416;
  unsigned char *llvm_cbe_tmp3419;
  unsigned char *llvm_cbe_tmp3420;
  unsigned int llvm_cbe_tmp3448;
  unsigned int llvm_cbe_tmp3451;
  unsigned int llvm_cbe_tmp3464;
  unsigned int llvm_cbe_recno_8;
  unsigned int llvm_cbe_recno_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3479;
  unsigned int llvm_cbe_tmp351513357;
  unsigned int llvm_cbe_i3185_613340_0;
  unsigned int llvm_cbe_i3185_613340_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_slot_613344_0_rec;
  unsigned int llvm_cbe_slot_613344_0_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_613344_0;
  unsigned int llvm_cbe_tmp3497;
  unsigned int llvm_cbe_tmp3507;
  unsigned int llvm_cbe_tmp3509_rec;
  unsigned char *llvm_cbe_tmp3509;
  unsigned int llvm_cbe_tmp3511;
  unsigned int llvm_cbe_i3185_613340_1;
  unsigned int llvm_cbe_i3185_613340_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_613344_1;
  unsigned char *llvm_cbe_slot_613344_1__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp3531;
  unsigned char llvm_cbe_tmp3536;
  unsigned int llvm_cbe_tmp3538;
  unsigned int llvm_cbe_tmp3555;
  unsigned int llvm_cbe_tmp3559;
  unsigned char *llvm_cbe_tmp519813360;
  unsigned char llvm_cbe_tmp3584;
  unsigned char llvm_cbe_tmp3593;
  unsigned int llvm_cbe_tmp35933594;
  unsigned char llvm_cbe_tmp3596;
  unsigned char *llvm_cbe_tmp519813361;
  unsigned int llvm_cbe_tmp3614;
  unsigned int llvm_cbe_indvar26632;
  unsigned int llvm_cbe_indvar26632__PHI_TEMPORARY;
  unsigned int llvm_cbe_recno_9;
  unsigned int llvm_cbe_recno_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_i3185_7;
  unsigned int llvm_cbe_recno_10;
  unsigned int llvm_cbe_tmp3652;
  unsigned char *llvm_cbe_tmp519813362;
  unsigned char *llvm_cbe_tmp3700;
  unsigned char llvm_cbe_tmp3701;
  unsigned int llvm_cbe_tmp37013702;
  unsigned char *llvm_cbe_tmp3712;
  unsigned char llvm_cbe_tmp3718;
  unsigned char *llvm_cbe_tmp3737;
  unsigned char *llvm_cbe_tmp374726477;
  unsigned char *llvm_cbe_tmp374826478;
  unsigned char llvm_cbe_tmp375026479;
  unsigned char llvm_cbe_tmp375326482;
  unsigned int llvm_cbe_n_526476;
  unsigned int llvm_cbe_n_526476__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3741;
  unsigned char llvm_cbe_tmp3742;
  unsigned int llvm_cbe_tmp3745;
  unsigned char *llvm_cbe_tmp3748;
  unsigned char llvm_cbe_tmp3750;
  unsigned char llvm_cbe_tmp3753;
  unsigned int llvm_cbe_n_5_lcssa;
  unsigned int llvm_cbe_n_5_lcssa__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3760;
  unsigned char llvm_cbe_tmp3761;
  unsigned char *llvm_cbe_tmp3780;
  unsigned char *llvm_cbe_tmp3784;
  unsigned char *llvm_cbe_tmp3792;
  unsigned char *llvm_cbe_tmp3797;
  unsigned char *llvm_cbe_tmp3810;
  unsigned char *llvm_cbe_tmp519613366;
  unsigned char *llvm_cbe_tmp3813;
  unsigned char llvm_cbe_tmp3815;
  unsigned int llvm_cbe_tmp38293830;
  unsigned char *llvm_cbe_tmp402913426;
  unsigned char *llvm_cbe_tmp3841;
  unsigned char llvm_cbe_tmp3842;
  unsigned int llvm_cbe_iftmp_415_0;
  unsigned char *llvm_cbe_tmp3851;
  unsigned char *llvm_cbe_tmp385926515;
  unsigned char llvm_cbe_tmp386126517;
  unsigned char llvm_cbe_tmp386426520;
  unsigned char *llvm_cbe_tmp385426524;
  unsigned char *llvm_cbe_tmp385426524__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp3855;
  unsigned char llvm_cbe_tmp3861;
  unsigned char llvm_cbe_tmp3864;
  unsigned char *llvm_cbe_tmp3854_lcssa;
  unsigned char *llvm_cbe_tmp3854_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp3875;
  unsigned char *llvm_cbe_tmp3881;
  unsigned char llvm_cbe_tmp3882;
  unsigned int llvm_cbe_tmp3893;
  unsigned int llvm_cbe_tmp3901;
  unsigned int llvm_cbe_tmp3904;
  unsigned char *llvm_cbe_tmp3924;
  unsigned int llvm_cbe_tmp398813449;
  unsigned int llvm_cbe_tmp3937;
  unsigned int llvm_cbe_i3185_1013432_0_us;
  unsigned int llvm_cbe_i3185_1013432_0_us__PHI_TEMPORARY;
  unsigned int llvm_cbe_slot_913437_0_us_rec;
  unsigned int llvm_cbe_slot_913437_0_us_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp3930_us;
  unsigned int llvm_cbe_tmp3980_us;
  unsigned int llvm_cbe_tmp3982_us_rec;
  unsigned int llvm_cbe_tmp3984_us;
  unsigned int llvm_cbe_crc_10_us;
  unsigned int llvm_cbe_crc_10_us__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp3940_us;
  unsigned int llvm_cbe_i3185_1013432_0;
  unsigned int llvm_cbe_i3185_1013432_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_slot_913437_0_rec;
  unsigned int llvm_cbe_slot_913437_0_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp3930;
  unsigned char llvm_cbe_tmp3940;
  unsigned char *llvm_cbe_slot_913437_0_us_le62;
  unsigned char *llvm_cbe_tmp519813452;
  unsigned int llvm_cbe_crc_10;
  unsigned int llvm_cbe_crc_10__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_0_us_le;
  unsigned char *llvm_cbe_slot_913437_0_le;
  unsigned char *llvm_cbe_slot_913437_0_us61_14;
  unsigned char *llvm_cbe_slot_913437_0_us61_14__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_14;
  unsigned char *llvm_cbe_slot_913437_059_14__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_0_lcssa14144_us_lcssa;
  unsigned char *llvm_cbe_slot_913437_0_lcssa14144_us_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_i3185_1013432_0_lcssa14142_us_lcssa;
  unsigned int llvm_cbe_i3185_1013432_0_lcssa14142_us_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp3969;
  unsigned char *ltmp_14_1;
  unsigned int llvm_cbe_tmp3980;
  unsigned int llvm_cbe_tmp3982_rec;
  unsigned int llvm_cbe_tmp3984;
  unsigned char *llvm_cbe_slot_913437_0_us;
  unsigned char *llvm_cbe_tmp3982_us;
  unsigned char *llvm_cbe_slot_913437_0;
  unsigned char *llvm_cbe_tmp3982;
  unsigned char *llvm_cbe_slot_913437_0_us61_0;
  unsigned char *llvm_cbe_slot_913437_0_us61_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_0;
  unsigned char *llvm_cbe_slot_913437_059_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_1;
  unsigned char *llvm_cbe_slot_913437_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp3996;
  unsigned int llvm_cbe_tmp4004;
  unsigned char *ltmp_15_1;
  unsigned char *llvm_cbe_slot_913437_0_us61_1;
  unsigned char *llvm_cbe_slot_913437_0_us61_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_1;
  unsigned char *llvm_cbe_slot_913437_059_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4019;
  unsigned int llvm_cbe_tmp4023;
  unsigned int llvm_cbe_terminator_7;
  unsigned int llvm_cbe_terminator_7__PHI_TEMPORARY;
  unsigned int llvm_cbe_is_recurse_0;
  unsigned int llvm_cbe_is_recurse_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_10;
  unsigned char *llvm_cbe_save_hwm_10__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_4;
  unsigned int llvm_cbe_zerofirstbyte_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_4;
  unsigned int llvm_cbe_zeroreqbyte_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_13;
  unsigned int llvm_cbe_firstbyte102_13__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4028;
  unsigned char *llvm_cbe_tmp4029;
  unsigned int llvm_cbe_terminator_713369_0_ph;
  unsigned int llvm_cbe_terminator_713369_0_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_is_recurse_013390_0_ph;
  unsigned int llvm_cbe_is_recurse_013390_0_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_1013396_0_ph;
  unsigned char *llvm_cbe_save_hwm_1013396_0_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_413398_0_ph;
  unsigned int llvm_cbe_zerofirstbyte_413398_0_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_413400_0_ph;
  unsigned int llvm_cbe_zeroreqbyte_413400_0_ph__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_1313402_0_ph;
  unsigned int llvm_cbe_firstbyte102_1313402_0_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp402913416_0_ph;
  unsigned char *llvm_cbe_tmp402913416_0_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_storemerge5542_ph;
  unsigned char *llvm_cbe_storemerge5542_ph__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp403726541;
  unsigned char llvm_cbe_tmp403926543;
  unsigned char llvm_cbe_tmp404226546;
  unsigned char *llvm_cbe_tmp403226550;
  unsigned char *llvm_cbe_tmp403226550__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4033;
  unsigned char llvm_cbe_tmp4039;
  unsigned char llvm_cbe_tmp4042;
  unsigned char *llvm_cbe_tmp4032_lcssa;
  unsigned char *llvm_cbe_tmp4032_lcssa__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp4053;
  unsigned char *llvm_cbe_tmp4059;
  unsigned char llvm_cbe_tmp4060;
  unsigned char *llvm_cbe_tmp4079;
  unsigned int llvm_cbe_tmp411513469;
  unsigned int llvm_cbe_i3185_1413455_0;
  unsigned int llvm_cbe_i3185_1413455_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_slot_1313461_0_rec;
  unsigned int llvm_cbe_slot_1313461_0_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_1313461_0;
  unsigned int llvm_cbe_tmp4097;
  unsigned int llvm_cbe_tmp4107;
  unsigned int llvm_cbe_tmp4109_rec;
  unsigned char *llvm_cbe_tmp4109;
  unsigned int llvm_cbe_tmp4111;
  unsigned int llvm_cbe_i3185_1413455_1;
  unsigned int llvm_cbe_i3185_1413455_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_1313461_1;
  unsigned char *llvm_cbe_slot_1313461_1__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp4131;
  unsigned char llvm_cbe_tmp4136;
  unsigned int llvm_cbe_tmp4138;
  unsigned int llvm_cbe_tmp4145;
  unsigned char *llvm_cbe_tmp4146;
  unsigned int llvm_cbe_tmp4149;
  unsigned char *llvm_cbe_tmp519813472;
  unsigned int llvm_cbe_recno_11;
  unsigned int llvm_cbe_recno_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4178;
  unsigned char llvm_cbe_tmp4179;
  unsigned char *llvm_cbe_tmp4195;
  unsigned char llvm_cbe_tmp4196;
  unsigned char llvm_cbe_tmp4199;
  unsigned char *llvm_cbe_tmp422226553;
  unsigned char llvm_cbe_tmp422326554;
  unsigned char llvm_cbe_tmp422626557;
  unsigned int llvm_cbe_recno_1426552;
  unsigned int llvm_cbe_recno_1426552__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4215;
  unsigned char llvm_cbe_tmp4216;
  unsigned int llvm_cbe_tmp4219;
  unsigned char *llvm_cbe_tmp4220;
  unsigned char llvm_cbe_tmp4223;
  unsigned char llvm_cbe_tmp4226;
  unsigned int llvm_cbe_recno_14_lcssa;
  unsigned int llvm_cbe_recno_14_lcssa__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4233;
  unsigned char llvm_cbe_tmp4234;
  unsigned int llvm_cbe_tmp4255;
  unsigned int llvm_cbe_tmp4258;
  unsigned int llvm_cbe_tmp4281;
  unsigned int llvm_cbe_tmp4283;
  unsigned int llvm_cbe_recno_12;
  unsigned int llvm_cbe_recno_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_11;
  unsigned char *llvm_cbe_save_hwm_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_8;
  unsigned int llvm_cbe_zerofirstbyte_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_8;
  unsigned int llvm_cbe_zeroreqbyte_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_15;
  unsigned int llvm_cbe_firstbyte102_15__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4289;
  unsigned char *llvm_cbe_tmp4303;
  unsigned char *llvm_cbe_tmp4306;
  unsigned char *llvm_cbe_called_5;
  unsigned char *llvm_cbe_called_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp4318;
  unsigned char *llvm_cbe_tmp4319;
  unsigned int llvm_cbe_tmp4321;
  unsigned char *llvm_cbe_tmp519813477;
  unsigned char *llvm_cbe_tmp4330;
  unsigned char *llvm_cbe_tmp4332;
  unsigned char *llvm_cbe_tmp4335;
  unsigned char *llvm_cbe_tmp4338;
  unsigned char *llvm_cbe_tmp4350;
  unsigned char *llvm_cbe_tmp4358;
  unsigned char *llvm_cbe_tmp4366;
  unsigned char llvm_cbe_tmp4373;
  unsigned char llvm_cbe_tmp4378;
  unsigned int llvm_cbe_tmp4399;
  struct l_struct_2E_branch_chain *llvm_cbe_tmp4408;
  struct l_struct_2E_branch_chain *llvm_cbe_bcptr4386_6;
  struct l_struct_2E_branch_chain *llvm_cbe_bcptr4386_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4418;
  unsigned char *llvm_cbe_tmp519813478;
  unsigned char *llvm_cbe_called_6;
  unsigned char *llvm_cbe_called_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4448;
  unsigned char *llvm_cbe_tmp4460;
  unsigned char *llvm_cbe_tmp4475;
  unsigned char *llvm_cbe_tmp519613479;
  unsigned int llvm_cbe_tmp4492;
  unsigned int llvm_cbe_tmp4497;
  unsigned int llvm_cbe_tmp4503;
  unsigned int llvm_cbe_tmp4508;
  unsigned int llvm_cbe_tmp4513;
  unsigned int llvm_cbe_tmp4518;
  unsigned int llvm_cbe_tmp4523;
  unsigned int llvm_cbe_tmp4528;
  unsigned int *llvm_cbe_optset_6;
  unsigned int *llvm_cbe_optset_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4537;
  unsigned char llvm_cbe_tmp4538;
  unsigned int llvm_cbe_tmp4551;
  unsigned int llvm_cbe_tmp4554;
  unsigned int llvm_cbe_tmp4555;
  unsigned char *llvm_cbe_tmp4564;
  unsigned int llvm_cbe_tmp4580;
  unsigned char *llvm_cbe_tmp4606;
  unsigned char *llvm_cbe_code105_24;
  unsigned char *llvm_cbe_code105_24__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp4613;
  unsigned int llvm_cbe_tmp4615;
  unsigned char *llvm_cbe_tmp519613486;
  unsigned char *llvm_cbe_slot_913437_0_us61_2;
  unsigned char *llvm_cbe_slot_913437_0_us61_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_2;
  unsigned char *llvm_cbe_slot_913437_059_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp4639;
  unsigned int llvm_cbe_tmp4640;
  unsigned int llvm_cbe_tmp4652;
  unsigned char *llvm_cbe_slot_913437_0_us61_3;
  unsigned char *llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_3;
  unsigned char *llvm_cbe_slot_913437_059_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_skipbytes133_8;
  unsigned int llvm_cbe_skipbytes133_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_newoptions_8;
  unsigned int llvm_cbe_newoptions_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_reset_bracount132_9;
  unsigned int llvm_cbe_reset_bracount132_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_bravalue_8;
  unsigned int llvm_cbe_bravalue_8__PHI_TEMPORARY;
  unsigned char *llvm_cbe_iftmp_467_0;
  unsigned int llvm_cbe_tmp4674;
  unsigned int llvm_cbe_tmp4695;
  unsigned int llvm_cbe_condcount_6;
  unsigned int llvm_cbe_condcount_6__PHI_TEMPORARY;
  unsigned int llvm_cbe_tc_6_rec;
  unsigned int llvm_cbe_tc_6_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp4716;
  unsigned char llvm_cbe_tmp4719;
  unsigned char llvm_cbe_tmp4724;
  unsigned int llvm_cbe_tmp4728_rec;
  unsigned char llvm_cbe_tmp4730;
  unsigned char llvm_cbe_tmp4737;
  unsigned char *llvm_cbe_tmp519813489;
  unsigned char *llvm_cbe_tmp519813490;
  unsigned int llvm_cbe_bravalue_9;
  unsigned int llvm_cbe_bravalue_9__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4766;
  unsigned char llvm_cbe_tmp4767;
  unsigned int llvm_cbe_tmp4780;
  unsigned int llvm_cbe_tmp4781;
  unsigned char *llvm_cbe_tmp4801;
  unsigned char *llvm_cbe_tmp4803;
  unsigned char *llvm_cbe_code105_26;
  unsigned char *llvm_cbe_code105_26__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp4823;
  unsigned int llvm_cbe_tmp4837;
  unsigned int llvm_cbe_groupsetfirstbyte_4;
  unsigned int llvm_cbe_groupsetfirstbyte_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_3;
  unsigned int llvm_cbe_zerofirstbyte_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_11;
  unsigned int llvm_cbe_firstbyte102_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp4848;
  unsigned char *llvm_cbe_tmp519613492;
  unsigned int llvm_cbe_tmp4861;
  unsigned char *llvm_cbe_tmp519613495;
  unsigned char *llvm_cbe_tmp4871;
  unsigned int llvm_cbe_tmp4874;
  unsigned int llvm_cbe_tmp4877;
  unsigned int llvm_cbe_tmp4879;
  unsigned char *llvm_cbe_tmp4895;
  unsigned char llvm_cbe_tmp4897;
  unsigned char llvm_cbe_tmp4905;
  unsigned int llvm_cbe_firstbyte102_12;
  unsigned int llvm_cbe_firstbyte102_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp4949;
  unsigned char *llvm_cbe_tmp4950;
  unsigned char llvm_cbe_tmp4951;
  unsigned char llvm_cbe_tmp4975;
  unsigned int llvm_cbe_tmp5002;
  unsigned int llvm_cbe_recno_13;
  unsigned int llvm_cbe_recno_13__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_14;
  unsigned char *llvm_cbe_save_hwm_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_5;
  unsigned int llvm_cbe_zerofirstbyte_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_5;
  unsigned int llvm_cbe_zeroreqbyte_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_14;
  unsigned int llvm_cbe_firstbyte102_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_16;
  unsigned char *llvm_cbe_tmp5024;
  unsigned int llvm_cbe_tmp5027;
  unsigned int llvm_cbe_tmp5043;
  unsigned char *llvm_cbe_tmp519613501;
  unsigned char *llvm_cbe_tmp519813504;
  unsigned char *llvm_cbe_iftmp_490_0;
  unsigned char *llvm_cbe_tmp5089;
  unsigned char *llvm_cbe_tmp519613505;
  unsigned int llvm_cbe_storemerge26707_in;
  unsigned int llvm_cbe_storemerge26707_in__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_15771_4;
  unsigned char *llvm_cbe_last_code_15771_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_25789_11;
  unsigned int llvm_cbe_options_addr_25789_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_11;
  unsigned char *llvm_cbe_save_hwm_79347_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_11;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_11;
  unsigned int llvm_cbe_options104_39466_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_11;
  unsigned int llvm_cbe_req_caseopt_39708_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_11;
  unsigned int llvm_cbe_reqbyte103_59784_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_11;
  unsigned int llvm_cbe_firstbyte102_39829_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_11;
  unsigned int llvm_cbe_greedy_non_default_39879_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_11;
  unsigned int llvm_cbe_greedy_default_39909_11__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_3;
  unsigned char *llvm_cbe_previous_callout_3__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_6;
  unsigned int llvm_cbe_inescq_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_8;
  unsigned char *llvm_cbe_code105_8__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_3;
  unsigned int llvm_cbe_after_manual_callout_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_code_15771_5;
  unsigned char *llvm_cbe_last_code_15771_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_25789_12;
  unsigned int llvm_cbe_options_addr_25789_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_12;
  unsigned char *llvm_cbe_save_hwm_79347_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_12;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_12;
  unsigned int llvm_cbe_options104_39466_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_12;
  unsigned int llvm_cbe_req_caseopt_39708_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_12;
  unsigned int llvm_cbe_reqbyte103_59784_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_12;
  unsigned int llvm_cbe_firstbyte102_39829_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_12;
  unsigned int llvm_cbe_greedy_non_default_39879_12__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_12;
  unsigned int llvm_cbe_greedy_default_39909_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_312578_1;
  unsigned char *llvm_cbe_previous_callout_312578_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_612580_1;
  unsigned int llvm_cbe_inescq_612580_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_812582_1;
  unsigned char *llvm_cbe_code105_812582_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_312586_1;
  unsigned int llvm_cbe_after_manual_callout_312586_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_3812612_0;
  unsigned int llvm_cbe_c_3812612_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_2712615_0;
  unsigned char *llvm_cbe_code105_2712615_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp5114;
  unsigned int llvm_cbe_tmp5119;
  unsigned char *llvm_cbe_last_code_15771_11;
  unsigned char *llvm_cbe_last_code_15771_11__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_25789_14;
  unsigned int llvm_cbe_options_addr_25789_14__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_79347_14;
  unsigned char *llvm_cbe_save_hwm_79347_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_14;
  unsigned int llvm_cbe_groupsetfirstbyte_29395_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_39466_14;
  unsigned int llvm_cbe_options104_39466_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_39708_14;
  unsigned int llvm_cbe_req_caseopt_39708_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte103_59784_14;
  unsigned int llvm_cbe_reqbyte103_59784_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_39829_14;
  unsigned int llvm_cbe_firstbyte102_39829_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_39879_14;
  unsigned int llvm_cbe_greedy_non_default_39879_14__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_39909_14;
  unsigned int llvm_cbe_greedy_default_39909_14__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_312578_0;
  unsigned char *llvm_cbe_previous_callout_312578_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_612580_0;
  unsigned int llvm_cbe_inescq_612580_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_812582_0;
  unsigned char *llvm_cbe_code105_812582_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_after_manual_callout_312586_0;
  unsigned int llvm_cbe_after_manual_callout_312586_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_8_pn;
  unsigned char *llvm_cbe_code105_8_pn__PHI_TEMPORARY;
  unsigned int llvm_cbe_c_38;
  unsigned int llvm_cbe_c_38__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_27;
  unsigned char llvm_cbe_tmp5146;
  unsigned int llvm_cbe_tmp5149;
  unsigned char *llvm_cbe_tmp519613508;
  unsigned char llvm_cbe_tmp5184;
  unsigned int llvm_cbe_tmp5190;
  unsigned int llvm_cbe_tmp5191;
  unsigned char *llvm_cbe_tmp519613511;
  unsigned char *llvm_cbe_slot_913437_0_us61_5;
  unsigned char *llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_5;
  unsigned char *llvm_cbe_slot_913437_059_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_1;
  unsigned int llvm_cbe_options_addr_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_save_hwm_6;
  unsigned char *llvm_cbe_save_hwm_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_callout_4;
  unsigned char *llvm_cbe_previous_callout_4__PHI_TEMPORARY;
  unsigned char *llvm_cbe_previous_6;
  unsigned char *llvm_cbe_previous_6__PHI_TEMPORARY;
  unsigned int llvm_cbe_groupsetfirstbyte_1;
  unsigned int llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_inescq_1;
  unsigned int llvm_cbe_inescq_1__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code105_9;
  unsigned char *llvm_cbe_code105_9__PHI_TEMPORARY;
  unsigned int llvm_cbe_options104_2;
  unsigned int llvm_cbe_options104_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_req_caseopt_2;
  unsigned int llvm_cbe_req_caseopt_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_zerofirstbyte_1;
  unsigned int llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_zeroreqbyte_1;
  unsigned int llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte102_2;
  unsigned int llvm_cbe_firstbyte102_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_non_default_2;
  unsigned int llvm_cbe_greedy_non_default_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_greedy_default_2;
  unsigned int llvm_cbe_greedy_default_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp5196;
  unsigned char *llvm_cbe_slot_913437_0_us61_12;
  unsigned char *llvm_cbe_slot_913437_0_us61_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_12;
  unsigned char *llvm_cbe_slot_913437_059_12__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp5198;
  unsigned char *llvm_cbe_slot_913437_0_us61_7;
  unsigned char *llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY;
  unsigned char *llvm_cbe_slot_913437_059_7;
  unsigned char *llvm_cbe_slot_913437_059_7__PHI_TEMPORARY;
  unsigned int llvm_cbe_options_addr_25789_6;
  unsigned int llvm_cbe_options_addr_25789_6__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchreqbyte_5;
  unsigned int llvm_cbe_branchreqbyte_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchfirstbyte_0;
  unsigned int llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_code_6;
  unsigned char *llvm_cbe_code_6__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_1;
  unsigned char *llvm_cbe_ptr_1__PHI_TEMPORARY;
  bool llvm_cbe_tmp_0;
  bool llvm_cbe_tmp_0__PHI_TEMPORARY;
  unsigned char *llvm_cbe_ptr_15625_2;
  unsigned char *llvm_cbe_ptr_15625_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp5213;
  unsigned int llvm_cbe_max_bracount_0;
  unsigned char llvm_cbe_tmp5229;
  unsigned int llvm_cbe_reqbyte_5;
  unsigned int llvm_cbe_reqbyte_5__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte_4;
  unsigned int llvm_cbe_firstbyte_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_branchreqbyte_0;
  unsigned int llvm_cbe_tmp5287;
  unsigned int llvm_cbe_branchreqbyte_1;
  unsigned int llvm_cbe_branchreqbyte_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte_0;
  unsigned int llvm_cbe_reqbyte_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte_0;
  unsigned int llvm_cbe_firstbyte_0__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp5298;
  unsigned int llvm_cbe_branchreqbyte_2;
  unsigned int llvm_cbe_branchreqbyte_2__PHI_TEMPORARY;
  unsigned int llvm_cbe_reqbyte_1;
  unsigned int llvm_cbe_reqbyte_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_firstbyte_1;
  unsigned int llvm_cbe_firstbyte_1__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp5330;
  unsigned int llvm_cbe_tmp5344;
  unsigned int llvm_cbe_last_branch_0_rec;
  unsigned int llvm_cbe_last_branch_0_rec__PHI_TEMPORARY;
  unsigned int llvm_cbe_branch_length_2;
  unsigned int llvm_cbe_branch_length_2__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp5347;
  unsigned char llvm_cbe_tmp5348;
  unsigned char *llvm_cbe_tmp5352;
  unsigned char llvm_cbe_tmp5353;
  unsigned int llvm_cbe_tmp5355;
  unsigned char *llvm_cbe_tmp5398;
  unsigned char llvm_cbe_tmp5407;
  unsigned char *llvm_cbe_tmp5417;
  unsigned int llvm_cbe_tmp5418;
  unsigned char *llvm_cbe_code_4;
  unsigned char *llvm_cbe_code_4__PHI_TEMPORARY;
  unsigned int llvm_cbe_tmp5439;
  unsigned int llvm_cbe_tmp5440;
  unsigned char *llvm_cbe_tmp5452;
  unsigned char *llvm_cbe_tmp5456;
  unsigned int llvm_cbe_tmp5457;
  unsigned char *llvm_cbe_tmp5484;
  unsigned char *llvm_cbe_code_5;
  unsigned char *llvm_cbe_code_5__PHI_TEMPORARY;
  unsigned char *llvm_cbe_last_branch_3;
  unsigned char *llvm_cbe_last_branch_3__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp5487;

  llvm_cbe_tmp3 = *llvm_cbe_ptrptr;
  llvm_cbe_tmp5 = *llvm_cbe_codeptr;
  *(&llvm_cbe_bc.field0) = llvm_cbe_bcptr;
  llvm_cbe_tmp11 = &llvm_cbe_bc.field1;
  *llvm_cbe_tmp11 = llvm_cbe_tmp5;
  *(&llvm_cbe_length) = (llvm_cbe_skipbytes + ((unsigned int )6));
  *(&llvm_cbe_tmp5[((unsigned int )1)]) = ((unsigned char )0);
  *(&llvm_cbe_tmp5[((unsigned int )2)]) = ((unsigned char )0);
  llvm_cbe_tmp24 = &llvm_cbe_cd->field12;
  llvm_cbe_tmp25 = *llvm_cbe_tmp24;
  llvm_cbe_tmp21 = llvm_cbe_skipbytes + ((unsigned int )3);
  llvm_cbe_tmp22 = &llvm_cbe_tmp5[llvm_cbe_tmp21];
  if ((llvm_cbe_reset_bracount == ((unsigned int )0))) {
    llvm_cbe_code_113600_1_ph__PHI_TEMPORARY = llvm_cbe_tmp22;   /* for PHI node */
    llvm_cbe_ptr_013604_1_ph__PHI_TEMPORARY = llvm_cbe_tmp3;   /* for PHI node */
    llvm_cbe_options_addr_313605_1_ph__PHI_TEMPORARY = llvm_cbe_options;   /* for PHI node */
    llvm_cbe_max_bracount_113905_1_ph__PHI_TEMPORARY = llvm_cbe_tmp25;   /* for PHI node */
    llvm_cbe_reqbyte_313907_1_ph__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
    llvm_cbe_firstbyte_313911_1_ph__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
    llvm_cbe_reverse_count_113916_1_ph__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    llvm_cbe_last_branch_413917_1_ph__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_cond_next_preheader;
  } else {
    llvm_cbe_code_113600_0__PHI_TEMPORARY = llvm_cbe_tmp22;   /* for PHI node */
    llvm_cbe_ptr_013604_0__PHI_TEMPORARY = llvm_cbe_tmp3;   /* for PHI node */
    llvm_cbe_options_addr_313605_0__PHI_TEMPORARY = llvm_cbe_options;   /* for PHI node */
    llvm_cbe_max_bracount_113905_0__PHI_TEMPORARY = llvm_cbe_tmp25;   /* for PHI node */
    llvm_cbe_reqbyte_313907_0__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
    llvm_cbe_firstbyte_313911_0__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
    llvm_cbe_reverse_count_113916_0__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    llvm_cbe_last_branch_413917_0__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_cond_true;
  }

llvm_cbe_cond_true:
  llvm_cbe_slot_913437_0_us61_8 = llvm_cbe_slot_913437_0_us61_8__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_8 = llvm_cbe_slot_913437_059_8__PHI_TEMPORARY;
  llvm_cbe_branchreqbyte_413514_0 = llvm_cbe_branchreqbyte_413514_0__PHI_TEMPORARY;
  llvm_cbe_branchfirstbyte_113557_0 = llvm_cbe_branchfirstbyte_113557_0__PHI_TEMPORARY;
  llvm_cbe_code_113600_0 = llvm_cbe_code_113600_0__PHI_TEMPORARY;
  llvm_cbe_ptr_013604_0 = llvm_cbe_ptr_013604_0__PHI_TEMPORARY;
  llvm_cbe_options_addr_313605_0 = llvm_cbe_options_addr_313605_0__PHI_TEMPORARY;
  llvm_cbe_max_bracount_113905_0 = llvm_cbe_max_bracount_113905_0__PHI_TEMPORARY;
  llvm_cbe_reqbyte_313907_0 = llvm_cbe_reqbyte_313907_0__PHI_TEMPORARY;
  llvm_cbe_firstbyte_313911_0 = llvm_cbe_firstbyte_313911_0__PHI_TEMPORARY;
  llvm_cbe_reverse_count_113916_0 = llvm_cbe_reverse_count_113916_0__PHI_TEMPORARY;
  llvm_cbe_last_branch_413917_0 = llvm_cbe_last_branch_413917_0__PHI_TEMPORARY;
  *llvm_cbe_tmp24 = llvm_cbe_tmp25;
  llvm_cbe_slot_913437_0_us61_11__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_8;   /* for PHI node */
  llvm_cbe_slot_913437_059_11__PHI_TEMPORARY = llvm_cbe_slot_913437_059_8;   /* for PHI node */
  llvm_cbe_branchreqbyte_413514_1_ph__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_0;   /* for PHI node */
  llvm_cbe_branchfirstbyte_113557_1_ph__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_0;   /* for PHI node */
  llvm_cbe_code_113600_1_ph__PHI_TEMPORARY = llvm_cbe_code_113600_0;   /* for PHI node */
  llvm_cbe_ptr_013604_1_ph__PHI_TEMPORARY = llvm_cbe_ptr_013604_0;   /* for PHI node */
  llvm_cbe_options_addr_313605_1_ph__PHI_TEMPORARY = llvm_cbe_options_addr_313605_0;   /* for PHI node */
  llvm_cbe_max_bracount_113905_1_ph__PHI_TEMPORARY = llvm_cbe_max_bracount_113905_0;   /* for PHI node */
  llvm_cbe_reqbyte_313907_1_ph__PHI_TEMPORARY = llvm_cbe_reqbyte_313907_0;   /* for PHI node */
  llvm_cbe_firstbyte_313911_1_ph__PHI_TEMPORARY = llvm_cbe_firstbyte_313911_0;   /* for PHI node */
  llvm_cbe_reverse_count_113916_1_ph__PHI_TEMPORARY = llvm_cbe_reverse_count_113916_0;   /* for PHI node */
  llvm_cbe_last_branch_413917_1_ph__PHI_TEMPORARY = llvm_cbe_last_branch_413917_0;   /* for PHI node */
  goto llvm_cbe_cond_next_preheader;

llvm_cbe_cond_next_preheader:
  llvm_cbe_slot_913437_0_us61_11 = llvm_cbe_slot_913437_0_us61_11__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_11 = llvm_cbe_slot_913437_059_11__PHI_TEMPORARY;
  llvm_cbe_branchreqbyte_413514_1_ph = llvm_cbe_branchreqbyte_413514_1_ph__PHI_TEMPORARY;
  llvm_cbe_branchfirstbyte_113557_1_ph = llvm_cbe_branchfirstbyte_113557_1_ph__PHI_TEMPORARY;
  llvm_cbe_code_113600_1_ph = llvm_cbe_code_113600_1_ph__PHI_TEMPORARY;
  llvm_cbe_ptr_013604_1_ph = llvm_cbe_ptr_013604_1_ph__PHI_TEMPORARY;
  llvm_cbe_options_addr_313605_1_ph = llvm_cbe_options_addr_313605_1_ph__PHI_TEMPORARY;
  llvm_cbe_max_bracount_113905_1_ph = llvm_cbe_max_bracount_113905_1_ph__PHI_TEMPORARY;
  llvm_cbe_reqbyte_313907_1_ph = llvm_cbe_reqbyte_313907_1_ph__PHI_TEMPORARY;
  llvm_cbe_firstbyte_313911_1_ph = llvm_cbe_firstbyte_313911_1_ph__PHI_TEMPORARY;
  llvm_cbe_reverse_count_113916_1_ph = llvm_cbe_reverse_count_113916_1_ph__PHI_TEMPORARY;
  llvm_cbe_last_branch_413917_1_ph = llvm_cbe_last_branch_413917_1_ph__PHI_TEMPORARY;
  llvm_cbe_iftmp_509_0 = (((llvm_cbe_lengthptr == ((unsigned int *)/*NULL*/0))) ? (((unsigned int *)/*NULL*/0)) : ((&llvm_cbe_length)));
  llvm_cbe_tmp143 = &llvm_cbe_cd->field4;
  llvm_cbe_tmp203 = &llvm_cbe_cd->field8;
  llvm_cbe_tmp435 = &llvm_cbe_cd->field3;
  llvm_cbe_tmp453 = &llvm_cbe_cd->field18;
  llvm_cbe_tmp460 = &llvm_cbe_cd->field7;
  llvm_cbe_tmp469 = &llvm_cbe_cd->field19;
  llvm_cbe_tmp508 = &llvm_cbe_cd->field20[((unsigned int )0)];
  llvm_cbe_tmp528 = &llvm_cbe_cd->field20[((unsigned int )1)];
  llvm_cbe_classbits711 = &llvm_cbe_classbits[((unsigned int )0)];
  llvm_cbe_tmp776 = &llvm_cbe_cd->field2;
  llvm_cbe_pbits885 = &llvm_cbe_pbits[((unsigned int )0)];
  llvm_cbe_tmp962 = &llvm_cbe_pbits[((unsigned int )1)];
  llvm_cbe_tmp972 = &llvm_cbe_pbits[((unsigned int )11)];
  llvm_cbe_tmp1237 = &llvm_cbe_classbits[((unsigned int )1)];
  llvm_cbe_tmp1288 = &llvm_cbe_classbits[((unsigned int )4)];
  llvm_cbe_tmp1292 = &llvm_cbe_classbits[((unsigned int )20)];
  llvm_cbe_tmp1353 = &llvm_cbe_classbits[((unsigned int )16)];
  llvm_cbe_tmp1623 = &llvm_cbe_cd->field1;
  llvm_cbe_tmp1755 = &llvm_cbe_mcbuffer[((unsigned int )0)];
  llvm_cbe_tmp1993 = &llvm_cbe_cd->field16;
  llvm_cbe_tmp2377 = &llvm_cbe_cd->field17;
  llvm_cbe_tmp3478 = &llvm_cbe_cd->field9;
  llvm_cbe_tmp351413356 = &llvm_cbe_cd->field10;
  llvm_cbe_tmp3506 = &llvm_cbe_cd->field11;
  llvm_cbe_tmp3783 = &llvm_cbe_cd->field6;
  llvm_cbe_tmp4496 = &llvm_cbe_cd->field15;
  llvm_cbe_tmp4563 = &llvm_cbe_cd->field5;
  llvm_cbe_iftmp_468_0 = (((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) ? (((unsigned int *)/*NULL*/0)) : ((&llvm_cbe_length_prevgroup)));
  llvm_cbe_tmp5026 = &llvm_cbe_cd->field14;
  llvm_cbe_tmp5042 = &llvm_cbe_cd->field13;
  llvm_cbe_tmp138_not = (llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0)) ^ 1;
  llvm_cbe_slot_913437_0_us61_9__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_11;   /* for PHI node */
  llvm_cbe_slot_913437_059_9__PHI_TEMPORARY = llvm_cbe_slot_913437_059_11;   /* for PHI node */
  llvm_cbe_branchreqbyte_413514_1__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1_ph;   /* for PHI node */
  llvm_cbe_branchfirstbyte_113557_1__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1_ph;   /* for PHI node */
  llvm_cbe_code_113600_1__PHI_TEMPORARY = llvm_cbe_code_113600_1_ph;   /* for PHI node */
  llvm_cbe_ptr_013604_1__PHI_TEMPORARY = llvm_cbe_ptr_013604_1_ph;   /* for PHI node */
  llvm_cbe_options_addr_313605_1__PHI_TEMPORARY = llvm_cbe_options_addr_313605_1_ph;   /* for PHI node */
  llvm_cbe_max_bracount_113905_1__PHI_TEMPORARY = llvm_cbe_max_bracount_113905_1_ph;   /* for PHI node */
  llvm_cbe_reqbyte_313907_1__PHI_TEMPORARY = llvm_cbe_reqbyte_313907_1_ph;   /* for PHI node */
  llvm_cbe_firstbyte_313911_1__PHI_TEMPORARY = llvm_cbe_firstbyte_313911_1_ph;   /* for PHI node */
  llvm_cbe_reverse_count_113916_1__PHI_TEMPORARY = llvm_cbe_reverse_count_113916_1_ph;   /* for PHI node */
  llvm_cbe_last_branch_413917_1__PHI_TEMPORARY = llvm_cbe_last_branch_413917_1_ph;   /* for PHI node */
  goto llvm_cbe_cond_next;

  do {     /* Syntactic loop 'cond_next' to make GCC happy */
llvm_cbe_cond_next:
  llvm_cbe_slot_913437_0_us61_9 = llvm_cbe_slot_913437_0_us61_9__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_9 = llvm_cbe_slot_913437_059_9__PHI_TEMPORARY;
  llvm_cbe_branchreqbyte_413514_1 = llvm_cbe_branchreqbyte_413514_1__PHI_TEMPORARY;
  llvm_cbe_branchfirstbyte_113557_1 = llvm_cbe_branchfirstbyte_113557_1__PHI_TEMPORARY;
  llvm_cbe_code_113600_1 = llvm_cbe_code_113600_1__PHI_TEMPORARY;
  llvm_cbe_ptr_013604_1 = llvm_cbe_ptr_013604_1__PHI_TEMPORARY;
  llvm_cbe_options_addr_313605_1 = llvm_cbe_options_addr_313605_1__PHI_TEMPORARY;
  llvm_cbe_max_bracount_113905_1 = llvm_cbe_max_bracount_113905_1__PHI_TEMPORARY;
  llvm_cbe_reqbyte_313907_1 = llvm_cbe_reqbyte_313907_1__PHI_TEMPORARY;
  llvm_cbe_firstbyte_313911_1 = llvm_cbe_firstbyte_313911_1__PHI_TEMPORARY;
  llvm_cbe_reverse_count_113916_1 = llvm_cbe_reverse_count_113916_1__PHI_TEMPORARY;
  llvm_cbe_last_branch_413917_1 = llvm_cbe_last_branch_413917_1__PHI_TEMPORARY;
  if (((llvm_cbe_options_addr_313605_1 & ((unsigned int )7)) == llvm_cbe_oldims)) {
    llvm_cbe_code_0__PHI_TEMPORARY = llvm_cbe_code_113600_1;   /* for PHI node */
    goto llvm_cbe_cond_next49;
  } else {
    goto llvm_cbe_cond_true39;
  }

llvm_cbe_cond_next5485:
  llvm_cbe_code_5 = llvm_cbe_code_5__PHI_TEMPORARY;
  llvm_cbe_last_branch_3 = llvm_cbe_last_branch_3__PHI_TEMPORARY;
  llvm_cbe_tmp5487 = &llvm_cbe_ptr_1[((unsigned int )1)];
  if ((llvm_cbe_reset_bracount == ((unsigned int )0))) {
    llvm_cbe_slot_913437_0_us61_9__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_7;   /* for PHI node */
    llvm_cbe_slot_913437_059_9__PHI_TEMPORARY = llvm_cbe_slot_913437_059_7;   /* for PHI node */
    llvm_cbe_branchreqbyte_413514_1__PHI_TEMPORARY = llvm_cbe_branchreqbyte_2;   /* for PHI node */
    llvm_cbe_branchfirstbyte_113557_1__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_0;   /* for PHI node */
    llvm_cbe_code_113600_1__PHI_TEMPORARY = llvm_cbe_code_5;   /* for PHI node */
    llvm_cbe_ptr_013604_1__PHI_TEMPORARY = llvm_cbe_tmp5487;   /* for PHI node */
    llvm_cbe_options_addr_313605_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_6;   /* for PHI node */
    llvm_cbe_max_bracount_113905_1__PHI_TEMPORARY = llvm_cbe_max_bracount_0;   /* for PHI node */
    llvm_cbe_reqbyte_313907_1__PHI_TEMPORARY = llvm_cbe_reqbyte_1;   /* for PHI node */
    llvm_cbe_firstbyte_313911_1__PHI_TEMPORARY = llvm_cbe_firstbyte_1;   /* for PHI node */
    llvm_cbe_reverse_count_113916_1__PHI_TEMPORARY = llvm_cbe_reverse_count_0;   /* for PHI node */
    llvm_cbe_last_branch_413917_1__PHI_TEMPORARY = llvm_cbe_last_branch_3;   /* for PHI node */
    goto llvm_cbe_cond_next;
  } else {
    llvm_cbe_slot_913437_0_us61_8__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_7;   /* for PHI node */
    llvm_cbe_slot_913437_059_8__PHI_TEMPORARY = llvm_cbe_slot_913437_059_7;   /* for PHI node */
    llvm_cbe_branchreqbyte_413514_0__PHI_TEMPORARY = llvm_cbe_branchreqbyte_2;   /* for PHI node */
    llvm_cbe_branchfirstbyte_113557_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_0;   /* for PHI node */
    llvm_cbe_code_113600_0__PHI_TEMPORARY = llvm_cbe_code_5;   /* for PHI node */
    llvm_cbe_ptr_013604_0__PHI_TEMPORARY = llvm_cbe_tmp5487;   /* for PHI node */
    llvm_cbe_options_addr_313605_0__PHI_TEMPORARY = llvm_cbe_options_addr_25789_6;   /* for PHI node */
    llvm_cbe_max_bracount_113905_0__PHI_TEMPORARY = llvm_cbe_max_bracount_0;   /* for PHI node */
    llvm_cbe_reqbyte_313907_0__PHI_TEMPORARY = llvm_cbe_reqbyte_1;   /* for PHI node */
    llvm_cbe_firstbyte_313911_0__PHI_TEMPORARY = llvm_cbe_firstbyte_1;   /* for PHI node */
    llvm_cbe_reverse_count_113916_0__PHI_TEMPORARY = llvm_cbe_reverse_count_0;   /* for PHI node */
    llvm_cbe_last_branch_413917_0__PHI_TEMPORARY = llvm_cbe_last_branch_3;   /* for PHI node */
    goto llvm_cbe_cond_true;
  }

llvm_cbe_cond_true5450:
  llvm_cbe_tmp5452 = *llvm_cbe_codeptr;
  llvm_cbe_tmp5456 = &llvm_cbe_tmp5452[llvm_cbe_tmp21];
  llvm_cbe_tmp5457 = *(&llvm_cbe_length);
  *(&llvm_cbe_length) = (llvm_cbe_tmp5457 + ((unsigned int )3));
  llvm_cbe_code_5__PHI_TEMPORARY = llvm_cbe_tmp5456;   /* for PHI node */
  llvm_cbe_last_branch_3__PHI_TEMPORARY = llvm_cbe_last_branch_413917_1;   /* for PHI node */
  goto llvm_cbe_cond_next5485;

llvm_cbe_cond_next5445:
  if ((llvm_cbe_lengthptr == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_false5459;
  } else {
    goto llvm_cbe_cond_true5450;
  }

llvm_cbe_cond_next5328:
  llvm_cbe_branchreqbyte_2 = llvm_cbe_branchreqbyte_2__PHI_TEMPORARY;
  llvm_cbe_reqbyte_1 = llvm_cbe_reqbyte_1__PHI_TEMPORARY;
  llvm_cbe_firstbyte_1 = llvm_cbe_firstbyte_1__PHI_TEMPORARY;
  llvm_cbe_tmp5330 = *llvm_cbe_ptr_1;
  if ((llvm_cbe_tmp5330 == ((unsigned char )124))) {
    goto llvm_cbe_cond_next5445;
  } else {
    goto llvm_cbe_cond_true5334;
  }

llvm_cbe_cond_next5210:
  llvm_cbe_tmp5213 = *llvm_cbe_tmp24;
  llvm_cbe_max_bracount_0 = (((((signed int )llvm_cbe_tmp5213) > ((signed int )llvm_cbe_max_bracount_113905_1))) ? (llvm_cbe_tmp5213) : (llvm_cbe_max_bracount_113905_1));
  if ((llvm_cbe_lengthptr == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_true5227;
  } else {
    llvm_cbe_branchreqbyte_2__PHI_TEMPORARY = llvm_cbe_branchreqbyte_5;   /* for PHI node */
    llvm_cbe_reqbyte_1__PHI_TEMPORARY = llvm_cbe_reqbyte_313907_1;   /* for PHI node */
    llvm_cbe_firstbyte_1__PHI_TEMPORARY = llvm_cbe_firstbyte_313911_1;   /* for PHI node */
    goto llvm_cbe_cond_next5328;
  }

llvm_cbe_bb5201:
  llvm_cbe_slot_913437_0_us61_7 = llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_7 = llvm_cbe_slot_913437_059_7__PHI_TEMPORARY;
  llvm_cbe_options_addr_25789_6 = llvm_cbe_options_addr_25789_6__PHI_TEMPORARY;
  llvm_cbe_branchreqbyte_5 = llvm_cbe_branchreqbyte_5__PHI_TEMPORARY;
  llvm_cbe_branchfirstbyte_0 = llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY;
  llvm_cbe_code_6 = llvm_cbe_code_6__PHI_TEMPORARY;
  llvm_cbe_ptr_1 = llvm_cbe_ptr_1__PHI_TEMPORARY;
  llvm_cbe_tmp_0 = llvm_cbe_tmp_0__PHI_TEMPORARY;
  if (llvm_cbe_tmp_0) {
    llvm_cbe_ptr_15625_2__PHI_TEMPORARY = llvm_cbe_ptr_1;   /* for PHI node */
    goto llvm_cbe_cond_true5206;
  } else {
    goto llvm_cbe_cond_next5210;
  }

llvm_cbe_cond_true212:
  *llvm_cbe_errorcodeptr = ((unsigned int )52);
  llvm_cbe_tmp51985785 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_4;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp51985785;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

  do {     /* Syntactic loop 'bb131' to make GCC happy */
llvm_cbe_bb131:
  llvm_cbe_slot_913437_0_us61_13 = llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_13 = llvm_cbe_slot_913437_059_13__PHI_TEMPORARY;
  llvm_cbe_repeat_max_10 = llvm_cbe_repeat_max_10__PHI_TEMPORARY;
  llvm_cbe_repeat_min_10 = llvm_cbe_repeat_min_10__PHI_TEMPORARY;
  llvm_cbe_options_addr_2 = llvm_cbe_options_addr_2__PHI_TEMPORARY;
  llvm_cbe_save_hwm_7 = llvm_cbe_save_hwm_7__PHI_TEMPORARY;
  llvm_cbe_previous_callout_5 = llvm_cbe_previous_callout_5__PHI_TEMPORARY;
  llvm_cbe_previous_3 = llvm_cbe_previous_3__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_2 = llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY;
  llvm_cbe_inescq_2 = llvm_cbe_inescq_2__PHI_TEMPORARY;
  llvm_cbe_last_code_2 = llvm_cbe_last_code_2__PHI_TEMPORARY;
  llvm_cbe_code105_3 = llvm_cbe_code105_3__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_7 = llvm_cbe_after_manual_callout_7__PHI_TEMPORARY;
  llvm_cbe_options104_3 = llvm_cbe_options104_3__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_3 = llvm_cbe_req_caseopt_3__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_2 = llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_2 = llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_5 = llvm_cbe_reqbyte103_5__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_3 = llvm_cbe_firstbyte102_3__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_3 = llvm_cbe_greedy_non_default_3__PHI_TEMPORARY;
  llvm_cbe_greedy_default_3 = llvm_cbe_greedy_default_3__PHI_TEMPORARY;
  llvm_cbe_tmp134 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp135 = *llvm_cbe_tmp134;
  llvm_cbe_tmp135136 = ((unsigned int )(unsigned char )llvm_cbe_tmp135);
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    llvm_cbe_options_addr_25789_4__PHI_TEMPORARY = llvm_cbe_options_addr_2;   /* for PHI node */
    llvm_cbe_save_hwm_79347_4__PHI_TEMPORARY = llvm_cbe_save_hwm_7;   /* for PHI node */
    llvm_cbe_previous_callout_59381_4__PHI_TEMPORARY = llvm_cbe_previous_callout_5;   /* for PHI node */
    llvm_cbe_previous_39389_4__PHI_TEMPORARY = llvm_cbe_previous_3;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_4__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_2;   /* for PHI node */
    llvm_cbe_inescq_29422_4__PHI_TEMPORARY = llvm_cbe_inescq_2;   /* for PHI node */
    llvm_cbe_last_code_29452_4__PHI_TEMPORARY = llvm_cbe_last_code_2;   /* for PHI node */
    llvm_cbe_code105_39456_4__PHI_TEMPORARY = llvm_cbe_code105_3;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_4__PHI_TEMPORARY = llvm_cbe_after_manual_callout_7;   /* for PHI node */
    llvm_cbe_options104_39466_4__PHI_TEMPORARY = llvm_cbe_options104_3;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_4__PHI_TEMPORARY = llvm_cbe_req_caseopt_3;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_4__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_2;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_4__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_2;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_4__PHI_TEMPORARY = llvm_cbe_reqbyte103_5;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_4__PHI_TEMPORARY = llvm_cbe_firstbyte102_3;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_4__PHI_TEMPORARY = llvm_cbe_greedy_non_default_3;   /* for PHI node */
    llvm_cbe_greedy_default_39909_4__PHI_TEMPORARY = llvm_cbe_greedy_default_3;   /* for PHI node */
    llvm_cbe_tmp13512351_4__PHI_TEMPORARY = llvm_cbe_tmp135;   /* for PHI node */
    llvm_cbe_tmp13513612358_4__PHI_TEMPORARY = llvm_cbe_tmp135136;   /* for PHI node */
    goto llvm_cbe_cond_false201;
  } else {
    llvm_cbe_options_addr_25789_3__PHI_TEMPORARY = llvm_cbe_options_addr_2;   /* for PHI node */
    llvm_cbe_save_hwm_79347_3__PHI_TEMPORARY = llvm_cbe_save_hwm_7;   /* for PHI node */
    llvm_cbe_previous_callout_59381_3__PHI_TEMPORARY = llvm_cbe_previous_callout_5;   /* for PHI node */
    llvm_cbe_previous_39389_3__PHI_TEMPORARY = llvm_cbe_previous_3;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_3__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_2;   /* for PHI node */
    llvm_cbe_inescq_29422_3__PHI_TEMPORARY = llvm_cbe_inescq_2;   /* for PHI node */
    llvm_cbe_last_code_29452_3__PHI_TEMPORARY = llvm_cbe_last_code_2;   /* for PHI node */
    llvm_cbe_code105_39456_3__PHI_TEMPORARY = llvm_cbe_code105_3;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_3__PHI_TEMPORARY = llvm_cbe_after_manual_callout_7;   /* for PHI node */
    llvm_cbe_options104_39466_3__PHI_TEMPORARY = llvm_cbe_options104_3;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_3;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_3__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_2;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_3__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_2;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_3__PHI_TEMPORARY = llvm_cbe_reqbyte103_5;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_3;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_3;   /* for PHI node */
    llvm_cbe_greedy_default_39909_3__PHI_TEMPORARY = llvm_cbe_greedy_default_3;   /* for PHI node */
    llvm_cbe_tmp13512351_3__PHI_TEMPORARY = llvm_cbe_tmp135;   /* for PHI node */
    llvm_cbe_tmp13513612358_3__PHI_TEMPORARY = llvm_cbe_tmp135136;   /* for PHI node */
    goto llvm_cbe_cond_true141;
  }

llvm_cbe_bb615:
  llvm_cbe_firstbyte102_1 = ((((llvm_cbe_firstbyte102_39829_9 == ((unsigned int )-2)) & ((llvm_cbe_options104_39466_9 & ((unsigned int )2)) != ((unsigned int )0)))) ? (((unsigned int )-1)) : (llvm_cbe_firstbyte102_39829_9));
  *llvm_cbe_code105_10 = ((unsigned char )25);
  llvm_cbe_tmp631 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_tmp519612381 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519612381[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp631;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_1;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next586:
  llvm_cbe_previous_callout_9 = llvm_cbe_previous_callout_9__PHI_TEMPORARY;
  llvm_cbe_code105_10 = llvm_cbe_code105_10__PHI_TEMPORARY;
  switch (llvm_cbe_c_2) {
  default:
    llvm_cbe_storemerge26707_in__PHI_TEMPORARY = llvm_cbe_c_2;   /* for PHI node */
    llvm_cbe_last_code_15771_4__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
    llvm_cbe_options_addr_25789_11__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_79347_11__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_11__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_options104_39466_11__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_11__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_11__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_39909_11__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    llvm_cbe_previous_callout_3__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_inescq_6__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_8__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_after_manual_callout_3__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
    goto llvm_cbe_ONE_CHAR;
;
  case ((unsigned int )0):
    goto llvm_cbe_bb590;
    break;
  case ((unsigned int )36):
    goto llvm_cbe_bb632;
  case ((unsigned int )40):
    goto llvm_cbe_bb3172;
  case ((unsigned int )41):
    goto llvm_cbe_bb590;
    break;
  case ((unsigned int )42):
    goto llvm_cbe_bb1921;
  case ((unsigned int )43):
    llvm_cbe_repeat_max_4__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_repeat_min_4__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_REPEAT;
  case ((unsigned int )46):
    goto llvm_cbe_bb636;
  case ((unsigned int )63):
    goto llvm_cbe_bb1923;
  case ((unsigned int )91):
    goto llvm_cbe_bb649;
  case ((unsigned int )92):
    goto llvm_cbe_bb4870;
  case ((unsigned int )94):
    goto llvm_cbe_bb615;
  case ((unsigned int )123):
    goto llvm_cbe_bb1803;
  case ((unsigned int )124):
    goto llvm_cbe_bb590;
    break;
  }
llvm_cbe_cond_next566:
  llvm_cbe_c_2 = llvm_cbe_c_2__PHI_TEMPORARY;
  if ((llvm_cbe_iftmp_136_0 & ((llvm_cbe_options104_39466_9 & ((unsigned int )16384)) != ((unsigned int )0)))) {
    goto llvm_cbe_cond_true580;
  } else {
    llvm_cbe_previous_callout_9__PHI_TEMPORARY = llvm_cbe_previous_callout_6;   /* for PHI node */
    llvm_cbe_code105_10__PHI_TEMPORARY = llvm_cbe_code105_25773_1;   /* for PHI node */
    goto llvm_cbe_cond_next586;
  }

llvm_cbe_cond_next427:
  llvm_cbe_previous_callout_6 = llvm_cbe_previous_callout_6__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_1 = llvm_cbe_after_manual_callout_1__PHI_TEMPORARY;
  if (((llvm_cbe_options104_39466_9 & ((unsigned int )8)) == ((unsigned int )0))) {
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp13513612358_9;   /* for PHI node */
    goto llvm_cbe_cond_next566;
  } else {
    goto llvm_cbe_cond_true433;
  }

llvm_cbe_bb396:
  llvm_cbe_iftmp_136_0 = llvm_cbe_iftmp_136_0__PHI_TEMPORARY;
  if ((llvm_cbe_iftmp_136_0 & (llvm_cbe_previous_callout_59381_9 != ((unsigned char *)/*NULL*/0)))) {
    goto llvm_cbe_cond_true409;
  } else {
    llvm_cbe_previous_callout_6__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_9;   /* for PHI node */
    llvm_cbe_after_manual_callout_1__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_9;   /* for PHI node */
    goto llvm_cbe_cond_next427;
  }

llvm_cbe_cond_next272:
  llvm_cbe_previous_25708_1 = llvm_cbe_previous_25708_1__PHI_TEMPORARY;
  llvm_cbe_last_code_15771_1 = llvm_cbe_last_code_15771_1__PHI_TEMPORARY;
  llvm_cbe_code105_25773_1 = llvm_cbe_code105_25773_1__PHI_TEMPORARY;
  llvm_cbe_options_addr_25789_9 = llvm_cbe_options_addr_25789_9__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_9 = llvm_cbe_save_hwm_79347_9__PHI_TEMPORARY;
  llvm_cbe_previous_callout_59381_9 = llvm_cbe_previous_callout_59381_9__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_9 = llvm_cbe_groupsetfirstbyte_29395_9__PHI_TEMPORARY;
  llvm_cbe_inescq_29422_9 = llvm_cbe_inescq_29422_9__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_79460_9 = llvm_cbe_after_manual_callout_79460_9__PHI_TEMPORARY;
  llvm_cbe_options104_39466_9 = llvm_cbe_options104_39466_9__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_9 = llvm_cbe_req_caseopt_39708_9__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_29740_9 = llvm_cbe_zerofirstbyte_29740_9__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_29762_9 = llvm_cbe_zeroreqbyte_29762_9__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_9 = llvm_cbe_reqbyte103_59784_9__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_9 = llvm_cbe_firstbyte102_39829_9__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_9 = llvm_cbe_greedy_non_default_39879_9__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_9 = llvm_cbe_greedy_default_39909_9__PHI_TEMPORARY;
  llvm_cbe_tmp13512351_9 = llvm_cbe_tmp13512351_9__PHI_TEMPORARY;
  llvm_cbe_tmp13513612358_9 = llvm_cbe_tmp13513612358_9__PHI_TEMPORARY;
  if (((((unsigned int )(llvm_cbe_tmp13513612358_9 + ((unsigned int )-42))) < ((unsigned int )((unsigned int )2))) | (llvm_cbe_tmp13512351_9 == ((unsigned char )63)))) {
    llvm_cbe_iftmp_136_0__PHI_TEMPORARY = 0;   /* for PHI node */
    goto llvm_cbe_bb396;
  } else {
    goto llvm_cbe_cond_next286;
  }

llvm_cbe_cond_true180:
  llvm_cbe_tmp183184 = ((unsigned int )(unsigned long)llvm_cbe_previous_39389_3);
  ltmp_4_1 = memmove(llvm_cbe_code_2, llvm_cbe_previous_39389_3, (llvm_cbe_tmp163164 - llvm_cbe_tmp183184));
  llvm_cbe_tmp194 = &llvm_cbe_code105_1[(llvm_cbe_tmp188189 - llvm_cbe_tmp183184)];
  if (((llvm_cbe_tmp13512351_3 != ((unsigned char )0)) & (llvm_cbe_inescq_29422_3 != ((unsigned int )0)))) {
    llvm_cbe_previous_25708_0__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
    llvm_cbe_last_code_15771_0__PHI_TEMPORARY = llvm_cbe_tmp194;   /* for PHI node */
    llvm_cbe_code105_25773_0__PHI_TEMPORARY = llvm_cbe_tmp194;   /* for PHI node */
    llvm_cbe_options_addr_25789_8__PHI_TEMPORARY = llvm_cbe_options_addr_25789_3;   /* for PHI node */
    llvm_cbe_save_hwm_79347_8__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_3;   /* for PHI node */
    llvm_cbe_previous_callout_59381_8__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_3;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_8__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_3;   /* for PHI node */
    llvm_cbe_inescq_29422_8__PHI_TEMPORARY = llvm_cbe_inescq_29422_3;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_8__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_3;   /* for PHI node */
    llvm_cbe_options104_39466_8__PHI_TEMPORARY = llvm_cbe_options104_39466_3;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_8__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_3;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_8__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_3;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_8__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_3;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_3;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_8__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_3;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_8__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_3;   /* for PHI node */
    llvm_cbe_greedy_default_39909_8__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_3;   /* for PHI node */
    llvm_cbe_tmp13512351_8__PHI_TEMPORARY = llvm_cbe_tmp13512351_3;   /* for PHI node */
    llvm_cbe_tmp13513612358_8__PHI_TEMPORARY = llvm_cbe_tmp13513612358_3;   /* for PHI node */
    goto llvm_cbe_cond_true227;
  } else {
    llvm_cbe_previous_25708_1__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
    llvm_cbe_last_code_15771_1__PHI_TEMPORARY = llvm_cbe_tmp194;   /* for PHI node */
    llvm_cbe_code105_25773_1__PHI_TEMPORARY = llvm_cbe_tmp194;   /* for PHI node */
    llvm_cbe_options_addr_25789_9__PHI_TEMPORARY = llvm_cbe_options_addr_25789_3;   /* for PHI node */
    llvm_cbe_save_hwm_79347_9__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_3;   /* for PHI node */
    llvm_cbe_previous_callout_59381_9__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_3;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_9__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_3;   /* for PHI node */
    llvm_cbe_inescq_29422_9__PHI_TEMPORARY = llvm_cbe_inescq_29422_3;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_9__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_3;   /* for PHI node */
    llvm_cbe_options104_39466_9__PHI_TEMPORARY = llvm_cbe_options104_39466_3;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_9__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_3;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_9__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_3;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_9__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_3;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_3;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_9__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_3;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_9__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_3;   /* for PHI node */
    llvm_cbe_greedy_default_39909_9__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_3;   /* for PHI node */
    llvm_cbe_tmp13512351_9__PHI_TEMPORARY = llvm_cbe_tmp13512351_3;   /* for PHI node */
    llvm_cbe_tmp13513612358_9__PHI_TEMPORARY = llvm_cbe_tmp13513612358_3;   /* for PHI node */
    goto llvm_cbe_cond_next272;
  }

llvm_cbe_cond_true174:
  if ((((unsigned char *)llvm_cbe_previous_39389_3) > ((unsigned char *)llvm_cbe_code_2))) {
    goto llvm_cbe_cond_true180;
  } else {
    llvm_cbe_options_addr_25789_5__PHI_TEMPORARY = llvm_cbe_options_addr_25789_3;   /* for PHI node */
    llvm_cbe_save_hwm_79347_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_3;   /* for PHI node */
    llvm_cbe_previous_callout_59381_5__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_3;   /* for PHI node */
    llvm_cbe_previous_39389_5__PHI_TEMPORARY = llvm_cbe_previous_39389_3;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_5__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_3;   /* for PHI node */
    llvm_cbe_inescq_29422_5__PHI_TEMPORARY = llvm_cbe_inescq_29422_3;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_5__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_3;   /* for PHI node */
    llvm_cbe_options104_39466_5__PHI_TEMPORARY = llvm_cbe_options104_39466_3;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_5__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_3;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_5__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_3;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_5__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_3;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_3;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_5__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_3;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_5__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_3;   /* for PHI node */
    llvm_cbe_greedy_default_39909_5__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_3;   /* for PHI node */
    llvm_cbe_tmp13512351_5__PHI_TEMPORARY = llvm_cbe_tmp13512351_3;   /* for PHI node */
    llvm_cbe_tmp13513612358_5__PHI_TEMPORARY = llvm_cbe_tmp13513612358_3;   /* for PHI node */
    llvm_cbe_last_code_1__PHI_TEMPORARY = llvm_cbe_code105_1;   /* for PHI node */
    llvm_cbe_code105_2__PHI_TEMPORARY = llvm_cbe_code105_1;   /* for PHI node */
    goto llvm_cbe_cond_next215;
  }

llvm_cbe_cond_next152:
  llvm_cbe_code105_1 = (((((unsigned char *)llvm_cbe_code105_39456_3) < ((unsigned char *)llvm_cbe_last_code_29452_3))) ? (llvm_cbe_last_code_29452_3) : (llvm_cbe_code105_39456_3));
  llvm_cbe_tmp162 = *llvm_cbe_iftmp_509_0;
  llvm_cbe_tmp163164 = ((unsigned int )(unsigned long)llvm_cbe_code105_1);
  *llvm_cbe_iftmp_509_0 = ((llvm_cbe_tmp163164 - (((unsigned int )(unsigned long)llvm_cbe_last_code_29452_3))) + llvm_cbe_tmp162);
  if ((llvm_cbe_previous_39389_3 == ((unsigned char *)/*NULL*/0))) {
    llvm_cbe_options_addr_25789_5__PHI_TEMPORARY = llvm_cbe_options_addr_25789_3;   /* for PHI node */
    llvm_cbe_save_hwm_79347_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_3;   /* for PHI node */
    llvm_cbe_previous_callout_59381_5__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_3;   /* for PHI node */
    llvm_cbe_previous_39389_5__PHI_TEMPORARY = llvm_cbe_previous_39389_3;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_5__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_3;   /* for PHI node */
    llvm_cbe_inescq_29422_5__PHI_TEMPORARY = llvm_cbe_inescq_29422_3;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_5__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_3;   /* for PHI node */
    llvm_cbe_options104_39466_5__PHI_TEMPORARY = llvm_cbe_options104_39466_3;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_5__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_3;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_5__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_3;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_5__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_3;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_3;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_5__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_3;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_5__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_3;   /* for PHI node */
    llvm_cbe_greedy_default_39909_5__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_3;   /* for PHI node */
    llvm_cbe_tmp13512351_5__PHI_TEMPORARY = llvm_cbe_tmp13512351_3;   /* for PHI node */
    llvm_cbe_tmp13513612358_5__PHI_TEMPORARY = llvm_cbe_tmp13513612358_3;   /* for PHI node */
    llvm_cbe_last_code_1__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
    llvm_cbe_code105_2__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
    goto llvm_cbe_cond_next215;
  } else {
    goto llvm_cbe_cond_true174;
  }

llvm_cbe_cond_true141:
  llvm_cbe_options_addr_25789_3 = llvm_cbe_options_addr_25789_3__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_3 = llvm_cbe_save_hwm_79347_3__PHI_TEMPORARY;
  llvm_cbe_previous_callout_59381_3 = llvm_cbe_previous_callout_59381_3__PHI_TEMPORARY;
  llvm_cbe_previous_39389_3 = llvm_cbe_previous_39389_3__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_3 = llvm_cbe_groupsetfirstbyte_29395_3__PHI_TEMPORARY;
  llvm_cbe_inescq_29422_3 = llvm_cbe_inescq_29422_3__PHI_TEMPORARY;
  llvm_cbe_last_code_29452_3 = llvm_cbe_last_code_29452_3__PHI_TEMPORARY;
  llvm_cbe_code105_39456_3 = llvm_cbe_code105_39456_3__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_79460_3 = llvm_cbe_after_manual_callout_79460_3__PHI_TEMPORARY;
  llvm_cbe_options104_39466_3 = llvm_cbe_options104_39466_3__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_3 = llvm_cbe_req_caseopt_39708_3__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_29740_3 = llvm_cbe_zerofirstbyte_29740_3__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_29762_3 = llvm_cbe_zeroreqbyte_29762_3__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_3 = llvm_cbe_reqbyte103_59784_3__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_3 = llvm_cbe_firstbyte102_39829_3__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_3 = llvm_cbe_greedy_non_default_39879_3__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_3 = llvm_cbe_greedy_default_39909_3__PHI_TEMPORARY;
  llvm_cbe_tmp13512351_3 = llvm_cbe_tmp13512351_3__PHI_TEMPORARY;
  llvm_cbe_tmp13513612358_3 = llvm_cbe_tmp13513612358_3__PHI_TEMPORARY;
  llvm_cbe_tmp144 = *llvm_cbe_tmp143;
  if ((((unsigned char *)(&llvm_cbe_tmp144[((unsigned int )4096)])) < ((unsigned char *)llvm_cbe_code105_39456_3))) {
    goto llvm_cbe_cond_true150;
  } else {
    goto llvm_cbe_cond_next152;
  }

llvm_cbe_cond_next241:
  llvm_cbe_tmp51975787 = &llvm_cbe_tmp234[((unsigned int )2)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp51975787;
  llvm_cbe_tmp13512365 = *llvm_cbe_tmp51975787;
  llvm_cbe_tmp13513612367 = ((unsigned int )(unsigned char )llvm_cbe_tmp13512365);
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    llvm_cbe_options_addr_25789_4__PHI_TEMPORARY = llvm_cbe_options_addr_25789_8;   /* for PHI node */
    llvm_cbe_save_hwm_79347_4__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_8;   /* for PHI node */
    llvm_cbe_previous_callout_59381_4__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_8;   /* for PHI node */
    llvm_cbe_previous_39389_4__PHI_TEMPORARY = llvm_cbe_previous_25708_0;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_4__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_8;   /* for PHI node */
    llvm_cbe_inescq_29422_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_last_code_29452_4__PHI_TEMPORARY = llvm_cbe_last_code_15771_0;   /* for PHI node */
    llvm_cbe_code105_39456_4__PHI_TEMPORARY = llvm_cbe_code105_25773_0;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_4__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_8;   /* for PHI node */
    llvm_cbe_options104_39466_4__PHI_TEMPORARY = llvm_cbe_options104_39466_8;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_4__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_8;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_4__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_8;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_4__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_8;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_4__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_8;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_4__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_8;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_4__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_8;   /* for PHI node */
    llvm_cbe_greedy_default_39909_4__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_8;   /* for PHI node */
    llvm_cbe_tmp13512351_4__PHI_TEMPORARY = llvm_cbe_tmp13512365;   /* for PHI node */
    llvm_cbe_tmp13513612358_4__PHI_TEMPORARY = llvm_cbe_tmp13513612367;   /* for PHI node */
    goto llvm_cbe_cond_false201;
  } else {
    llvm_cbe_options_addr_25789_3__PHI_TEMPORARY = llvm_cbe_options_addr_25789_8;   /* for PHI node */
    llvm_cbe_save_hwm_79347_3__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_8;   /* for PHI node */
    llvm_cbe_previous_callout_59381_3__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_8;   /* for PHI node */
    llvm_cbe_previous_39389_3__PHI_TEMPORARY = llvm_cbe_previous_25708_0;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_3__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_8;   /* for PHI node */
    llvm_cbe_inescq_29422_3__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_last_code_29452_3__PHI_TEMPORARY = llvm_cbe_last_code_15771_0;   /* for PHI node */
    llvm_cbe_code105_39456_3__PHI_TEMPORARY = llvm_cbe_code105_25773_0;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_3__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_8;   /* for PHI node */
    llvm_cbe_options104_39466_3__PHI_TEMPORARY = llvm_cbe_options104_39466_8;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_8;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_3__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_8;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_3__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_8;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_3__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_8;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_8;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_8;   /* for PHI node */
    llvm_cbe_greedy_default_39909_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_8;   /* for PHI node */
    llvm_cbe_tmp13512351_3__PHI_TEMPORARY = llvm_cbe_tmp13512365;   /* for PHI node */
    llvm_cbe_tmp13513612358_3__PHI_TEMPORARY = llvm_cbe_tmp13513612367;   /* for PHI node */
    goto llvm_cbe_cond_true141;
  }

llvm_cbe_cond_next233:
  llvm_cbe_tmp234 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp236 = *(&llvm_cbe_tmp234[((unsigned int )1)]);
  if ((llvm_cbe_tmp236 == ((unsigned char )69))) {
    goto llvm_cbe_cond_next241;
  } else {
    goto llvm_cbe_bb244;
  }

llvm_cbe_cond_true227:
  llvm_cbe_previous_25708_0 = llvm_cbe_previous_25708_0__PHI_TEMPORARY;
  llvm_cbe_last_code_15771_0 = llvm_cbe_last_code_15771_0__PHI_TEMPORARY;
  llvm_cbe_code105_25773_0 = llvm_cbe_code105_25773_0__PHI_TEMPORARY;
  llvm_cbe_options_addr_25789_8 = llvm_cbe_options_addr_25789_8__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_8 = llvm_cbe_save_hwm_79347_8__PHI_TEMPORARY;
  llvm_cbe_previous_callout_59381_8 = llvm_cbe_previous_callout_59381_8__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_8 = llvm_cbe_groupsetfirstbyte_29395_8__PHI_TEMPORARY;
  llvm_cbe_inescq_29422_8 = llvm_cbe_inescq_29422_8__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_79460_8 = llvm_cbe_after_manual_callout_79460_8__PHI_TEMPORARY;
  llvm_cbe_options104_39466_8 = llvm_cbe_options104_39466_8__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_8 = llvm_cbe_req_caseopt_39708_8__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_29740_8 = llvm_cbe_zerofirstbyte_29740_8__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_29762_8 = llvm_cbe_zeroreqbyte_29762_8__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_8 = llvm_cbe_reqbyte103_59784_8__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_8 = llvm_cbe_firstbyte102_39829_8__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_8 = llvm_cbe_greedy_non_default_39879_8__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_8 = llvm_cbe_greedy_default_39909_8__PHI_TEMPORARY;
  llvm_cbe_tmp13512351_8 = llvm_cbe_tmp13512351_8__PHI_TEMPORARY;
  llvm_cbe_tmp13513612358_8 = llvm_cbe_tmp13513612358_8__PHI_TEMPORARY;
  if ((llvm_cbe_tmp13512351_8 == ((unsigned char )92))) {
    goto llvm_cbe_cond_next233;
  } else {
    goto llvm_cbe_bb244;
  }

llvm_cbe_cond_next215:
  llvm_cbe_options_addr_25789_5 = llvm_cbe_options_addr_25789_5__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_5 = llvm_cbe_save_hwm_79347_5__PHI_TEMPORARY;
  llvm_cbe_previous_callout_59381_5 = llvm_cbe_previous_callout_59381_5__PHI_TEMPORARY;
  llvm_cbe_previous_39389_5 = llvm_cbe_previous_39389_5__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_5 = llvm_cbe_groupsetfirstbyte_29395_5__PHI_TEMPORARY;
  llvm_cbe_inescq_29422_5 = llvm_cbe_inescq_29422_5__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_79460_5 = llvm_cbe_after_manual_callout_79460_5__PHI_TEMPORARY;
  llvm_cbe_options104_39466_5 = llvm_cbe_options104_39466_5__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_5 = llvm_cbe_req_caseopt_39708_5__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_29740_5 = llvm_cbe_zerofirstbyte_29740_5__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_29762_5 = llvm_cbe_zeroreqbyte_29762_5__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_5 = llvm_cbe_reqbyte103_59784_5__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_5 = llvm_cbe_firstbyte102_39829_5__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_5 = llvm_cbe_greedy_non_default_39879_5__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_5 = llvm_cbe_greedy_default_39909_5__PHI_TEMPORARY;
  llvm_cbe_tmp13512351_5 = llvm_cbe_tmp13512351_5__PHI_TEMPORARY;
  llvm_cbe_tmp13513612358_5 = llvm_cbe_tmp13513612358_5__PHI_TEMPORARY;
  llvm_cbe_last_code_1 = llvm_cbe_last_code_1__PHI_TEMPORARY;
  llvm_cbe_code105_2 = llvm_cbe_code105_2__PHI_TEMPORARY;
  if (((llvm_cbe_tmp13512351_5 != ((unsigned char )0)) & (llvm_cbe_inescq_29422_5 != ((unsigned int )0)))) {
    llvm_cbe_previous_25708_0__PHI_TEMPORARY = llvm_cbe_previous_39389_5;   /* for PHI node */
    llvm_cbe_last_code_15771_0__PHI_TEMPORARY = llvm_cbe_last_code_1;   /* for PHI node */
    llvm_cbe_code105_25773_0__PHI_TEMPORARY = llvm_cbe_code105_2;   /* for PHI node */
    llvm_cbe_options_addr_25789_8__PHI_TEMPORARY = llvm_cbe_options_addr_25789_5;   /* for PHI node */
    llvm_cbe_save_hwm_79347_8__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_5;   /* for PHI node */
    llvm_cbe_previous_callout_59381_8__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_5;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_8__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_5;   /* for PHI node */
    llvm_cbe_inescq_29422_8__PHI_TEMPORARY = llvm_cbe_inescq_29422_5;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_8__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_5;   /* for PHI node */
    llvm_cbe_options104_39466_8__PHI_TEMPORARY = llvm_cbe_options104_39466_5;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_8__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_5;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_8__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_5;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_8__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_5;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_5;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_8__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_5;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_8__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_5;   /* for PHI node */
    llvm_cbe_greedy_default_39909_8__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_5;   /* for PHI node */
    llvm_cbe_tmp13512351_8__PHI_TEMPORARY = llvm_cbe_tmp13512351_5;   /* for PHI node */
    llvm_cbe_tmp13513612358_8__PHI_TEMPORARY = llvm_cbe_tmp13513612358_5;   /* for PHI node */
    goto llvm_cbe_cond_true227;
  } else {
    llvm_cbe_previous_25708_1__PHI_TEMPORARY = llvm_cbe_previous_39389_5;   /* for PHI node */
    llvm_cbe_last_code_15771_1__PHI_TEMPORARY = llvm_cbe_last_code_1;   /* for PHI node */
    llvm_cbe_code105_25773_1__PHI_TEMPORARY = llvm_cbe_code105_2;   /* for PHI node */
    llvm_cbe_options_addr_25789_9__PHI_TEMPORARY = llvm_cbe_options_addr_25789_5;   /* for PHI node */
    llvm_cbe_save_hwm_79347_9__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_5;   /* for PHI node */
    llvm_cbe_previous_callout_59381_9__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_5;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_9__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_5;   /* for PHI node */
    llvm_cbe_inescq_29422_9__PHI_TEMPORARY = llvm_cbe_inescq_29422_5;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_9__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_5;   /* for PHI node */
    llvm_cbe_options104_39466_9__PHI_TEMPORARY = llvm_cbe_options104_39466_5;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_9__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_5;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_9__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_5;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_9__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_5;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_5;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_9__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_5;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_9__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_5;   /* for PHI node */
    llvm_cbe_greedy_default_39909_9__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_5;   /* for PHI node */
    llvm_cbe_tmp13512351_9__PHI_TEMPORARY = llvm_cbe_tmp13512351_5;   /* for PHI node */
    llvm_cbe_tmp13513612358_9__PHI_TEMPORARY = llvm_cbe_tmp13513612358_5;   /* for PHI node */
    goto llvm_cbe_cond_next272;
  }

llvm_cbe_cond_false201:
  llvm_cbe_options_addr_25789_4 = llvm_cbe_options_addr_25789_4__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_4 = llvm_cbe_save_hwm_79347_4__PHI_TEMPORARY;
  llvm_cbe_previous_callout_59381_4 = llvm_cbe_previous_callout_59381_4__PHI_TEMPORARY;
  llvm_cbe_previous_39389_4 = llvm_cbe_previous_39389_4__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_4 = llvm_cbe_groupsetfirstbyte_29395_4__PHI_TEMPORARY;
  llvm_cbe_inescq_29422_4 = llvm_cbe_inescq_29422_4__PHI_TEMPORARY;
  llvm_cbe_last_code_29452_4 = llvm_cbe_last_code_29452_4__PHI_TEMPORARY;
  llvm_cbe_code105_39456_4 = llvm_cbe_code105_39456_4__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_79460_4 = llvm_cbe_after_manual_callout_79460_4__PHI_TEMPORARY;
  llvm_cbe_options104_39466_4 = llvm_cbe_options104_39466_4__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_4 = llvm_cbe_req_caseopt_39708_4__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_29740_4 = llvm_cbe_zerofirstbyte_29740_4__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_29762_4 = llvm_cbe_zeroreqbyte_29762_4__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_4 = llvm_cbe_reqbyte103_59784_4__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_4 = llvm_cbe_firstbyte102_39829_4__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_4 = llvm_cbe_greedy_non_default_39879_4__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_4 = llvm_cbe_greedy_default_39909_4__PHI_TEMPORARY;
  llvm_cbe_tmp13512351_4 = llvm_cbe_tmp13512351_4__PHI_TEMPORARY;
  llvm_cbe_tmp13513612358_4 = llvm_cbe_tmp13513612358_4__PHI_TEMPORARY;
  llvm_cbe_tmp204 = *llvm_cbe_tmp203;
  llvm_cbe_tmp207 = *llvm_cbe_tmp143;
  if ((((unsigned char *)llvm_cbe_tmp204) > ((unsigned char *)(&llvm_cbe_tmp207[((unsigned int )4096)])))) {
    goto llvm_cbe_cond_true212;
  } else {
    llvm_cbe_options_addr_25789_5__PHI_TEMPORARY = llvm_cbe_options_addr_25789_4;   /* for PHI node */
    llvm_cbe_save_hwm_79347_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_4;   /* for PHI node */
    llvm_cbe_previous_callout_59381_5__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_4;   /* for PHI node */
    llvm_cbe_previous_39389_5__PHI_TEMPORARY = llvm_cbe_previous_39389_4;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_5__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_4;   /* for PHI node */
    llvm_cbe_inescq_29422_5__PHI_TEMPORARY = llvm_cbe_inescq_29422_4;   /* for PHI node */
    llvm_cbe_after_manual_callout_79460_5__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_4;   /* for PHI node */
    llvm_cbe_options104_39466_5__PHI_TEMPORARY = llvm_cbe_options104_39466_4;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_5__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_4;   /* for PHI node */
    llvm_cbe_zerofirstbyte_29740_5__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_4;   /* for PHI node */
    llvm_cbe_zeroreqbyte_29762_5__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_4;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_4;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_5__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_4;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_5__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_4;   /* for PHI node */
    llvm_cbe_greedy_default_39909_5__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_4;   /* for PHI node */
    llvm_cbe_tmp13512351_5__PHI_TEMPORARY = llvm_cbe_tmp13512351_4;   /* for PHI node */
    llvm_cbe_tmp13513612358_5__PHI_TEMPORARY = llvm_cbe_tmp13513612358_4;   /* for PHI node */
    llvm_cbe_last_code_1__PHI_TEMPORARY = llvm_cbe_last_code_29452_4;   /* for PHI node */
    llvm_cbe_code105_2__PHI_TEMPORARY = llvm_cbe_code105_39456_4;   /* for PHI node */
    goto llvm_cbe_cond_next215;
  }

llvm_cbe_bb326:
  llvm_cbe_p_10_lcssa = llvm_cbe_p_10_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp328 = *llvm_cbe_p_10_lcssa;
  switch (llvm_cbe_tmp328) {
  default:
    goto llvm_cbe_bb395;
;
  case ((unsigned char )125):
    llvm_cbe_iftmp_136_0__PHI_TEMPORARY = 0;   /* for PHI node */
    goto llvm_cbe_bb396;
  case ((unsigned char )44):
    goto llvm_cbe_cond_next344;
    break;
  }
llvm_cbe_bb315_preheader:
  llvm_cbe_tmp31726562 = *llvm_cbe_tmp307;
  llvm_cbe_tmp32026565 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp31726562))]);
  if (((((unsigned char )(llvm_cbe_tmp32026565 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p_10_lcssa__PHI_TEMPORARY = llvm_cbe_tmp307;   /* for PHI node */
    goto llvm_cbe_bb326;
  } else {
    llvm_cbe_p_1026561_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb312;
  }

llvm_cbe_cond_next292:
  llvm_cbe_tmp293 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp297 = *(&llvm_cbe_tmp293[((unsigned int )1)]);
  llvm_cbe_tmp300 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp297))]);
  llvm_cbe_tmp307 = &llvm_cbe_tmp293[((unsigned int )2)];
  if (((((unsigned char )(llvm_cbe_tmp300 & ((unsigned char )4)))) == ((unsigned char )0))) {
    goto llvm_cbe_bb395;
  } else {
    goto llvm_cbe_bb315_preheader;
  }

llvm_cbe_cond_next286:
  if ((llvm_cbe_tmp13512351_9 == ((unsigned char )123))) {
    goto llvm_cbe_cond_next292;
  } else {
    goto llvm_cbe_bb395;
  }

  do {     /* Syntactic loop 'bb312' to make GCC happy */
llvm_cbe_bb312:
  llvm_cbe_p_1026561_rec = llvm_cbe_p_1026561_rec__PHI_TEMPORARY;
  llvm_cbe_tmp314 = &llvm_cbe_tmp293[(llvm_cbe_p_1026561_rec + ((unsigned int )3))];
  llvm_cbe_tmp317 = *llvm_cbe_tmp314;
  llvm_cbe_tmp320 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp317))]);
  if (((((unsigned char )(llvm_cbe_tmp320 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p_10_lcssa__PHI_TEMPORARY = llvm_cbe_tmp314;   /* for PHI node */
    goto llvm_cbe_bb326;
  } else {
    llvm_cbe_p_1026561_rec__PHI_TEMPORARY = (llvm_cbe_p_1026561_rec + ((unsigned int )1));   /* for PHI node */
    goto llvm_cbe_bb312;
  }

  } while (1); /* end of syntactic loop 'bb312' */
llvm_cbe_cond_next344:
  llvm_cbe_tmp346 = *(&llvm_cbe_p_10_lcssa[((unsigned int )1)]);
  if ((llvm_cbe_tmp346 == ((unsigned char )125))) {
    llvm_cbe_iftmp_136_0__PHI_TEMPORARY = 0;   /* for PHI node */
    goto llvm_cbe_bb396;
  } else {
    goto llvm_cbe_cond_next352;
  }

llvm_cbe_bb383:
  llvm_cbe_p_12_lcssa = llvm_cbe_p_12_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp385 = *llvm_cbe_p_12_lcssa;
  if ((llvm_cbe_tmp385 == ((unsigned char )125))) {
    llvm_cbe_iftmp_136_0__PHI_TEMPORARY = 0;   /* for PHI node */
    goto llvm_cbe_bb396;
  } else {
    goto llvm_cbe_bb395;
  }

llvm_cbe_bb372_preheader:
  llvm_cbe_tmp37426570 = *llvm_cbe_tmp364;
  llvm_cbe_tmp37726573 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp37426570))]);
  if (((((unsigned char )(llvm_cbe_tmp37726573 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p_12_lcssa__PHI_TEMPORARY = llvm_cbe_tmp364;   /* for PHI node */
    goto llvm_cbe_bb383;
  } else {
    llvm_cbe_p_1226569_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb369;
  }

llvm_cbe_cond_next352:
  llvm_cbe_tmp357 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp346))]);
  llvm_cbe_tmp364 = &llvm_cbe_p_10_lcssa[((unsigned int )2)];
  if (((((unsigned char )(llvm_cbe_tmp357 & ((unsigned char )4)))) == ((unsigned char )0))) {
    goto llvm_cbe_bb395;
  } else {
    goto llvm_cbe_bb372_preheader;
  }

  do {     /* Syntactic loop 'bb369' to make GCC happy */
llvm_cbe_bb369:
  llvm_cbe_p_1226569_rec = llvm_cbe_p_1226569_rec__PHI_TEMPORARY;
  llvm_cbe_tmp371 = &llvm_cbe_p_10_lcssa[(llvm_cbe_p_1226569_rec + ((unsigned int )3))];
  llvm_cbe_tmp374 = *llvm_cbe_tmp371;
  llvm_cbe_tmp377 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp374))]);
  if (((((unsigned char )(llvm_cbe_tmp377 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p_12_lcssa__PHI_TEMPORARY = llvm_cbe_tmp371;   /* for PHI node */
    goto llvm_cbe_bb383;
  } else {
    llvm_cbe_p_1226569_rec__PHI_TEMPORARY = (llvm_cbe_p_1226569_rec + ((unsigned int )1));   /* for PHI node */
    goto llvm_cbe_bb369;
  }

  } while (1); /* end of syntactic loop 'bb369' */
llvm_cbe_bb395:
  llvm_cbe_iftmp_136_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb396;

llvm_cbe_cond_true409:
  llvm_cbe_tmp414 = llvm_cbe_after_manual_callout_79460_9 + ((unsigned int )-1);
  llvm_cbe_previous_callout_59381_9_mux = (((((signed int )llvm_cbe_after_manual_callout_79460_9) < ((signed int )((unsigned int )1)))) ? (((unsigned char *)/*NULL*/0)) : (llvm_cbe_previous_callout_59381_9));
  if (((((signed int )llvm_cbe_after_manual_callout_79460_9) < ((signed int )((unsigned int )1))) & (llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0)))) {
    goto llvm_cbe_cond_true421;
  } else {
    llvm_cbe_previous_callout_6__PHI_TEMPORARY = llvm_cbe_previous_callout_59381_9_mux;   /* for PHI node */
    llvm_cbe_after_manual_callout_1__PHI_TEMPORARY = llvm_cbe_tmp414;   /* for PHI node */
    goto llvm_cbe_cond_next427;
  }

llvm_cbe_cond_true421:
  llvm_cbe_tmp422 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4_i9 = *llvm_cbe_tmp3783;
  llvm_cbe_tmp9_i13 = *(&llvm_cbe_previous_callout_59381_9[((unsigned int )2)]);
  llvm_cbe_tmp14_i17 = *(&llvm_cbe_previous_callout_59381_9[((unsigned int )3)]);
  llvm_cbe_tmp17_i20 = ((((unsigned int )(unsigned long)llvm_cbe_tmp422)) - (((unsigned int )(unsigned long)llvm_cbe_tmp4_i9))) - (((((unsigned int )(unsigned char )llvm_cbe_tmp9_i13)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp14_i17)));
  *(&llvm_cbe_previous_callout_59381_9[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp17_i20) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_previous_callout_59381_9[((unsigned int )5)]) = (((unsigned char )llvm_cbe_tmp17_i20));
  llvm_cbe_previous_callout_6__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_after_manual_callout_1__PHI_TEMPORARY = llvm_cbe_tmp414;   /* for PHI node */
  goto llvm_cbe_cond_next427;

llvm_cbe_cond_next445:
  if ((llvm_cbe_tmp13512351_9 == ((unsigned char )35))) {
    goto llvm_cbe_bb549;
  } else {
    llvm_cbe_c_2__PHI_TEMPORARY = llvm_cbe_tmp13513612358_9;   /* for PHI node */
    goto llvm_cbe_cond_next566;
  }

llvm_cbe_cond_true433:
  llvm_cbe_tmp436 = *llvm_cbe_tmp435;
  llvm_cbe_tmp439 = *(&llvm_cbe_tmp436[llvm_cbe_tmp13513612358_9]);
  if (((((unsigned char )(llvm_cbe_tmp439 & ((unsigned char )1)))) == ((unsigned char )0))) {
    goto llvm_cbe_cond_next445;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_6;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_25773_1;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_bb557:
  llvm_cbe_tmp558 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp559 = *llvm_cbe_tmp558;
  if ((llvm_cbe_tmp559 == ((unsigned char )0))) {
    llvm_cbe_c_2__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_next566;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_6;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_25773_1;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_cond_true541:
  llvm_cbe_tmp542 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp545 = *llvm_cbe_tmp469;
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp542[(llvm_cbe_tmp545 + ((unsigned int )-1))]);
  goto llvm_cbe_bb557;

  do {     /* Syntactic loop 'bb549' to make GCC happy */
llvm_cbe_bb549:
  llvm_cbe_tmp550 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp551 = &llvm_cbe_tmp550[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp551;
  llvm_cbe_tmp553 = *llvm_cbe_tmp551;
  if ((llvm_cbe_tmp553 == ((unsigned char )0))) {
    goto llvm_cbe_bb557;
  } else {
    goto llvm_cbe_bb451;
  }

llvm_cbe_cond_true458:
  if ((((unsigned char *)llvm_cbe_tmp461) > ((unsigned char *)llvm_cbe_tmp551))) {
    goto llvm_cbe_cond_next467;
  } else {
    goto llvm_cbe_bb549;
  }

llvm_cbe_bb451:
  llvm_cbe_tmp454 = *llvm_cbe_tmp453;
  llvm_cbe_tmp461 = *llvm_cbe_tmp460;
  if ((llvm_cbe_tmp454 == ((unsigned int )0))) {
    goto llvm_cbe_cond_false488;
  } else {
    goto llvm_cbe_cond_true458;
  }

llvm_cbe_cond_next467:
  llvm_cbe_tmp478 = _pcre_is_newline(llvm_cbe_tmp551, llvm_cbe_tmp454, llvm_cbe_tmp461, llvm_cbe_tmp469);
  if ((llvm_cbe_tmp478 == ((unsigned int )0))) {
    goto llvm_cbe_bb549;
  } else {
    goto llvm_cbe_cond_true541;
  }

llvm_cbe_cond_false488:
  llvm_cbe_tmp494 = *llvm_cbe_tmp469;
  if ((((unsigned char *)(&llvm_cbe_tmp461[(-(llvm_cbe_tmp494))])) < ((unsigned char *)llvm_cbe_tmp551))) {
    goto llvm_cbe_bb549;
  } else {
    goto llvm_cbe_cond_next502;
  }

llvm_cbe_cond_next502:
  llvm_cbe_tmp509 = *llvm_cbe_tmp508;
  if ((llvm_cbe_tmp553 == llvm_cbe_tmp509)) {
    goto llvm_cbe_cond_next514;
  } else {
    goto llvm_cbe_bb549;
  }

llvm_cbe_cond_next522:
  llvm_cbe_tmp525 = *(&llvm_cbe_tmp550[((unsigned int )2)]);
  llvm_cbe_tmp529 = *llvm_cbe_tmp528;
  if ((llvm_cbe_tmp525 == llvm_cbe_tmp529)) {
    goto llvm_cbe_cond_true541;
  } else {
    goto llvm_cbe_bb549;
  }

llvm_cbe_cond_next514:
  if ((llvm_cbe_tmp494 == ((unsigned int )1))) {
    goto llvm_cbe_cond_true541;
  } else {
    goto llvm_cbe_cond_next522;
  }

  } while (1); /* end of syntactic loop 'bb549' */
llvm_cbe_cond_true580:
  llvm_cbe_tmp582 = *(&llvm_cbe_ptr106);
  *llvm_cbe_code105_25773_1 = ((unsigned char )82);
  *(&llvm_cbe_code105_25773_1[((unsigned int )1)]) = ((unsigned char )-1);
  llvm_cbe_tmp11_i30 = *llvm_cbe_tmp3783;
  *(&llvm_cbe_code105_25773_1[((unsigned int )2)]) = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)llvm_cbe_tmp582)) - (((unsigned int )(unsigned long)llvm_cbe_tmp11_i30)))) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp23_i36 = *llvm_cbe_tmp3783;
  *(&llvm_cbe_code105_25773_1[((unsigned int )3)]) = (((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_tmp582)) - (((unsigned char )(unsigned long)llvm_cbe_tmp23_i36)))));
  *(&llvm_cbe_code105_25773_1[((unsigned int )4)]) = ((unsigned char )0);
  *(&llvm_cbe_code105_25773_1[((unsigned int )5)]) = ((unsigned char )0);
  llvm_cbe_tmp35_i42 = &llvm_cbe_code105_25773_1[((unsigned int )6)];
  llvm_cbe_previous_callout_9__PHI_TEMPORARY = llvm_cbe_code105_25773_1;   /* for PHI node */
  llvm_cbe_code105_10__PHI_TEMPORARY = llvm_cbe_tmp35_i42;   /* for PHI node */
  goto llvm_cbe_cond_next586;

llvm_cbe_bb632:
  *llvm_cbe_code105_10 = ((unsigned char )26);
  llvm_cbe_tmp635 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_tmp519612384 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519612384[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp635;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_bb636:
  llvm_cbe_firstbyte102_4 = (((llvm_cbe_firstbyte102_39829_9 == ((unsigned int )-2))) ? (((unsigned int )-1)) : (llvm_cbe_firstbyte102_39829_9));
  *llvm_cbe_code105_10 = ((unsigned char )12);
  llvm_cbe_tmp648 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_tmp519612387 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519612387[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp648;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_4;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_4;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_true1736:
  llvm_cbe_firstbyte102_5 = (((llvm_cbe_firstbyte102_39829_9 == ((unsigned int )-2))) ? (((unsigned int )-1)) : (llvm_cbe_firstbyte102_39829_9));
  *llvm_cbe_code105_10 = ((unsigned char )29);
  *(&llvm_cbe_code105_10[((unsigned int )1)]) = (((unsigned char )llvm_cbe_class_lastchar_8));
  llvm_cbe_tmp1751 = &llvm_cbe_code105_10[((unsigned int )2)];
  llvm_cbe_tmp519612533 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519612533[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_5;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp1751;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_5;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_5;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_true1730:
  if ((llvm_cbe_negate_class_0 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next1752;
  } else {
    goto llvm_cbe_cond_true1736;
  }

llvm_cbe_cond_next1725:
  if ((llvm_cbe_class_charcount_8 == ((unsigned int )1))) {
    goto llvm_cbe_cond_true1730;
  } else {
    goto llvm_cbe_cond_next1756;
  }

llvm_cbe_cond_next1718:
  llvm_cbe_class_lastchar_8 = llvm_cbe_class_lastchar_8__PHI_TEMPORARY;
  llvm_cbe_class_charcount_8 = llvm_cbe_class_charcount_8__PHI_TEMPORARY;
  llvm_cbe_inescq_5 = llvm_cbe_inescq_5__PHI_TEMPORARY;
  llvm_cbe_c_12 = llvm_cbe_c_12__PHI_TEMPORARY;
  if ((llvm_cbe_c_12 == ((unsigned int )0))) {
    goto llvm_cbe_cond_true1723;
  } else {
    goto llvm_cbe_cond_next1725;
  }

llvm_cbe_cond_next710:
  llvm_cbe_negate_class_0 = llvm_cbe_negate_class_0__PHI_TEMPORARY;
  llvm_cbe_c_8_in = llvm_cbe_c_8_in__PHI_TEMPORARY;
  llvm_cbe_c_8 = ((unsigned int )(unsigned char )llvm_cbe_c_8_in);
  ltmp_5_1 = memset(llvm_cbe_classbits711, (((unsigned int )(unsigned char )((unsigned char )0))), ((unsigned int )32));
  if ((llvm_cbe_c_8_in == ((unsigned char )0))) {
    llvm_cbe_class_lastchar_8__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_class_charcount_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_inescq_5__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_c_12__PHI_TEMPORARY = llvm_cbe_c_8;   /* for PHI node */
    goto llvm_cbe_cond_next1718;
  } else {
    goto llvm_cbe_bb717_preheader;
  }

llvm_cbe_bb693:
  llvm_cbe_tmp694 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp695 = &llvm_cbe_tmp694[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp695;
  llvm_cbe_tmp697 = *llvm_cbe_tmp695;
  if ((llvm_cbe_tmp697 == ((unsigned char )94))) {
    goto llvm_cbe_cond_true703;
  } else {
    llvm_cbe_negate_class_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_c_8_in__PHI_TEMPORARY = llvm_cbe_tmp697;   /* for PHI node */
    goto llvm_cbe_cond_next710;
  }

llvm_cbe_bb649:
  llvm_cbe_tmp651 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp653 = *(&llvm_cbe_tmp651[((unsigned int )1)]);
  switch (llvm_cbe_tmp653) {
  default:
    goto llvm_cbe_bb693;
;
  case ((unsigned char )46):
    goto llvm_cbe_bb673;
    break;
  case ((unsigned char )58):
    goto llvm_cbe_bb673;
    break;
  case ((unsigned char )61):
    goto llvm_cbe_bb673;
    break;
  }
llvm_cbe_bb673:
  llvm_cbe_tmp676 = check_posix_syntax(llvm_cbe_tmp651, (&llvm_cbe_tempptr), llvm_cbe_cd);
  if ((llvm_cbe_tmp676 == ((unsigned int )0))) {
    goto llvm_cbe_bb693;
  } else {
    goto llvm_cbe_cond_true680;
  }

llvm_cbe_cond_true703:
  llvm_cbe_tmp705 = &llvm_cbe_tmp694[((unsigned int )2)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp705;
  llvm_cbe_tmp707 = *llvm_cbe_tmp705;
  llvm_cbe_negate_class_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  llvm_cbe_c_8_in__PHI_TEMPORARY = llvm_cbe_tmp707;   /* for PHI node */
  goto llvm_cbe_cond_next710;

  do {     /* Syntactic loop 'bb717' to make GCC happy */
llvm_cbe_bb717:
  llvm_cbe_class_lastchar_7 = llvm_cbe_class_lastchar_7__PHI_TEMPORARY;
  llvm_cbe_class_charcount_7 = llvm_cbe_class_charcount_7__PHI_TEMPORARY;
  llvm_cbe_inescq_4 = llvm_cbe_inescq_4__PHI_TEMPORARY;
  llvm_cbe_c_11 = llvm_cbe_c_11__PHI_TEMPORARY;
  if ((llvm_cbe_inescq_4 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next739;
  } else {
    goto llvm_cbe_cond_true722;
  }

llvm_cbe_cond_next1705:
  if (((llvm_cbe_tmp1698 != ((unsigned char )93)) | (llvm_cbe_inescq_3 != ((unsigned int )0)))) {
    llvm_cbe_class_lastchar_7__PHI_TEMPORARY = llvm_cbe_class_lastchar_6;   /* for PHI node */
    llvm_cbe_class_charcount_7__PHI_TEMPORARY = llvm_cbe_class_charcount_6;   /* for PHI node */
    llvm_cbe_inescq_4__PHI_TEMPORARY = llvm_cbe_inescq_3;   /* for PHI node */
    llvm_cbe_c_11__PHI_TEMPORARY = llvm_cbe_tmp16981699;   /* for PHI node */
    goto llvm_cbe_bb717;
  } else {
    llvm_cbe_class_lastchar_8__PHI_TEMPORARY = llvm_cbe_class_lastchar_6;   /* for PHI node */
    llvm_cbe_class_charcount_8__PHI_TEMPORARY = llvm_cbe_class_charcount_6;   /* for PHI node */
    llvm_cbe_inescq_5__PHI_TEMPORARY = llvm_cbe_inescq_3;   /* for PHI node */
    llvm_cbe_c_12__PHI_TEMPORARY = llvm_cbe_tmp16981699;   /* for PHI node */
    goto llvm_cbe_cond_next1718;
  }

llvm_cbe_bb1694:
  llvm_cbe_class_lastchar_6 = llvm_cbe_class_lastchar_6__PHI_TEMPORARY;
  llvm_cbe_class_charcount_6 = llvm_cbe_class_charcount_6__PHI_TEMPORARY;
  llvm_cbe_inescq_3 = llvm_cbe_inescq_3__PHI_TEMPORARY;
  llvm_cbe_tmp1695 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1696 = &llvm_cbe_tmp1695[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1696;
  llvm_cbe_tmp1698 = *llvm_cbe_tmp1696;
  llvm_cbe_tmp16981699 = ((unsigned int )(unsigned char )llvm_cbe_tmp1698);
  if ((llvm_cbe_tmp1698 == ((unsigned char )0))) {
    llvm_cbe_class_lastchar_8__PHI_TEMPORARY = llvm_cbe_class_lastchar_6;   /* for PHI node */
    llvm_cbe_class_charcount_8__PHI_TEMPORARY = llvm_cbe_class_charcount_6;   /* for PHI node */
    llvm_cbe_inescq_5__PHI_TEMPORARY = llvm_cbe_inescq_3;   /* for PHI node */
    llvm_cbe_c_12__PHI_TEMPORARY = llvm_cbe_tmp16981699;   /* for PHI node */
    goto llvm_cbe_cond_next1718;
  } else {
    goto llvm_cbe_cond_next1705;
  }

llvm_cbe_cond_true734:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp729;
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_class_charcount_7;   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb1694;

llvm_cbe_cond_true727:
  llvm_cbe_tmp728 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp729 = &llvm_cbe_tmp728[((unsigned int )1)];
  llvm_cbe_tmp730 = *llvm_cbe_tmp729;
  if ((llvm_cbe_tmp730 == ((unsigned char )69))) {
    goto llvm_cbe_cond_true734;
  } else {
    llvm_cbe_inescq_9__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_18__PHI_TEMPORARY = llvm_cbe_c_11;   /* for PHI node */
    goto llvm_cbe_bb1415;
  }

llvm_cbe_cond_true722:
  if ((llvm_cbe_c_11 == ((unsigned int )92))) {
    goto llvm_cbe_cond_true727;
  } else {
    llvm_cbe_inescq_9__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_18__PHI_TEMPORARY = llvm_cbe_c_11;   /* for PHI node */
    goto llvm_cbe_bb1415;
  }

llvm_cbe_cond_next1020:
  llvm_cbe_tmp1021 = *(&llvm_cbe_tempptr);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp1021[((unsigned int )1)]);
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = ((unsigned int )10);   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
  goto llvm_cbe_bb1694;

  do {     /* Syntactic loop 'bb983' to make GCC happy */
llvm_cbe_bb983:
  llvm_cbe_c_1512416_13 = llvm_cbe_c_1512416_13__PHI_TEMPORARY;
  llvm_cbe_tmp986 = &llvm_cbe_classbits[llvm_cbe_c_1512416_13];
  llvm_cbe_tmp987 = *llvm_cbe_tmp986;
  llvm_cbe_tmp990 = *(&llvm_cbe_pbits[llvm_cbe_c_1512416_13]);
  *llvm_cbe_tmp986 = (((unsigned char )(llvm_cbe_tmp987 | (((unsigned char )(llvm_cbe_tmp990 ^ ((unsigned char )-1)))))));
  llvm_cbe_indvar_next26683 = llvm_cbe_c_1512416_13 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26683 == ((unsigned int )32))) {
    goto llvm_cbe_cond_next1020;
  } else {
    llvm_cbe_c_1512416_13__PHI_TEMPORARY = llvm_cbe_indvar_next26683;   /* for PHI node */
    goto llvm_cbe_bb983;
  }

  } while (1); /* end of syntactic loop 'bb983' */
llvm_cbe_cond_true961:
  llvm_cbe_tmp963 = *llvm_cbe_tmp962;
  *llvm_cbe_tmp962 = (((unsigned char )(llvm_cbe_tmp963 & ((unsigned char )-61))));
  if (llvm_cbe_local_negate_9) {
    llvm_cbe_c_1726583__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1002;
  } else {
    llvm_cbe_c_1512416_13__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb983;
  }

llvm_cbe_cond_next948:
  switch (((((((signed int )llvm_cbe_tmp893) < ((signed int )((unsigned int )0)))) ? ((-(llvm_cbe_tmp893))) : (llvm_cbe_tmp893)))) {
  default:
    goto llvm_cbe_cond_next977;
;
  case ((unsigned int )1):
    goto llvm_cbe_cond_true961;
    break;
  case ((unsigned int )2):
    goto llvm_cbe_cond_true971;
  }
llvm_cbe_cond_next863:
  llvm_cbe_tmp879 = ((((((signed int )llvm_cbe_tmp88_0) < ((signed int )((unsigned int )3))) & (llvm_cbe_tmp865 != ((unsigned int )0)))) ? (((unsigned int )0)) : ((llvm_cbe_tmp88_0 * ((unsigned int )3))));
  llvm_cbe_tmp882 = *(&posix_class_maps[llvm_cbe_tmp879]);
  ltmp_6_1 = memcpy(llvm_cbe_pbits885, (&llvm_cbe_tmp777[llvm_cbe_tmp882]), ((unsigned int )32));
  llvm_cbe_tmp889 = *(&posix_class_maps[(llvm_cbe_tmp879 + ((unsigned int )1))]);
  llvm_cbe_tmp893 = *(&posix_class_maps[(llvm_cbe_tmp879 + ((unsigned int )2))]);
  if ((((signed int )llvm_cbe_tmp889) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true898;
  } else {
    goto llvm_cbe_cond_next948;
  }

llvm_cbe_bb855:
  llvm_cbe_tmp88_0 = llvm_cbe_tmp88_0__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_tmp88_0) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true861;
  } else {
    goto llvm_cbe_cond_next863;
  }

  do {     /* Syntactic loop 'bb807' to make GCC happy */
llvm_cbe_bb807:
  llvm_cbe_yield_912392_0 = llvm_cbe_yield_912392_0__PHI_TEMPORARY;
  llvm_cbe_tmp810 = *(&posix_name_lengths[llvm_cbe_yield_912392_0]);
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp810)) == llvm_cbe_tmp802)) {
    goto llvm_cbe_cond_false827;
  } else {
    goto llvm_cbe_cond_next843;
  }

llvm_cbe_cond_next843:
  llvm_cbe_tmp845 = llvm_cbe_yield_912392_0 + ((unsigned int )1);
  llvm_cbe_tmp849 = *(&posix_name_lengths[llvm_cbe_tmp845]);
  if ((llvm_cbe_tmp849 == ((unsigned char )0))) {
    llvm_cbe_tmp88_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_bb855;
  } else {
    llvm_cbe_yield_912392_0__PHI_TEMPORARY = llvm_cbe_tmp845;   /* for PHI node */
    goto llvm_cbe_bb807;
  }

llvm_cbe_cond_false827:
  llvm_cbe_tmp831 = *(&posix_names[llvm_cbe_yield_912392_0]);
  llvm_cbe_tmp833 = strncmp(llvm_cbe_tmp800, llvm_cbe_tmp831, llvm_cbe_tmp802);
  if ((llvm_cbe_tmp833 == ((unsigned int )0))) {
    llvm_cbe_tmp88_0__PHI_TEMPORARY = llvm_cbe_yield_912392_0;   /* for PHI node */
    goto llvm_cbe_bb855;
  } else {
    goto llvm_cbe_cond_next843;
  }

  } while (1); /* end of syntactic loop 'bb807' */
llvm_cbe_cond_next797:
  llvm_cbe_local_negate_9 = llvm_cbe_local_negate_9__PHI_TEMPORARY;
  llvm_cbe_tmp798 = *(&llvm_cbe_tempptr);
  llvm_cbe_tmp800 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp802 = (((unsigned int )(unsigned long)llvm_cbe_tmp798)) - (((unsigned int )(unsigned long)llvm_cbe_tmp800));
  llvm_cbe_yield_912392_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb807;

llvm_cbe_cond_next786:
  llvm_cbe_tmp788 = &llvm_cbe_tmp778[((unsigned int )2)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp788;
  llvm_cbe_tmp790 = *llvm_cbe_tmp788;
  if ((llvm_cbe_tmp790 == ((unsigned char )94))) {
    goto llvm_cbe_cond_true794;
  } else {
    llvm_cbe_local_negate_9__PHI_TEMPORARY = 1;   /* for PHI node */
    goto llvm_cbe_cond_next797;
  }

llvm_cbe_cond_true774:
  llvm_cbe_tmp777 = *llvm_cbe_tmp776;
  llvm_cbe_tmp778 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp780 = *(&llvm_cbe_tmp778[((unsigned int )1)]);
  if ((llvm_cbe_tmp780 == ((unsigned char )58))) {
    goto llvm_cbe_cond_next786;
  } else {
    goto llvm_cbe_cond_true784;
  }

llvm_cbe_bb767:
  llvm_cbe_tmp770 = check_posix_syntax(llvm_cbe_tmp745, (&llvm_cbe_tempptr), llvm_cbe_cd);
  if ((llvm_cbe_tmp770 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next1025;
  } else {
    goto llvm_cbe_cond_true774;
  }

llvm_cbe_cond_true744:
  llvm_cbe_tmp745 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp747 = *(&llvm_cbe_tmp745[((unsigned int )1)]);
  switch (llvm_cbe_tmp747) {
  default:
    goto llvm_cbe_cond_next1025;
;
  case ((unsigned char )46):
    goto llvm_cbe_bb767;
    break;
  case ((unsigned char )58):
    goto llvm_cbe_bb767;
    break;
  case ((unsigned char )61):
    goto llvm_cbe_bb767;
    break;
  }
llvm_cbe_cond_next739:
  switch (llvm_cbe_c_11) {
  default:
    llvm_cbe_inescq_9__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_18__PHI_TEMPORARY = llvm_cbe_c_11;   /* for PHI node */
    goto llvm_cbe_bb1415;
;
  case ((unsigned int )91):
    goto llvm_cbe_cond_true744;
    break;
  case ((unsigned int )92):
    goto llvm_cbe_cond_true1030;
  }
llvm_cbe_cond_true794:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp778[((unsigned int )3)]);
  llvm_cbe_local_negate_9__PHI_TEMPORARY = 0;   /* for PHI node */
  goto llvm_cbe_cond_next797;

  do {     /* Syntactic loop 'bb904' to make GCC happy */
llvm_cbe_bb904:
  llvm_cbe_c_1312408_13 = llvm_cbe_c_1312408_13__PHI_TEMPORARY;
  llvm_cbe_tmp907 = &llvm_cbe_pbits[llvm_cbe_c_1312408_13];
  llvm_cbe_tmp908 = *llvm_cbe_tmp907;
  llvm_cbe_tmp914 = *(&llvm_cbe_tmp777[(llvm_cbe_c_1312408_13 + llvm_cbe_tmp889)]);
  *llvm_cbe_tmp907 = (((unsigned char )(llvm_cbe_tmp914 | llvm_cbe_tmp908)));
  llvm_cbe_indvar_next26678 = llvm_cbe_c_1312408_13 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26678 == ((unsigned int )32))) {
    goto llvm_cbe_cond_next948;
  } else {
    llvm_cbe_c_1312408_13__PHI_TEMPORARY = llvm_cbe_indvar_next26678;   /* for PHI node */
    goto llvm_cbe_bb904;
  }

  } while (1); /* end of syntactic loop 'bb904' */
llvm_cbe_cond_true898:
  if ((((signed int )llvm_cbe_tmp893) > ((signed int )((unsigned int )-1)))) {
    llvm_cbe_c_1312408_13__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb904;
  } else {
    llvm_cbe_c_1426586__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb926;
  }

  do {     /* Syntactic loop 'bb926' to make GCC happy */
llvm_cbe_bb926:
  llvm_cbe_c_1426586 = llvm_cbe_c_1426586__PHI_TEMPORARY;
  llvm_cbe_tmp929 = &llvm_cbe_pbits[llvm_cbe_c_1426586];
  llvm_cbe_tmp930 = *llvm_cbe_tmp929;
  llvm_cbe_tmp936 = *(&llvm_cbe_tmp777[(llvm_cbe_c_1426586 + llvm_cbe_tmp889)]);
  *llvm_cbe_tmp929 = (((unsigned char )(llvm_cbe_tmp930 & (((unsigned char )(llvm_cbe_tmp936 ^ ((unsigned char )-1)))))));
  llvm_cbe_tmp940 = llvm_cbe_c_1426586 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp940) < ((signed int )((unsigned int )32)))) {
    llvm_cbe_c_1426586__PHI_TEMPORARY = llvm_cbe_tmp940;   /* for PHI node */
    goto llvm_cbe_bb926;
  } else {
    goto llvm_cbe_cond_next948;
  }

  } while (1); /* end of syntactic loop 'bb926' */
llvm_cbe_cond_true971:
  llvm_cbe_tmp973 = *llvm_cbe_tmp972;
  *llvm_cbe_tmp972 = (((unsigned char )(llvm_cbe_tmp973 & ((unsigned char )127))));
  if (llvm_cbe_local_negate_9) {
    llvm_cbe_c_1726583__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1002;
  } else {
    llvm_cbe_c_1512416_13__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb983;
  }

llvm_cbe_cond_next977:
  if (llvm_cbe_local_negate_9) {
    llvm_cbe_c_1726583__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1002;
  } else {
    llvm_cbe_c_1512416_13__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb983;
  }

  do {     /* Syntactic loop 'bb1002' to make GCC happy */
llvm_cbe_bb1002:
  llvm_cbe_c_1726583 = llvm_cbe_c_1726583__PHI_TEMPORARY;
  llvm_cbe_tmp1005 = &llvm_cbe_classbits[llvm_cbe_c_1726583];
  llvm_cbe_tmp1006 = *llvm_cbe_tmp1005;
  llvm_cbe_tmp1009 = *(&llvm_cbe_pbits[llvm_cbe_c_1726583]);
  *llvm_cbe_tmp1005 = (((unsigned char )(llvm_cbe_tmp1009 | llvm_cbe_tmp1006)));
  llvm_cbe_tmp1013 = llvm_cbe_c_1726583 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp1013) < ((signed int )((unsigned int )32)))) {
    llvm_cbe_c_1726583__PHI_TEMPORARY = llvm_cbe_tmp1013;   /* for PHI node */
    goto llvm_cbe_bb1002;
  } else {
    goto llvm_cbe_cond_next1020;
  }

  } while (1); /* end of syntactic loop 'bb1002' */
llvm_cbe_cond_true1066:
  llvm_cbe_tmp1067 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1069 = *(&llvm_cbe_tmp1067[((unsigned int )1)]);
  if ((llvm_cbe_tmp1069 == ((unsigned char )92))) {
    goto llvm_cbe_cond_next1074;
  } else {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_class_charcount_7;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_bb1694;
  }

llvm_cbe_cond_next1043:
  switch (llvm_cbe_tmp1036) {
  default:
    goto llvm_cbe_cond_false1061_cond_next1090_crit_edge;
;
  case ((unsigned int )-5):
    llvm_cbe_c_19__PHI_TEMPORARY = ((unsigned int )8);   /* for PHI node */
    goto llvm_cbe_cond_next1090;
  case ((unsigned int )-21):
    llvm_cbe_inescq_9__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_18__PHI_TEMPORARY = ((unsigned int )88);   /* for PHI node */
    goto llvm_cbe_bb1415;
  case ((unsigned int )-16):
    goto llvm_cbe_cond_true1060;
    break;
  case ((unsigned int )-25):
    goto llvm_cbe_cond_true1066;
  }
llvm_cbe_cond_true1030:
  llvm_cbe_tmp1033 = *llvm_cbe_tmp24;
  llvm_cbe_tmp1036 = check_escape((&llvm_cbe_ptr106), llvm_cbe_errorcodeptr, llvm_cbe_tmp1033, llvm_cbe_options104_39466_9, ((unsigned int )1));
  llvm_cbe_tmp1038 = *llvm_cbe_errorcodeptr;
  if ((llvm_cbe_tmp1038 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next1043;
  } else {
    llvm_cbe_slot_913437_0_us61_12__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_12__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    goto llvm_cbe_FAILED;
  }

llvm_cbe_cond_next1025:
  if ((llvm_cbe_c_11 == ((unsigned int )92))) {
    goto llvm_cbe_cond_true1030;
  } else {
    llvm_cbe_inescq_9__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_18__PHI_TEMPORARY = llvm_cbe_c_11;   /* for PHI node */
    goto llvm_cbe_bb1415;
  }

llvm_cbe_cond_next1074:
  llvm_cbe_tmp1076 = &llvm_cbe_tmp1067[((unsigned int )2)];
  llvm_cbe_tmp1077 = *llvm_cbe_tmp1076;
  if ((llvm_cbe_tmp1077 == ((unsigned char )69))) {
    goto llvm_cbe_cond_next1082;
  } else {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_class_charcount_7;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_bb1694;
  }

llvm_cbe_cond_next1082:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1076;
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_class_charcount_7;   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
  goto llvm_cbe_bb1694;

llvm_cbe_cond_true1106:
  switch ((-(llvm_cbe_c_19))) {
  default:
    goto llvm_cbe_cond_next1278;
;
  case ((unsigned int )6):
    llvm_cbe_c_2112444_12__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1131;
  case ((unsigned int )7):
    llvm_cbe_c_2012437_12__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1110;
    break;
  case ((unsigned int )8):
    llvm_cbe_c_2526577__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1218;
  case ((unsigned int )9):
    llvm_cbe_c_2426580__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1194;
  case ((unsigned int )10):
    llvm_cbe_c_2312458_12__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1173;
  case ((unsigned int )11):
    llvm_cbe_c_2212451_12__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1152;
  case ((unsigned int )24):
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  }
llvm_cbe_cond_true1095:
  llvm_cbe_tmp1099 = *llvm_cbe_tmp776;
  llvm_cbe_tmp1101 = llvm_cbe_class_charcount_7 + ((unsigned int )2);
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_true1106;
  } else {
    goto llvm_cbe_cond_false1244;
  }

llvm_cbe_cond_next1090:
  llvm_cbe_c_19 = llvm_cbe_c_19__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_c_19) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true1095;
  } else {
    llvm_cbe_inescq_9__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_18__PHI_TEMPORARY = llvm_cbe_c_19;   /* for PHI node */
    goto llvm_cbe_bb1415;
  }

llvm_cbe_cond_false1061_cond_next1090_crit_edge:
  llvm_cbe_c_19__PHI_TEMPORARY = llvm_cbe_tmp1036;   /* for PHI node */
  goto llvm_cbe_cond_next1090;

  do {     /* Syntactic loop 'bb1110' to make GCC happy */
llvm_cbe_bb1110:
  llvm_cbe_c_2012437_12 = llvm_cbe_c_2012437_12__PHI_TEMPORARY;
  llvm_cbe_tmp1113 = &llvm_cbe_classbits[llvm_cbe_c_2012437_12];
  llvm_cbe_tmp1114 = *llvm_cbe_tmp1113;
  llvm_cbe_tmp1119 = *(&llvm_cbe_tmp1099[(llvm_cbe_c_2012437_12 + ((unsigned int )64))]);
  *llvm_cbe_tmp1113 = (((unsigned char )(llvm_cbe_tmp1119 | llvm_cbe_tmp1114)));
  llvm_cbe_indvar_next26667 = llvm_cbe_c_2012437_12 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26667 == ((unsigned int )32))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_c_2012437_12__PHI_TEMPORARY = llvm_cbe_indvar_next26667;   /* for PHI node */
    goto llvm_cbe_bb1110;
  }

  } while (1); /* end of syntactic loop 'bb1110' */
  do {     /* Syntactic loop 'bb1131' to make GCC happy */
llvm_cbe_bb1131:
  llvm_cbe_c_2112444_12 = llvm_cbe_c_2112444_12__PHI_TEMPORARY;
  llvm_cbe_tmp1134 = &llvm_cbe_classbits[llvm_cbe_c_2112444_12];
  llvm_cbe_tmp1135 = *llvm_cbe_tmp1134;
  llvm_cbe_tmp1140 = *(&llvm_cbe_tmp1099[(llvm_cbe_c_2112444_12 + ((unsigned int )64))]);
  *llvm_cbe_tmp1134 = (((unsigned char )(llvm_cbe_tmp1135 | (((unsigned char )(llvm_cbe_tmp1140 ^ ((unsigned char )-1)))))));
  llvm_cbe_indvar_next26664 = llvm_cbe_c_2112444_12 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26664 == ((unsigned int )32))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_c_2112444_12__PHI_TEMPORARY = llvm_cbe_indvar_next26664;   /* for PHI node */
    goto llvm_cbe_bb1131;
  }

  } while (1); /* end of syntactic loop 'bb1131' */
  do {     /* Syntactic loop 'bb1152' to make GCC happy */
llvm_cbe_bb1152:
  llvm_cbe_c_2212451_12 = llvm_cbe_c_2212451_12__PHI_TEMPORARY;
  llvm_cbe_tmp1155 = &llvm_cbe_classbits[llvm_cbe_c_2212451_12];
  llvm_cbe_tmp1156 = *llvm_cbe_tmp1155;
  llvm_cbe_tmp1161 = *(&llvm_cbe_tmp1099[(llvm_cbe_c_2212451_12 + ((unsigned int )160))]);
  *llvm_cbe_tmp1155 = (((unsigned char )(llvm_cbe_tmp1161 | llvm_cbe_tmp1156)));
  llvm_cbe_indvar_next26661 = llvm_cbe_c_2212451_12 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26661 == ((unsigned int )32))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_c_2212451_12__PHI_TEMPORARY = llvm_cbe_indvar_next26661;   /* for PHI node */
    goto llvm_cbe_bb1152;
  }

  } while (1); /* end of syntactic loop 'bb1152' */
  do {     /* Syntactic loop 'bb1173' to make GCC happy */
llvm_cbe_bb1173:
  llvm_cbe_c_2312458_12 = llvm_cbe_c_2312458_12__PHI_TEMPORARY;
  llvm_cbe_tmp1176 = &llvm_cbe_classbits[llvm_cbe_c_2312458_12];
  llvm_cbe_tmp1177 = *llvm_cbe_tmp1176;
  llvm_cbe_tmp1182 = *(&llvm_cbe_tmp1099[(llvm_cbe_c_2312458_12 + ((unsigned int )160))]);
  *llvm_cbe_tmp1176 = (((unsigned char )(llvm_cbe_tmp1177 | (((unsigned char )(llvm_cbe_tmp1182 ^ ((unsigned char )-1)))))));
  llvm_cbe_indvar_next26658 = llvm_cbe_c_2312458_12 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26658 == ((unsigned int )32))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_c_2312458_12__PHI_TEMPORARY = llvm_cbe_indvar_next26658;   /* for PHI node */
    goto llvm_cbe_bb1173;
  }

  } while (1); /* end of syntactic loop 'bb1173' */
llvm_cbe_bb1212:
  llvm_cbe_tmp1214 = *llvm_cbe_tmp1237;
  *llvm_cbe_tmp1237 = (((unsigned char )(llvm_cbe_tmp1214 & ((unsigned char )-9))));
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
  goto llvm_cbe_bb1694;

  do {     /* Syntactic loop 'bb1194' to make GCC happy */
llvm_cbe_bb1194:
  llvm_cbe_c_2426580 = llvm_cbe_c_2426580__PHI_TEMPORARY;
  llvm_cbe_tmp1197 = &llvm_cbe_classbits[llvm_cbe_c_2426580];
  llvm_cbe_tmp1198 = *llvm_cbe_tmp1197;
  llvm_cbe_tmp1202 = *(&llvm_cbe_tmp1099[llvm_cbe_c_2426580]);
  *llvm_cbe_tmp1197 = (((unsigned char )(llvm_cbe_tmp1202 | llvm_cbe_tmp1198)));
  llvm_cbe_tmp1206 = llvm_cbe_c_2426580 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp1206) < ((signed int )((unsigned int )32)))) {
    llvm_cbe_c_2426580__PHI_TEMPORARY = llvm_cbe_tmp1206;   /* for PHI node */
    goto llvm_cbe_bb1194;
  } else {
    goto llvm_cbe_bb1212;
  }

  } while (1); /* end of syntactic loop 'bb1194' */
llvm_cbe_bb1236:
  llvm_cbe_tmp1238 = *llvm_cbe_tmp1237;
  *llvm_cbe_tmp1237 = (((unsigned char )(llvm_cbe_tmp1238 | ((unsigned char )8))));
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
  goto llvm_cbe_bb1694;

  do {     /* Syntactic loop 'bb1218' to make GCC happy */
llvm_cbe_bb1218:
  llvm_cbe_c_2526577 = llvm_cbe_c_2526577__PHI_TEMPORARY;
  llvm_cbe_tmp1221 = &llvm_cbe_classbits[llvm_cbe_c_2526577];
  llvm_cbe_tmp1222 = *llvm_cbe_tmp1221;
  llvm_cbe_tmp1226 = *(&llvm_cbe_tmp1099[llvm_cbe_c_2526577]);
  *llvm_cbe_tmp1221 = (((unsigned char )(llvm_cbe_tmp1222 | (((unsigned char )(llvm_cbe_tmp1226 ^ ((unsigned char )-1)))))));
  llvm_cbe_tmp1230 = llvm_cbe_c_2526577 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp1230) < ((signed int )((unsigned int )32)))) {
    llvm_cbe_c_2526577__PHI_TEMPORARY = llvm_cbe_tmp1230;   /* for PHI node */
    goto llvm_cbe_bb1218;
  } else {
    goto llvm_cbe_bb1236;
  }

  } while (1); /* end of syntactic loop 'bb1218' */
llvm_cbe_cond_false1244:
  if (((((unsigned int )(llvm_cbe_c_19 + ((unsigned int )7))) < ((unsigned int )((unsigned int )2))) | (llvm_cbe_c_19 == ((unsigned int )-11)))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    goto llvm_cbe_cond_next1258;
  }

llvm_cbe_cond_next1258:
  if (((((unsigned int )(llvm_cbe_c_19 + ((unsigned int )10))) < ((unsigned int )((unsigned int )2))) | (llvm_cbe_c_19 == ((unsigned int )-8)))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    goto llvm_cbe_cond_next1278;
  }

llvm_cbe_cond_true1283:
  llvm_cbe_tmp1285 = *llvm_cbe_tmp1237;
  *llvm_cbe_tmp1237 = (((unsigned char )(llvm_cbe_tmp1285 | ((unsigned char )2))));
  llvm_cbe_tmp1289 = *llvm_cbe_tmp1288;
  *llvm_cbe_tmp1288 = (((unsigned char )(llvm_cbe_tmp1289 | ((unsigned char )1))));
  llvm_cbe_tmp1293 = *llvm_cbe_tmp1292;
  *llvm_cbe_tmp1292 = (((unsigned char )(llvm_cbe_tmp1293 | ((unsigned char )1))));
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
  goto llvm_cbe_bb1694;

llvm_cbe_cond_next1278:
  switch (llvm_cbe_c_19) {
  default:
    goto llvm_cbe_cond_next1396;
;
  case ((unsigned int )-18):
    goto llvm_cbe_cond_true1283;
    break;
  case ((unsigned int )-17):
    llvm_cbe_c_2612465_12__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1302;
  case ((unsigned int )-20):
    goto llvm_cbe_cond_true1336;
  case ((unsigned int )-19):
    llvm_cbe_c_2712472_12__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1363;
  }
  do {     /* Syntactic loop 'bb1302' to make GCC happy */
llvm_cbe_bb1302:
  llvm_cbe_c_2612465_12 = llvm_cbe_c_2612465_12__PHI_TEMPORARY;
  switch (llvm_cbe_c_2612465_12) {
  default:
    llvm_cbe_x_10__PHI_TEMPORARY = ((unsigned char )-1);   /* for PHI node */
    goto llvm_cbe_bb1314;
;
  case ((unsigned int )1):
    goto llvm_cbe_bb1304;
    break;
  case ((unsigned int )4):
    goto llvm_cbe_bb1307;
  case ((unsigned int )20):
    goto llvm_cbe_bb1310;
  }
llvm_cbe_bb1314:
  llvm_cbe_x_10 = llvm_cbe_x_10__PHI_TEMPORARY;
  llvm_cbe_tmp1317 = &llvm_cbe_classbits[llvm_cbe_c_2612465_12];
  llvm_cbe_tmp1318 = *llvm_cbe_tmp1317;
  *llvm_cbe_tmp1317 = (((unsigned char )(llvm_cbe_tmp1318 | llvm_cbe_x_10)));
  llvm_cbe_indvar_next26655 = llvm_cbe_c_2612465_12 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26655 == ((unsigned int )32))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_c_2612465_12__PHI_TEMPORARY = llvm_cbe_indvar_next26655;   /* for PHI node */
    goto llvm_cbe_bb1302;
  }

llvm_cbe_bb1304:
  llvm_cbe_x_10__PHI_TEMPORARY = ((unsigned char )-3);   /* for PHI node */
  goto llvm_cbe_bb1314;

llvm_cbe_bb1307:
  llvm_cbe_x_10__PHI_TEMPORARY = ((unsigned char )-2);   /* for PHI node */
  goto llvm_cbe_bb1314;

llvm_cbe_bb1310:
  llvm_cbe_x_10__PHI_TEMPORARY = ((unsigned char )-2);   /* for PHI node */
  goto llvm_cbe_bb1314;

  } while (1); /* end of syntactic loop 'bb1302' */
llvm_cbe_cond_true1336:
  llvm_cbe_tmp1338 = *llvm_cbe_tmp1237;
  *llvm_cbe_tmp1237 = (((unsigned char )(llvm_cbe_tmp1338 | ((unsigned char )60))));
  llvm_cbe_tmp1354 = *llvm_cbe_tmp1353;
  *llvm_cbe_tmp1353 = (((unsigned char )(llvm_cbe_tmp1354 | ((unsigned char )32))));
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
  goto llvm_cbe_bb1694;

  do {     /* Syntactic loop 'bb1363' to make GCC happy */
llvm_cbe_bb1363:
  llvm_cbe_c_2712472_12 = llvm_cbe_c_2712472_12__PHI_TEMPORARY;
  switch (llvm_cbe_c_2712472_12) {
  default:
    llvm_cbe_x1364_10__PHI_TEMPORARY = ((unsigned char )-1);   /* for PHI node */
    goto llvm_cbe_bb1379;
;
  case ((unsigned int )1):
    goto llvm_cbe_bb1366;
    break;
  case ((unsigned int )16):
    goto llvm_cbe_bb1375;
  }
llvm_cbe_bb1379:
  llvm_cbe_x1364_10 = llvm_cbe_x1364_10__PHI_TEMPORARY;
  llvm_cbe_tmp1382 = &llvm_cbe_classbits[llvm_cbe_c_2712472_12];
  llvm_cbe_tmp1383 = *llvm_cbe_tmp1382;
  *llvm_cbe_tmp1382 = (((unsigned char )(llvm_cbe_tmp1383 | llvm_cbe_x1364_10)));
  llvm_cbe_indvar_next26652 = llvm_cbe_c_2712472_12 + ((unsigned int )1);
  if ((llvm_cbe_indvar_next26652 == ((unsigned int )32))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_class_lastchar_7;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1101;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_c_2712472_12__PHI_TEMPORARY = llvm_cbe_indvar_next26652;   /* for PHI node */
    goto llvm_cbe_bb1363;
  }

llvm_cbe_bb1366:
  llvm_cbe_x1364_10__PHI_TEMPORARY = ((unsigned char )-61);   /* for PHI node */
  goto llvm_cbe_bb1379;

llvm_cbe_bb1375:
  llvm_cbe_x1364_10__PHI_TEMPORARY = ((unsigned char )-33);   /* for PHI node */
  goto llvm_cbe_bb1379;

  } while (1); /* end of syntactic loop 'bb1363' */
llvm_cbe_cond_next1591:
  llvm_cbe_tmp1597 = ((llvm_cbe_class_charcount_7 + ((unsigned int )1)) - llvm_cbe_c_1812491_1) + llvm_cbe_d_1012504_1;
  if (((((signed int )llvm_cbe_c_1812491_1) > ((signed int )llvm_cbe_d_1012504_1)) | llvm_cbe_tmp138_not)) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_d_1012504_1;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1597;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_10;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    goto llvm_cbe_bb1604_preheader;
  }

llvm_cbe_cond_next1584:
  llvm_cbe_d_1012504_1 = llvm_cbe_d_1012504_1__PHI_TEMPORARY;
  if ((llvm_cbe_d_1012504_1 == llvm_cbe_c_1812491_1)) {
    llvm_cbe_inescq_11__PHI_TEMPORARY = llvm_cbe_inescq_10;   /* for PHI node */
    goto llvm_cbe_LONE_SINGLE_CHARACTER;
  } else {
    goto llvm_cbe_cond_next1591;
  }

llvm_cbe_cond_true1563:
  if ((((signed int )llvm_cbe_c_1812491_1) > ((signed int )((unsigned int )88)))) {
    goto llvm_cbe_cond_true1582;
  } else {
    llvm_cbe_d_1012504_1__PHI_TEMPORARY = ((unsigned int )88);   /* for PHI node */
    goto llvm_cbe_cond_next1584;
  }

llvm_cbe_cond_true1552:
  switch (llvm_cbe_tmp1540) {
  default:
    goto llvm_cbe_cond_false1570;
;
  case ((unsigned int )-5):
    llvm_cbe_d_10__PHI_TEMPORARY = ((unsigned int )8);   /* for PHI node */
    goto llvm_cbe_cond_next1576;
  case ((unsigned int )-21):
    goto llvm_cbe_cond_true1563;
    break;
  case ((unsigned int )-16):
    goto llvm_cbe_cond_true1569;
  }
llvm_cbe_cond_next1547:
  if ((((signed int )llvm_cbe_tmp1540) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true1552;
  } else {
    llvm_cbe_d_10__PHI_TEMPORARY = llvm_cbe_tmp1540;   /* for PHI node */
    goto llvm_cbe_cond_next1576;
  }

llvm_cbe_cond_true1534:
  llvm_cbe_tmp1537 = *llvm_cbe_tmp24;
  llvm_cbe_tmp1540 = check_escape((&llvm_cbe_ptr106), llvm_cbe_errorcodeptr, llvm_cbe_tmp1537, llvm_cbe_options104_39466_9, ((unsigned int )1));
  llvm_cbe_tmp1542 = *llvm_cbe_errorcodeptr;
  if ((llvm_cbe_tmp1542 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next1547;
  } else {
    llvm_cbe_slot_913437_0_us61_12__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_12__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    goto llvm_cbe_FAILED;
  }

llvm_cbe_bb1519:
  llvm_cbe_tmp15211522 = ((unsigned int )(unsigned char )llvm_cbe_tmp1500);
  if (((llvm_cbe_tmp1500 == ((unsigned char )92)) & (llvm_cbe_inescq_10 == ((unsigned int )0)))) {
    goto llvm_cbe_cond_true1534;
  } else {
    llvm_cbe_d_10__PHI_TEMPORARY = llvm_cbe_tmp15211522;   /* for PHI node */
    goto llvm_cbe_cond_next1576;
  }

llvm_cbe_cond_next1505:
  if (((llvm_cbe_inescq_10 == ((unsigned int )0)) & (llvm_cbe_tmp1500 == ((unsigned char )93)))) {
    goto llvm_cbe_bb1517;
  } else {
    goto llvm_cbe_bb1519;
  }

llvm_cbe_bb1498:
  llvm_cbe_inescq_10 = llvm_cbe_inescq_10__PHI_TEMPORARY;
  llvm_cbe_tmp1499 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1500 = *llvm_cbe_tmp1499;
  if ((llvm_cbe_tmp1500 == ((unsigned char )0))) {
    goto llvm_cbe_bb1517;
  } else {
    goto llvm_cbe_cond_next1505;
  }

  do {     /* Syntactic loop 'bb1484' to make GCC happy */
llvm_cbe_bb1484:
  llvm_cbe_tmp1485 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1486 = *llvm_cbe_tmp1485;
  if ((llvm_cbe_tmp1486 == ((unsigned char )92))) {
    goto llvm_cbe_cond_next1491;
  } else {
    llvm_cbe_inescq_10__PHI_TEMPORARY = llvm_cbe_inescq_912486_1;   /* for PHI node */
    goto llvm_cbe_bb1498;
  }

llvm_cbe_cond_true1479:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp1485[((unsigned int )4)]);
  goto llvm_cbe_bb1484;

llvm_cbe_cond_true1472:
  llvm_cbe_tmp1475 = *(&llvm_cbe_tmp1485[((unsigned int )3)]);
  if ((llvm_cbe_tmp1475 == ((unsigned char )69))) {
    goto llvm_cbe_cond_true1479;
  } else {
    llvm_cbe_inescq_10__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_bb1498;
  }

llvm_cbe_bb1464:
  llvm_cbe_tmp1466 = &llvm_cbe_tmp1485[((unsigned int )2)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1466;
  llvm_cbe_tmp1468 = *llvm_cbe_tmp1466;
  if ((llvm_cbe_tmp1468 == ((unsigned char )92))) {
    goto llvm_cbe_cond_true1472;
  } else {
    llvm_cbe_inescq_10__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_bb1498;
  }

llvm_cbe_cond_next1491:
  llvm_cbe_tmp1494 = *(&llvm_cbe_tmp1485[((unsigned int )1)]);
  if ((llvm_cbe_tmp1494 == ((unsigned char )81))) {
    goto llvm_cbe_bb1464;
  } else {
    llvm_cbe_inescq_10__PHI_TEMPORARY = llvm_cbe_inescq_912486_1;   /* for PHI node */
    goto llvm_cbe_bb1498;
  }

  } while (1); /* end of syntactic loop 'bb1484' */
  do {     /* Syntactic loop 'bb1449' to make GCC happy */
llvm_cbe_bb1449:
  llvm_cbe_tmp1447_pn = *(&llvm_cbe_ptr106);
  llvm_cbe_storemerge5533 = &llvm_cbe_tmp1447_pn[((unsigned int )2)];
  *(&llvm_cbe_ptr106) = llvm_cbe_storemerge5533;
  llvm_cbe_tmp1451 = *llvm_cbe_storemerge5533;
  if ((llvm_cbe_tmp1451 == ((unsigned char )92))) {
    goto llvm_cbe_cond_next1456;
  } else {
    goto llvm_cbe_bb1484;
  }

llvm_cbe_cond_next1456:
  llvm_cbe_tmp1459 = *(&llvm_cbe_tmp1447_pn[((unsigned int )3)]);
  if ((llvm_cbe_tmp1459 == ((unsigned char )69))) {
    goto llvm_cbe_bb1449;
  } else {
    goto llvm_cbe_bb1484;
  }

  } while (1); /* end of syntactic loop 'bb1449' */
llvm_cbe_cond_true1436:
  llvm_cbe_tmp1439 = *(&llvm_cbe_tmp1431[((unsigned int )1)]);
  if ((llvm_cbe_tmp1439 == ((unsigned char )45))) {
    goto llvm_cbe_bb1449;
  } else {
    llvm_cbe_inescq_11__PHI_TEMPORARY = llvm_cbe_inescq_912486_1;   /* for PHI node */
    goto llvm_cbe_LONE_SINGLE_CHARACTER;
  }

llvm_cbe_bb1430:
  llvm_cbe_inescq_912486_1 = llvm_cbe_inescq_912486_1__PHI_TEMPORARY;
  llvm_cbe_c_1812491_1 = llvm_cbe_c_1812491_1__PHI_TEMPORARY;
  llvm_cbe_tmp1431 = *(&llvm_cbe_ptr106);
  if ((llvm_cbe_inescq_912486_1 == ((unsigned int )0))) {
    goto llvm_cbe_cond_true1436;
  } else {
    llvm_cbe_inescq_11__PHI_TEMPORARY = llvm_cbe_inescq_912486_1;   /* for PHI node */
    goto llvm_cbe_LONE_SINGLE_CHARACTER;
  }

llvm_cbe_cond_next1404:
  llvm_cbe_tmp1407 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1408 = *llvm_cbe_tmp1407;
  llvm_cbe_tmp14081409 = ((unsigned int )(unsigned char )llvm_cbe_tmp1408);
  llvm_cbe_tmp141812502 = *(&llvm_cbe_tmp1407[((unsigned int )1)]);
  if ((llvm_cbe_tmp141812502 == ((unsigned char )92))) {
    llvm_cbe_inescq_912486_0__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_1812491_0__PHI_TEMPORARY = llvm_cbe_tmp14081409;   /* for PHI node */
    goto llvm_cbe_cond_next1423;
  } else {
    llvm_cbe_inescq_912486_1__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
    llvm_cbe_c_1812491_1__PHI_TEMPORARY = llvm_cbe_tmp14081409;   /* for PHI node */
    goto llvm_cbe_bb1430;
  }

llvm_cbe_cond_next1396:
  if (((llvm_cbe_options104_39466_9 & ((unsigned int )64)) == ((unsigned int )0))) {
    goto llvm_cbe_cond_next1404;
  } else {
    goto llvm_cbe_cond_true1402;
  }

llvm_cbe_bb1415:
  llvm_cbe_inescq_9 = llvm_cbe_inescq_9__PHI_TEMPORARY;
  llvm_cbe_c_18 = llvm_cbe_c_18__PHI_TEMPORARY;
  llvm_cbe_tmp1416 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1418 = *(&llvm_cbe_tmp1416[((unsigned int )1)]);
  if ((llvm_cbe_tmp1418 == ((unsigned char )92))) {
    llvm_cbe_inescq_912486_0__PHI_TEMPORARY = llvm_cbe_inescq_9;   /* for PHI node */
    llvm_cbe_c_1812491_0__PHI_TEMPORARY = llvm_cbe_c_18;   /* for PHI node */
    goto llvm_cbe_cond_next1423;
  } else {
    llvm_cbe_inescq_912486_1__PHI_TEMPORARY = llvm_cbe_inescq_9;   /* for PHI node */
    llvm_cbe_c_1812491_1__PHI_TEMPORARY = llvm_cbe_c_18;   /* for PHI node */
    goto llvm_cbe_bb1430;
  }

llvm_cbe_cond_true1060:
  llvm_cbe_inescq_9__PHI_TEMPORARY = llvm_cbe_inescq_4;   /* for PHI node */
  llvm_cbe_c_18__PHI_TEMPORARY = ((unsigned int )82);   /* for PHI node */
  goto llvm_cbe_bb1415;

llvm_cbe_bb1412:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1425;
  llvm_cbe_inescq_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_c_18__PHI_TEMPORARY = llvm_cbe_c_1812491_0;   /* for PHI node */
  goto llvm_cbe_bb1415;

llvm_cbe_cond_next1423:
  llvm_cbe_inescq_912486_0 = llvm_cbe_inescq_912486_0__PHI_TEMPORARY;
  llvm_cbe_c_1812491_0 = llvm_cbe_c_1812491_0__PHI_TEMPORARY;
  llvm_cbe_tmp1424 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1425 = &llvm_cbe_tmp1424[((unsigned int )2)];
  llvm_cbe_tmp1426 = *llvm_cbe_tmp1425;
  if ((llvm_cbe_tmp1426 == ((unsigned char )69))) {
    goto llvm_cbe_bb1412;
  } else {
    llvm_cbe_inescq_912486_1__PHI_TEMPORARY = llvm_cbe_inescq_912486_0;   /* for PHI node */
    llvm_cbe_c_1812491_1__PHI_TEMPORARY = llvm_cbe_c_1812491_0;   /* for PHI node */
    goto llvm_cbe_bb1430;
  }

llvm_cbe_cond_true1569:
  if ((((signed int )llvm_cbe_c_1812491_1) > ((signed int )((unsigned int )82)))) {
    goto llvm_cbe_cond_true1582;
  } else {
    llvm_cbe_d_1012504_1__PHI_TEMPORARY = ((unsigned int )82);   /* for PHI node */
    goto llvm_cbe_cond_next1584;
  }

llvm_cbe_cond_next1576:
  llvm_cbe_d_10 = llvm_cbe_d_10__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_d_10) < ((signed int )llvm_cbe_c_1812491_1))) {
    goto llvm_cbe_cond_true1582;
  } else {
    llvm_cbe_d_1012504_1__PHI_TEMPORARY = llvm_cbe_d_10;   /* for PHI node */
    goto llvm_cbe_cond_next1584;
  }

  do {     /* Syntactic loop 'bb1604' to make GCC happy */
llvm_cbe_bb1604:
  llvm_cbe_indvar26648 = llvm_cbe_indvar26648__PHI_TEMPORARY;
  llvm_cbe_c_2812521_12 = llvm_cbe_indvar26648 + llvm_cbe_c_1812491_1;
  llvm_cbe_tmp1609 = &llvm_cbe_classbits[(((signed int )(((signed int )llvm_cbe_c_2812521_12) / ((signed int )((unsigned int )8)))))];
  llvm_cbe_tmp1610 = *llvm_cbe_tmp1609;
  *llvm_cbe_tmp1609 = (((unsigned char )((((unsigned char )(((unsigned int )1) << (llvm_cbe_c_2812521_12 & ((unsigned int )7))))) | llvm_cbe_tmp1610)));
  llvm_cbe_tmp1624 = *llvm_cbe_tmp1623;
  llvm_cbe_tmp1627 = *(&llvm_cbe_tmp1624[llvm_cbe_c_2812521_12]);
  llvm_cbe_tmp16271628 = ((unsigned int )(unsigned char )llvm_cbe_tmp1627);
  llvm_cbe_tmp1633 = &llvm_cbe_classbits[(((unsigned int )(((unsigned int )llvm_cbe_tmp16271628) >> ((unsigned int )((unsigned int )3)))))];
  llvm_cbe_tmp1634 = *llvm_cbe_tmp1633;
  *llvm_cbe_tmp1633 = (((unsigned char )((((unsigned char )(((unsigned int )1) << (llvm_cbe_tmp16271628 & ((unsigned int )7))))) | llvm_cbe_tmp1634)));
  if ((((signed int )(llvm_cbe_c_2812521_12 + ((unsigned int )1))) > ((signed int )llvm_cbe_d_1012504_1))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_d_1012504_1;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1597;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_10;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_indvar26648__PHI_TEMPORARY = (llvm_cbe_indvar26648 + ((unsigned int )1));   /* for PHI node */
    goto llvm_cbe_bb1604;
  }

  } while (1); /* end of syntactic loop 'bb1604' */
llvm_cbe_bb1604_preheader:
  if ((llvm_cbe_tmp865 == ((unsigned int )0))) {
    llvm_cbe_indvar26645__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1604_us;
  } else {
    llvm_cbe_indvar26648__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1604;
  }

  do {     /* Syntactic loop 'bb1604.us' to make GCC happy */
llvm_cbe_bb1604_us:
  llvm_cbe_indvar26645 = llvm_cbe_indvar26645__PHI_TEMPORARY;
  llvm_cbe_c_2812521_12_us = llvm_cbe_indvar26645 + llvm_cbe_c_1812491_1;
  llvm_cbe_tmp1609_us = &llvm_cbe_classbits[(((signed int )(((signed int )llvm_cbe_c_2812521_12_us) / ((signed int )((unsigned int )8)))))];
  llvm_cbe_tmp1610_us = *llvm_cbe_tmp1609_us;
  *llvm_cbe_tmp1609_us = (((unsigned char )((((unsigned char )(((unsigned int )1) << (llvm_cbe_c_2812521_12_us & ((unsigned int )7))))) | llvm_cbe_tmp1610_us)));
  if ((((signed int )(llvm_cbe_c_2812521_12_us + ((unsigned int )1))) > ((signed int )llvm_cbe_d_1012504_1))) {
    llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_d_1012504_1;   /* for PHI node */
    llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1597;   /* for PHI node */
    llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_10;   /* for PHI node */
    goto llvm_cbe_bb1694;
  } else {
    llvm_cbe_indvar26645__PHI_TEMPORARY = (llvm_cbe_indvar26645 + ((unsigned int )1));   /* for PHI node */
    goto llvm_cbe_bb1604_us;
  }

  } while (1); /* end of syntactic loop 'bb1604.us' */
llvm_cbe_cond_next1690:
  llvm_cbe_c_29 = llvm_cbe_c_29__PHI_TEMPORARY;
  llvm_cbe_tmp1692 = llvm_cbe_class_charcount_7 + ((unsigned int )1);
  llvm_cbe_class_lastchar_6__PHI_TEMPORARY = llvm_cbe_c_29;   /* for PHI node */
  llvm_cbe_class_charcount_6__PHI_TEMPORARY = llvm_cbe_tmp1692;   /* for PHI node */
  llvm_cbe_inescq_3__PHI_TEMPORARY = llvm_cbe_inescq_11;   /* for PHI node */
  goto llvm_cbe_bb1694;

llvm_cbe_LONE_SINGLE_CHARACTER:
  llvm_cbe_inescq_11 = llvm_cbe_inescq_11__PHI_TEMPORARY;
  llvm_cbe_tmp1658 = &llvm_cbe_classbits[(((signed int )(((signed int )llvm_cbe_c_1812491_1) / ((signed int )((unsigned int )8)))))];
  llvm_cbe_tmp1659 = *llvm_cbe_tmp1658;
  *llvm_cbe_tmp1658 = (((unsigned char )(llvm_cbe_tmp1659 | (((unsigned char )(((unsigned int )1) << (llvm_cbe_c_1812491_1 & ((unsigned int )7))))))));
  if ((llvm_cbe_tmp865 == ((unsigned int )0))) {
    llvm_cbe_c_29__PHI_TEMPORARY = llvm_cbe_c_1812491_1;   /* for PHI node */
    goto llvm_cbe_cond_next1690;
  } else {
    goto llvm_cbe_cond_true1670;
  }

llvm_cbe_bb1517:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1431;
  llvm_cbe_inescq_11__PHI_TEMPORARY = llvm_cbe_inescq_10;   /* for PHI node */
  goto llvm_cbe_LONE_SINGLE_CHARACTER;

llvm_cbe_cond_false1570:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1431;
  llvm_cbe_inescq_11__PHI_TEMPORARY = llvm_cbe_inescq_10;   /* for PHI node */
  goto llvm_cbe_LONE_SINGLE_CHARACTER;

llvm_cbe_cond_true1670:
  llvm_cbe_tmp1673 = *llvm_cbe_tmp1623;
  llvm_cbe_tmp1676 = *(&llvm_cbe_tmp1673[llvm_cbe_c_1812491_1]);
  llvm_cbe_tmp16761677 = ((unsigned int )(unsigned char )llvm_cbe_tmp1676);
  llvm_cbe_tmp1682 = &llvm_cbe_classbits[(((unsigned int )(((unsigned int )llvm_cbe_tmp16761677) >> ((unsigned int )((unsigned int )3)))))];
  llvm_cbe_tmp1683 = *llvm_cbe_tmp1682;
  *llvm_cbe_tmp1682 = (((unsigned char )((((unsigned char )(((unsigned int )1) << (llvm_cbe_tmp16761677 & ((unsigned int )7))))) | llvm_cbe_tmp1683)));
  llvm_cbe_c_29__PHI_TEMPORARY = llvm_cbe_tmp16761677;   /* for PHI node */
  goto llvm_cbe_cond_next1690;

  } while (1); /* end of syntactic loop 'bb717' */
llvm_cbe_bb717_preheader:
  llvm_cbe_tmp865 = llvm_cbe_options104_39466_9 & ((unsigned int )1);
  llvm_cbe_class_lastchar_7__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  llvm_cbe_class_charcount_7__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_inescq_4__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_c_11__PHI_TEMPORARY = llvm_cbe_c_8;   /* for PHI node */
  goto llvm_cbe_bb717;

llvm_cbe_cond_next1800:
  llvm_cbe_code105_11 = llvm_cbe_code105_11__PHI_TEMPORARY;
  llvm_cbe_tmp1802 = &llvm_cbe_code105_11[((unsigned int )32)];
  llvm_cbe_tmp519612629 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519612629[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_5;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp1802;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_6;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_6;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_true1769:
  *llvm_cbe_code105_10 = ((unsigned char )78);
  llvm_cbe_tmp1772 = &llvm_cbe_code105_10[((unsigned int )1)];
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    llvm_cbe_c_3112623_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1778;
  } else {
    llvm_cbe_code105_11__PHI_TEMPORARY = llvm_cbe_tmp1772;   /* for PHI node */
    goto llvm_cbe_cond_next1800;
  }

llvm_cbe_cond_next1756:
  llvm_cbe_firstbyte102_6 = (((llvm_cbe_firstbyte102_39829_9 == ((unsigned int )-2))) ? (((unsigned int )-1)) : (llvm_cbe_firstbyte102_39829_9));
  if ((llvm_cbe_negate_class_0 == ((unsigned int )0))) {
    goto llvm_cbe_cond_false1794;
  } else {
    goto llvm_cbe_cond_true1769;
  }

  do {     /* Syntactic loop 'bb1778' to make GCC happy */
llvm_cbe_bb1778:
  llvm_cbe_c_3112623_4 = llvm_cbe_c_3112623_4__PHI_TEMPORARY;
  llvm_cbe_tmp1781 = *(&llvm_cbe_classbits[llvm_cbe_c_3112623_4]);
  llvm_cbe_tmp1772_sum = llvm_cbe_c_3112623_4 + ((unsigned int )1);
  *(&llvm_cbe_code105_10[llvm_cbe_tmp1772_sum]) = (((unsigned char )(llvm_cbe_tmp1781 ^ ((unsigned char )-1))));
  if ((llvm_cbe_tmp1772_sum == ((unsigned int )32))) {
    llvm_cbe_code105_11__PHI_TEMPORARY = llvm_cbe_tmp1772;   /* for PHI node */
    goto llvm_cbe_cond_next1800;
  } else {
    llvm_cbe_c_3112623_4__PHI_TEMPORARY = llvm_cbe_tmp1772_sum;   /* for PHI node */
    goto llvm_cbe_bb1778;
  }

  } while (1); /* end of syntactic loop 'bb1778' */
llvm_cbe_cond_false1794:
  *llvm_cbe_code105_10 = ((unsigned char )77);
  llvm_cbe_tmp1797 = &llvm_cbe_code105_10[((unsigned int )1)];
  ltmp_7_1 = memcpy(llvm_cbe_tmp1797, llvm_cbe_classbits711, ((unsigned int )32));
  llvm_cbe_code105_11__PHI_TEMPORARY = llvm_cbe_tmp1797;   /* for PHI node */
  goto llvm_cbe_cond_next1800;

llvm_cbe_END_REPEAT:
  llvm_cbe_repeat_max_13 = llvm_cbe_repeat_max_13__PHI_TEMPORARY;
  llvm_cbe_save_hwm_5 = llvm_cbe_save_hwm_5__PHI_TEMPORARY;
  llvm_cbe_code105_13 = llvm_cbe_code105_13__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_9 = llvm_cbe_reqbyte103_9__PHI_TEMPORARY;
  llvm_cbe_tmp3167 = *llvm_cbe_tmp1993;
  *llvm_cbe_tmp1993 = (llvm_cbe_tmp3167 | llvm_cbe_iftmp_236_0);
  llvm_cbe_tmp519613332 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613332[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_13;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_9;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_5;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_13;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_7;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next2111:
  llvm_cbe_possessive_quantifier_612736_1 = llvm_cbe_possessive_quantifier_612736_1__PHI_TEMPORARY;
  llvm_cbe_c_3212739_1 = llvm_cbe_c_3212739_1__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_712742_1 = llvm_cbe_reqbyte103_712742_1__PHI_TEMPORARY;
  llvm_cbe_op_type_512745_1 = llvm_cbe_op_type_512745_1__PHI_TEMPORARY;
  llvm_cbe_repeat_type_612750_1 = llvm_cbe_repeat_type_612750_1__PHI_TEMPORARY;
  llvm_cbe_prop_value_0 = llvm_cbe_prop_value_0__PHI_TEMPORARY;
  llvm_cbe_prop_type_0 = llvm_cbe_prop_type_0__PHI_TEMPORARY;
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )0))) {
    llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
    llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_712742_1;   /* for PHI node */
    goto llvm_cbe_END_REPEAT;
  } else {
    goto llvm_cbe_cond_next2119;
  }

llvm_cbe_cond_true2021:
  llvm_cbe_tmp209512753 = *llvm_cbe_previous_25708_1;
  if ((((unsigned char )(((unsigned char )(llvm_cbe_tmp209512753 + ((unsigned char )-14))))) < ((unsigned char )((unsigned char )2)))) {
    llvm_cbe_possessive_quantifier_612736_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    llvm_cbe_c_3212739_0__PHI_TEMPORARY = llvm_cbe_tmp19821983;   /* for PHI node */
    llvm_cbe_reqbyte103_712742_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_6;   /* for PHI node */
    llvm_cbe_op_type_512745_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_repeat_type_612750_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true2100;
  } else {
    llvm_cbe_possessive_quantifier_612736_1__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    llvm_cbe_c_3212739_1__PHI_TEMPORARY = llvm_cbe_tmp19821983;   /* for PHI node */
    llvm_cbe_reqbyte103_712742_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_6;   /* for PHI node */
    llvm_cbe_op_type_512745_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_repeat_type_612750_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_prop_value_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_prop_type_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_cond_next2111;
  }

llvm_cbe_cond_true2006:
  llvm_cbe_tmp2007 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp2010 = *llvm_cbe_previous_25708_1;
  llvm_cbe_tmp2017 = check_auto_possessive((((unsigned int )(unsigned char )llvm_cbe_tmp2010)), llvm_cbe_tmp19821983, (&llvm_cbe_tmp2007[((unsigned int )1)]), llvm_cbe_options104_39466_9, llvm_cbe_cd);
  if ((llvm_cbe_tmp2017 == ((unsigned int )0))) {
    llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_512675_0;   /* for PHI node */
    llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp19821983;   /* for PHI node */
    llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_6;   /* for PHI node */
    llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_repeat_type_6__PHI_TEMPORARY = llvm_cbe_repeat_type_012701_0;   /* for PHI node */
    goto llvm_cbe_OUTPUT_SINGLE_REPEAT;
  } else {
    goto llvm_cbe_cond_true2021;
  }

llvm_cbe_cond_next1996:
  llvm_cbe_reqbyte103_6 = llvm_cbe_reqbyte103_6__PHI_TEMPORARY;
  if (((llvm_cbe_possessive_quantifier_512675_0 == ((unsigned int )0)) & (((signed int )llvm_cbe_repeat_max_9) < ((signed int )((unsigned int )0))))) {
    goto llvm_cbe_cond_true2006;
  } else {
    llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_512675_0;   /* for PHI node */
    llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp19821983;   /* for PHI node */
    llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_6;   /* for PHI node */
    llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_repeat_type_6__PHI_TEMPORARY = llvm_cbe_repeat_type_012701_0;   /* for PHI node */
    goto llvm_cbe_OUTPUT_SINGLE_REPEAT;
  }

llvm_cbe_cond_true1979:
  llvm_cbe_possessive_quantifier_512675_0 = llvm_cbe_possessive_quantifier_512675_0__PHI_TEMPORARY;
  llvm_cbe_repeat_type_012701_0 = llvm_cbe_repeat_type_012701_0__PHI_TEMPORARY;
  llvm_cbe_tmp1982 = *(&llvm_cbe_code105_10[((unsigned int )-1)]);
  llvm_cbe_tmp19821983 = ((unsigned int )(unsigned char )llvm_cbe_tmp1982);
  if ((((signed int )llvm_cbe_repeat_min_9) > ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true1988;
  } else {
    llvm_cbe_reqbyte103_6__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_cond_next1996;
  }

llvm_cbe_cond_true1955:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1950;
  llvm_cbe_tmp197412733 = *llvm_cbe_previous_25708_1;
  if ((((unsigned char )(((unsigned char )(llvm_cbe_tmp197412733 + ((unsigned char )-27))))) < ((unsigned char )((unsigned char )2)))) {
    llvm_cbe_possessive_quantifier_512675_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    llvm_cbe_repeat_type_012701_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_true1979;
  } else {
    llvm_cbe_possessive_quantifier_512675_1__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    llvm_cbe_repeat_type_012701_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_false2025;
  }

llvm_cbe_cond_next1930:
  llvm_cbe_repeat_max_9 = llvm_cbe_repeat_max_9__PHI_TEMPORARY;
  llvm_cbe_repeat_min_9 = llvm_cbe_repeat_min_9__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_1 = (((llvm_cbe_repeat_min_9 == ((unsigned int )0))) ? (llvm_cbe_zeroreqbyte_29762_9) : (llvm_cbe_reqbyte103_59784_9));
  llvm_cbe_firstbyte102_7 = (((llvm_cbe_repeat_min_9 == ((unsigned int )0))) ? (llvm_cbe_zerofirstbyte_29740_9) : (llvm_cbe_firstbyte102_39829_9));
  llvm_cbe_iftmp_236_0 = (((llvm_cbe_repeat_min_9 == llvm_cbe_repeat_max_9)) ? (((unsigned int )0)) : (((unsigned int )512)));
  *(&llvm_cbe_tempcode) = llvm_cbe_previous_25708_1;
  llvm_cbe_tmp1949 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp1950 = &llvm_cbe_tmp1949[((unsigned int )1)];
  llvm_cbe_tmp1951 = *llvm_cbe_tmp1950;
  switch (llvm_cbe_tmp1951) {
  default:
    llvm_cbe_repeat_type_0__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_cond_next1972;
;
  case ((unsigned char )43):
    goto llvm_cbe_cond_true1955;
    break;
  case ((unsigned char )63):
    goto llvm_cbe_cond_true1965;
  }
llvm_cbe_bb1921:
  if ((llvm_cbe_previous_25708_1 == ((unsigned char *)/*NULL*/0))) {
    goto llvm_cbe_cond_true1928;
  } else {
    llvm_cbe_repeat_max_9__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_repeat_min_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_next1930;
  }

llvm_cbe_REPEAT:
  llvm_cbe_repeat_max_4 = llvm_cbe_repeat_max_4__PHI_TEMPORARY;
  llvm_cbe_repeat_min_4 = llvm_cbe_repeat_min_4__PHI_TEMPORARY;
  if ((llvm_cbe_previous_25708_1 == ((unsigned char *)/*NULL*/0))) {
    goto llvm_cbe_cond_true1928;
  } else {
    llvm_cbe_repeat_max_9__PHI_TEMPORARY = llvm_cbe_repeat_max_4;   /* for PHI node */
    llvm_cbe_repeat_min_9__PHI_TEMPORARY = llvm_cbe_repeat_min_4;   /* for PHI node */
    goto llvm_cbe_cond_next1930;
  }

llvm_cbe_bb1912:
  llvm_cbe_repeat_max_3 = llvm_cbe_repeat_max_3__PHI_TEMPORARY;
  llvm_cbe_repeat_min_3 = llvm_cbe_repeat_min_3__PHI_TEMPORARY;
  llvm_cbe_tmp93_0 = llvm_cbe_tmp93_0__PHI_TEMPORARY;
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp93_0;
  llvm_cbe_tmp1915 = *llvm_cbe_errorcodeptr;
  if ((llvm_cbe_tmp1915 == ((unsigned int )0))) {
    llvm_cbe_repeat_max_4__PHI_TEMPORARY = llvm_cbe_repeat_max_3;   /* for PHI node */
    llvm_cbe_repeat_min_4__PHI_TEMPORARY = llvm_cbe_repeat_min_3;   /* for PHI node */
    goto llvm_cbe_REPEAT;
  } else {
    llvm_cbe_slot_913437_0_us61_12__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_12__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    goto llvm_cbe_FAILED;
  }

llvm_cbe_cond_true1842:
  *llvm_cbe_errorcodeptr = ((unsigned int )5);
  llvm_cbe_repeat_max_3__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_3__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_tmp93_0__PHI_TEMPORARY = llvm_cbe_p1813_6_lcssa;   /* for PHI node */
  goto llvm_cbe_bb1912;

llvm_cbe_bb1837:
  llvm_cbe_p1813_6_lcssa = llvm_cbe_p1813_6_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp1810_pn_lcssa = llvm_cbe_tmp1810_pn_lcssa__PHI_TEMPORARY;
  llvm_cbe_min_6_lcssa = llvm_cbe_min_6_lcssa__PHI_TEMPORARY;
  if ((((unsigned int )llvm_cbe_min_6_lcssa) > ((unsigned int )((unsigned int )65535)))) {
    goto llvm_cbe_cond_true1842;
  } else {
    goto llvm_cbe_cond_next1846;
  }

llvm_cbe_cond_next1809:
  llvm_cbe_tmp1810 = *(&llvm_cbe_ptr106);
  llvm_cbe_p1813_626504 = &llvm_cbe_tmp1810[((unsigned int )1)];
  llvm_cbe_tmp182826506 = *llvm_cbe_p1813_626504;
  llvm_cbe_tmp183126509 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp182826506))]);
  if (((((unsigned char )(llvm_cbe_tmp183126509 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p1813_6_lcssa__PHI_TEMPORARY = llvm_cbe_p1813_626504;   /* for PHI node */
    llvm_cbe_tmp1810_pn_lcssa__PHI_TEMPORARY = llvm_cbe_tmp1810;   /* for PHI node */
    llvm_cbe_min_6_lcssa__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1837;
  } else {
    llvm_cbe_p1813_626505__PHI_TEMPORARY = llvm_cbe_p1813_626504;   /* for PHI node */
    llvm_cbe_min_626502__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1816;
  }

llvm_cbe_bb1803:
  if (llvm_cbe_iftmp_136_0) {
    llvm_cbe_storemerge26707_in__PHI_TEMPORARY = llvm_cbe_c_2;   /* for PHI node */
    llvm_cbe_last_code_15771_4__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
    llvm_cbe_options_addr_25789_11__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_79347_11__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_11__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_options104_39466_11__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_11__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_11__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_39909_11__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    llvm_cbe_previous_callout_3__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_inescq_6__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_8__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_after_manual_callout_3__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
    goto llvm_cbe_ONE_CHAR;
  } else {
    goto llvm_cbe_cond_next1809;
  }

  do {     /* Syntactic loop 'bb1816' to make GCC happy */
llvm_cbe_bb1816:
  llvm_cbe_p1813_626505 = llvm_cbe_p1813_626505__PHI_TEMPORARY;
  llvm_cbe_min_626502 = llvm_cbe_min_626502__PHI_TEMPORARY;
  llvm_cbe_tmp1820 = *llvm_cbe_p1813_626505;
  llvm_cbe_tmp1823 = ((llvm_cbe_min_626502 * ((unsigned int )10)) + ((unsigned int )-48)) + (((unsigned int )(unsigned char )llvm_cbe_tmp1820));
  llvm_cbe_p1813_6 = &llvm_cbe_p1813_626505[((unsigned int )1)];
  llvm_cbe_tmp1828 = *llvm_cbe_p1813_6;
  llvm_cbe_tmp1831 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp1828))]);
  if (((((unsigned char )(llvm_cbe_tmp1831 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p1813_6_lcssa__PHI_TEMPORARY = llvm_cbe_p1813_6;   /* for PHI node */
    llvm_cbe_tmp1810_pn_lcssa__PHI_TEMPORARY = llvm_cbe_p1813_626505;   /* for PHI node */
    llvm_cbe_min_6_lcssa__PHI_TEMPORARY = llvm_cbe_tmp1823;   /* for PHI node */
    goto llvm_cbe_bb1837;
  } else {
    llvm_cbe_p1813_626505__PHI_TEMPORARY = llvm_cbe_p1813_6;   /* for PHI node */
    llvm_cbe_min_626502__PHI_TEMPORARY = llvm_cbe_tmp1823;   /* for PHI node */
    goto llvm_cbe_bb1816;
  }

  } while (1); /* end of syntactic loop 'bb1816' */
llvm_cbe_cond_next1846:
  llvm_cbe_tmp1848 = *llvm_cbe_p1813_6_lcssa;
  if ((llvm_cbe_tmp1848 == ((unsigned char )125))) {
    llvm_cbe_repeat_max_3__PHI_TEMPORARY = llvm_cbe_min_6_lcssa;   /* for PHI node */
    llvm_cbe_repeat_min_3__PHI_TEMPORARY = llvm_cbe_min_6_lcssa;   /* for PHI node */
    llvm_cbe_tmp93_0__PHI_TEMPORARY = llvm_cbe_p1813_6_lcssa;   /* for PHI node */
    goto llvm_cbe_bb1912;
  } else {
    goto llvm_cbe_cond_false1854;
  }

llvm_cbe_cond_false1854:
  llvm_cbe_tmp1856 = &llvm_cbe_tmp1810_pn_lcssa[((unsigned int )2)];
  llvm_cbe_tmp1858 = *llvm_cbe_tmp1856;
  if ((llvm_cbe_tmp1858 == ((unsigned char )125))) {
    llvm_cbe_repeat_max_3__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_repeat_min_3__PHI_TEMPORARY = llvm_cbe_min_6_lcssa;   /* for PHI node */
    llvm_cbe_tmp93_0__PHI_TEMPORARY = llvm_cbe_tmp1856;   /* for PHI node */
    goto llvm_cbe_bb1912;
  } else {
    goto llvm_cbe_bb1873_preheader;
  }

llvm_cbe_cond_true1889:
  *llvm_cbe_errorcodeptr = ((unsigned int )5);
  llvm_cbe_repeat_max_3__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_3__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_tmp93_0__PHI_TEMPORARY = llvm_cbe_p1813_9_lcssa;   /* for PHI node */
  goto llvm_cbe_bb1912;

llvm_cbe_bb1884:
  llvm_cbe_p1813_9_lcssa = llvm_cbe_p1813_9_lcssa__PHI_TEMPORARY;
  llvm_cbe_max_8_lcssa = llvm_cbe_max_8_lcssa__PHI_TEMPORARY;
  if ((((unsigned int )llvm_cbe_max_8_lcssa) > ((unsigned int )((unsigned int )65535)))) {
    goto llvm_cbe_cond_true1889;
  } else {
    goto llvm_cbe_cond_next1893;
  }

llvm_cbe_bb1873_preheader:
  llvm_cbe_tmp187826498 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp1858))]);
  if (((((unsigned char )(llvm_cbe_tmp187826498 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p1813_9_lcssa__PHI_TEMPORARY = llvm_cbe_tmp1856;   /* for PHI node */
    llvm_cbe_max_8_lcssa__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1884;
  } else {
    llvm_cbe_p1813_926494_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_max_826493__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb1863;
  }

  do {     /* Syntactic loop 'bb1863' to make GCC happy */
llvm_cbe_bb1863:
  llvm_cbe_p1813_926494_rec = llvm_cbe_p1813_926494_rec__PHI_TEMPORARY;
  llvm_cbe_max_826493 = llvm_cbe_max_826493__PHI_TEMPORARY;
  llvm_cbe_tmp1867 = *(&llvm_cbe_tmp1810_pn_lcssa[(llvm_cbe_p1813_926494_rec + ((unsigned int )2))]);
  llvm_cbe_tmp1870 = ((llvm_cbe_max_826493 * ((unsigned int )10)) + ((unsigned int )-48)) + (((unsigned int )(unsigned char )llvm_cbe_tmp1867));
  llvm_cbe_tmp1872 = &llvm_cbe_tmp1810_pn_lcssa[(llvm_cbe_p1813_926494_rec + ((unsigned int )3))];
  llvm_cbe_tmp1875 = *llvm_cbe_tmp1872;
  llvm_cbe_tmp1878 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp1875))]);
  if (((((unsigned char )(llvm_cbe_tmp1878 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_p1813_9_lcssa__PHI_TEMPORARY = llvm_cbe_tmp1872;   /* for PHI node */
    llvm_cbe_max_8_lcssa__PHI_TEMPORARY = llvm_cbe_tmp1870;   /* for PHI node */
    goto llvm_cbe_bb1884;
  } else {
    llvm_cbe_p1813_926494_rec__PHI_TEMPORARY = (llvm_cbe_p1813_926494_rec + ((unsigned int )1));   /* for PHI node */
    llvm_cbe_max_826493__PHI_TEMPORARY = llvm_cbe_tmp1870;   /* for PHI node */
    goto llvm_cbe_bb1863;
  }

  } while (1); /* end of syntactic loop 'bb1863' */
llvm_cbe_cond_next1893:
  if ((((signed int )llvm_cbe_max_8_lcssa) < ((signed int )llvm_cbe_min_6_lcssa))) {
    goto llvm_cbe_cond_true1899;
  } else {
    llvm_cbe_repeat_max_3__PHI_TEMPORARY = llvm_cbe_max_8_lcssa;   /* for PHI node */
    llvm_cbe_repeat_min_3__PHI_TEMPORARY = llvm_cbe_min_6_lcssa;   /* for PHI node */
    llvm_cbe_tmp93_0__PHI_TEMPORARY = llvm_cbe_p1813_9_lcssa;   /* for PHI node */
    goto llvm_cbe_bb1912;
  }

llvm_cbe_cond_true1899:
  *llvm_cbe_errorcodeptr = ((unsigned int )4);
  llvm_cbe_repeat_max_3__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_3__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_tmp93_0__PHI_TEMPORARY = llvm_cbe_p1813_9_lcssa;   /* for PHI node */
  goto llvm_cbe_bb1912;

llvm_cbe_bb1923:
  llvm_cbe_repeat_max_4__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  llvm_cbe_repeat_min_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_REPEAT;

llvm_cbe_cond_next1972:
  llvm_cbe_repeat_type_0 = llvm_cbe_repeat_type_0__PHI_TEMPORARY;
  llvm_cbe_tmp1974 = *llvm_cbe_previous_25708_1;
  if ((((unsigned char )(((unsigned char )(llvm_cbe_tmp1974 + ((unsigned char )-27))))) < ((unsigned char )((unsigned char )2)))) {
    llvm_cbe_possessive_quantifier_512675_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_repeat_type_012701_0__PHI_TEMPORARY = llvm_cbe_repeat_type_0;   /* for PHI node */
    goto llvm_cbe_cond_true1979;
  } else {
    llvm_cbe_possessive_quantifier_512675_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_repeat_type_012701_1__PHI_TEMPORARY = llvm_cbe_repeat_type_0;   /* for PHI node */
    goto llvm_cbe_cond_false2025;
  }

llvm_cbe_cond_true1965:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp1950;
  llvm_cbe_repeat_type_0__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  goto llvm_cbe_cond_next1972;

llvm_cbe_cond_true1988:
  llvm_cbe_tmp1994 = *llvm_cbe_tmp1993;
  llvm_cbe_tmp1995 = (llvm_cbe_tmp19821983 | llvm_cbe_req_caseopt_39708_9) | llvm_cbe_tmp1994;
  llvm_cbe_reqbyte103_6__PHI_TEMPORARY = llvm_cbe_tmp1995;   /* for PHI node */
  goto llvm_cbe_cond_next1996;

llvm_cbe_OUTPUT_SINGLE_REPEAT:
  llvm_cbe_possessive_quantifier_6 = llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY;
  llvm_cbe_c_32 = llvm_cbe_c_32__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_7 = llvm_cbe_reqbyte103_7__PHI_TEMPORARY;
  llvm_cbe_op_type_5 = llvm_cbe_op_type_5__PHI_TEMPORARY;
  llvm_cbe_repeat_type_6 = llvm_cbe_repeat_type_6__PHI_TEMPORARY;
  llvm_cbe_tmp2095 = *llvm_cbe_previous_25708_1;
  if ((((unsigned char )(((unsigned char )(llvm_cbe_tmp2095 + ((unsigned char )-14))))) < ((unsigned char )((unsigned char )2)))) {
    llvm_cbe_possessive_quantifier_612736_0__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_6;   /* for PHI node */
    llvm_cbe_c_3212739_0__PHI_TEMPORARY = llvm_cbe_c_32;   /* for PHI node */
    llvm_cbe_reqbyte103_712742_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_7;   /* for PHI node */
    llvm_cbe_op_type_512745_0__PHI_TEMPORARY = llvm_cbe_op_type_5;   /* for PHI node */
    llvm_cbe_repeat_type_612750_0__PHI_TEMPORARY = llvm_cbe_repeat_type_6;   /* for PHI node */
    goto llvm_cbe_cond_true2100;
  } else {
    llvm_cbe_possessive_quantifier_612736_1__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_6;   /* for PHI node */
    llvm_cbe_c_3212739_1__PHI_TEMPORARY = llvm_cbe_c_32;   /* for PHI node */
    llvm_cbe_reqbyte103_712742_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_7;   /* for PHI node */
    llvm_cbe_op_type_512745_1__PHI_TEMPORARY = llvm_cbe_op_type_5;   /* for PHI node */
    llvm_cbe_repeat_type_612750_1__PHI_TEMPORARY = llvm_cbe_repeat_type_6;   /* for PHI node */
    llvm_cbe_prop_value_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_prop_type_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_cond_next2111;
  }

llvm_cbe_cond_true2031:
  llvm_cbe_tmp2034 = *(&llvm_cbe_previous_25708_1[((unsigned int )1)]);
  llvm_cbe_tmp20342035 = ((unsigned int )(unsigned char )llvm_cbe_tmp2034);
  if (((llvm_cbe_possessive_quantifier_512675_1 == ((unsigned int )0)) & (((signed int )llvm_cbe_repeat_max_9) < ((signed int )((unsigned int )0))))) {
    goto llvm_cbe_cond_true2045;
  } else {
    llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_512675_1;   /* for PHI node */
    llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp20342035;   /* for PHI node */
    llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )13);   /* for PHI node */
    llvm_cbe_repeat_type_6__PHI_TEMPORARY = llvm_cbe_repeat_type_012701_1;   /* for PHI node */
    goto llvm_cbe_OUTPUT_SINGLE_REPEAT;
  }

llvm_cbe_cond_false2025:
  llvm_cbe_possessive_quantifier_512675_1 = llvm_cbe_possessive_quantifier_512675_1__PHI_TEMPORARY;
  llvm_cbe_repeat_type_012701_1 = llvm_cbe_repeat_type_012701_1__PHI_TEMPORARY;
  llvm_cbe_tmp2027 = *llvm_cbe_previous_25708_1;
  if ((llvm_cbe_tmp2027 == ((unsigned char )29))) {
    goto llvm_cbe_cond_true2031;
  } else {
    goto llvm_cbe_cond_false2060;
  }

llvm_cbe_cond_true2045:
  llvm_cbe_tmp2046 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp2052 = check_auto_possessive(((unsigned int )29), llvm_cbe_tmp20342035, (&llvm_cbe_tmp2046[((unsigned int )1)]), llvm_cbe_options104_39466_9, llvm_cbe_cd);
  if ((llvm_cbe_tmp2052 == ((unsigned int )0))) {
    llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_512675_1;   /* for PHI node */
    llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp20342035;   /* for PHI node */
    llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )13);   /* for PHI node */
    llvm_cbe_repeat_type_6__PHI_TEMPORARY = llvm_cbe_repeat_type_012701_1;   /* for PHI node */
    goto llvm_cbe_OUTPUT_SINGLE_REPEAT;
  } else {
    goto llvm_cbe_cond_true2056;
  }

llvm_cbe_cond_true2056:
  llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp20342035;   /* for PHI node */
  llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
  llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )13);   /* for PHI node */
  llvm_cbe_repeat_type_6__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_OUTPUT_SINGLE_REPEAT;

llvm_cbe_cond_true2066:
  llvm_cbe_tmp20682069 = ((unsigned int )(unsigned char )llvm_cbe_tmp2027);
  if (((llvm_cbe_possessive_quantifier_512675_1 == ((unsigned int )0)) & (((signed int )llvm_cbe_repeat_max_9) < ((signed int )((unsigned int )0))))) {
    goto llvm_cbe_cond_true2079;
  } else {
    llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_512675_1;   /* for PHI node */
    llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp20682069;   /* for PHI node */
    llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )26);   /* for PHI node */
    llvm_cbe_repeat_type_6__PHI_TEMPORARY = llvm_cbe_repeat_type_012701_1;   /* for PHI node */
    goto llvm_cbe_OUTPUT_SINGLE_REPEAT;
  }

llvm_cbe_cond_false2060:
  if ((((unsigned char )llvm_cbe_tmp2027) < ((unsigned char )((unsigned char )22)))) {
    goto llvm_cbe_cond_true2066;
  } else {
    goto llvm_cbe_cond_false2348;
  }

llvm_cbe_cond_true2079:
  llvm_cbe_tmp2080 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp2086 = check_auto_possessive(llvm_cbe_tmp20682069, ((unsigned int )0), (&llvm_cbe_tmp2080[((unsigned int )1)]), llvm_cbe_options104_39466_9, llvm_cbe_cd);
  if ((llvm_cbe_tmp2086 == ((unsigned int )0))) {
    llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_512675_1;   /* for PHI node */
    llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp20682069;   /* for PHI node */
    llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )26);   /* for PHI node */
    llvm_cbe_repeat_type_6__PHI_TEMPORARY = llvm_cbe_repeat_type_012701_1;   /* for PHI node */
    goto llvm_cbe_OUTPUT_SINGLE_REPEAT;
  } else {
    goto llvm_cbe_cond_true2090;
  }

llvm_cbe_cond_true2090:
  llvm_cbe_possessive_quantifier_6__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  llvm_cbe_c_32__PHI_TEMPORARY = llvm_cbe_tmp20682069;   /* for PHI node */
  llvm_cbe_reqbyte103_7__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
  llvm_cbe_op_type_5__PHI_TEMPORARY = ((unsigned int )26);   /* for PHI node */
  llvm_cbe_repeat_type_6__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_OUTPUT_SINGLE_REPEAT;

llvm_cbe_cond_true2100:
  llvm_cbe_possessive_quantifier_612736_0 = llvm_cbe_possessive_quantifier_612736_0__PHI_TEMPORARY;
  llvm_cbe_c_3212739_0 = llvm_cbe_c_3212739_0__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_712742_0 = llvm_cbe_reqbyte103_712742_0__PHI_TEMPORARY;
  llvm_cbe_op_type_512745_0 = llvm_cbe_op_type_512745_0__PHI_TEMPORARY;
  llvm_cbe_repeat_type_612750_0 = llvm_cbe_repeat_type_612750_0__PHI_TEMPORARY;
  llvm_cbe_tmp2103 = *(&llvm_cbe_previous_25708_1[((unsigned int )1)]);
  llvm_cbe_tmp21032104 = ((unsigned int )(unsigned char )llvm_cbe_tmp2103);
  llvm_cbe_tmp2107 = *(&llvm_cbe_previous_25708_1[((unsigned int )2)]);
  llvm_cbe_tmp21072108 = ((unsigned int )(unsigned char )llvm_cbe_tmp2107);
  llvm_cbe_possessive_quantifier_612736_1__PHI_TEMPORARY = llvm_cbe_possessive_quantifier_612736_0;   /* for PHI node */
  llvm_cbe_c_3212739_1__PHI_TEMPORARY = llvm_cbe_c_3212739_0;   /* for PHI node */
  llvm_cbe_reqbyte103_712742_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_712742_0;   /* for PHI node */
  llvm_cbe_op_type_512745_1__PHI_TEMPORARY = llvm_cbe_op_type_512745_0;   /* for PHI node */
  llvm_cbe_repeat_type_612750_1__PHI_TEMPORARY = llvm_cbe_repeat_type_612750_0;   /* for PHI node */
  llvm_cbe_prop_value_0__PHI_TEMPORARY = llvm_cbe_tmp21072108;   /* for PHI node */
  llvm_cbe_prop_type_0__PHI_TEMPORARY = llvm_cbe_tmp21032104;   /* for PHI node */
  goto llvm_cbe_cond_next2111;

llvm_cbe_cond_false2197:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )1))) {
    llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
    llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_712742_1;   /* for PHI node */
    goto llvm_cbe_END_REPEAT;
  } else {
    goto llvm_cbe_cond_next2204;
  }

llvm_cbe_cond_true2185:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )-1))) {
    goto llvm_cbe_cond_true2190;
  } else {
    goto llvm_cbe_cond_false2197;
  }

llvm_cbe_cond_next2127:
  llvm_cbe_tmp2130 = llvm_cbe_repeat_type_612750_1 + llvm_cbe_op_type_512745_1;
  switch (llvm_cbe_repeat_min_9) {
  default:
    goto llvm_cbe_cond_false2226;
;
  case ((unsigned int )0):
    goto llvm_cbe_cond_true2135;
    break;
  case ((unsigned int )1):
    goto llvm_cbe_cond_true2185;
  }
llvm_cbe_cond_next2119:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )1))) {
    goto llvm_cbe_cond_next2127;
  } else {
    goto llvm_cbe_cond_true2124;
  }

llvm_cbe_cond_true2124:
  *llvm_cbe_tmp2377 = ((unsigned int )1);
  goto llvm_cbe_cond_next2127;

llvm_cbe_cond_next2342:
  llvm_cbe_repeat_max_11 = llvm_cbe_repeat_max_11__PHI_TEMPORARY;
  llvm_cbe_code105_14 = llvm_cbe_code105_14__PHI_TEMPORARY;
  *llvm_cbe_code105_14 = (((unsigned char )llvm_cbe_c_3212739_1));
  llvm_cbe_tmp2347 = &llvm_cbe_code105_14[((unsigned int )1)];
  if ((llvm_cbe_possessive_quantifier_612736_1 == ((unsigned int )0))) {
    llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_11;   /* for PHI node */
    llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_tmp2347;   /* for PHI node */
    llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_712742_1;   /* for PHI node */
    goto llvm_cbe_END_REPEAT;
  } else {
    llvm_cbe_repeat_max_12__PHI_TEMPORARY = llvm_cbe_repeat_max_11;   /* for PHI node */
    llvm_cbe_save_hwm_413043_0__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_1213058_0__PHI_TEMPORARY = llvm_cbe_tmp2347;   /* for PHI node */
    llvm_cbe_reqbyte103_813077_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_712742_1;   /* for PHI node */
    goto llvm_cbe_cond_true3056;
  }

llvm_cbe_cond_true2140:
  *llvm_cbe_previous_25708_1 = (((unsigned char )(llvm_cbe_tmp21412142 + ((unsigned char )30))));
  llvm_cbe_tmp2146 = &llvm_cbe_previous_25708_1[((unsigned int )1)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2146;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_true2135:
  llvm_cbe_tmp21412142 = ((unsigned char )llvm_cbe_tmp2130);
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )-1))) {
    goto llvm_cbe_cond_true2140;
  } else {
    goto llvm_cbe_cond_false2147;
  }

llvm_cbe_cond_true2152:
  *llvm_cbe_previous_25708_1 = (((unsigned char )(llvm_cbe_tmp21412142 + ((unsigned char )34))));
  llvm_cbe_tmp2158 = &llvm_cbe_previous_25708_1[((unsigned int )1)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2158;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_false2147:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )1))) {
    goto llvm_cbe_cond_true2152;
  } else {
    goto llvm_cbe_cond_false2159;
  }

llvm_cbe_cond_false2159:
  *llvm_cbe_previous_25708_1 = (((unsigned char )(llvm_cbe_tmp21412142 + ((unsigned char )36))));
  *(&llvm_cbe_previous_25708_1[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_repeat_max_9) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_previous_25708_1[((unsigned int )2)]) = (((unsigned char )llvm_cbe_repeat_max_9));
  llvm_cbe_tmp2177 = &llvm_cbe_previous_25708_1[((unsigned int )3)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2177;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_true2190:
  *llvm_cbe_previous_25708_1 = (((unsigned char )((((unsigned char )llvm_cbe_tmp2130)) + ((unsigned char )32))));
  llvm_cbe_tmp2196 = &llvm_cbe_previous_25708_1[((unsigned int )1)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2196;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_next2204:
  *llvm_cbe_code105_10 = (((unsigned char )((((unsigned char )llvm_cbe_tmp2130)) + ((unsigned char )36))));
  *(&llvm_cbe_code105_10[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )(llvm_cbe_repeat_max_9 + ((unsigned int )65535))) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )2)]) = (((unsigned char )((((unsigned char )llvm_cbe_repeat_max_9)) + ((unsigned char )-1))));
  llvm_cbe_tmp2224 = &llvm_cbe_code105_10[((unsigned int )3)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2224;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_true2259:
  *llvm_cbe_tmp2254 = (((unsigned char )llvm_cbe_prop_type_0));
  *(&llvm_cbe_previous_25708_1[((unsigned int )5)]) = (((unsigned char )llvm_cbe_prop_value_0));
  *(&llvm_cbe_previous_25708_1[((unsigned int )6)]) = llvm_cbe_tmp2273;
  llvm_cbe_tmp227612756 = &llvm_cbe_previous_25708_1[((unsigned int )7)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp227612756;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_true2249:
  *llvm_cbe_tmp2244 = (((unsigned char )llvm_cbe_c_3212739_1));
  llvm_cbe_tmp2254 = &llvm_cbe_previous_25708_1[((unsigned int )4)];
  llvm_cbe_tmp2273 = ((unsigned char )((((unsigned char )llvm_cbe_tmp2130)) + ((unsigned char )30)));
  if ((((signed int )llvm_cbe_prop_type_0) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true2259;
  } else {
    goto llvm_cbe_cond_next2270;
  }

llvm_cbe_cond_false2226:
  *llvm_cbe_previous_25708_1 = (((unsigned char )((((unsigned char )llvm_cbe_op_type_512745_1)) + ((unsigned char )38))));
  *(&llvm_cbe_previous_25708_1[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_repeat_min_9) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_previous_25708_1[((unsigned int )2)]) = (((unsigned char )llvm_cbe_repeat_min_9));
  llvm_cbe_tmp2244 = &llvm_cbe_previous_25708_1[((unsigned int )3)];
  if ((((signed int )llvm_cbe_repeat_max_9) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true2249;
  } else {
    goto llvm_cbe_cond_false2277;
  }

llvm_cbe_cond_next2270:
  *llvm_cbe_tmp2254 = llvm_cbe_tmp2273;
  llvm_cbe_tmp2276 = &llvm_cbe_previous_25708_1[((unsigned int )5)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2276;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_false2277:
  if ((llvm_cbe_repeat_min_9 == llvm_cbe_repeat_max_9)) {
    llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
    llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2244;   /* for PHI node */
    goto llvm_cbe_cond_next2342;
  } else {
    goto llvm_cbe_cond_true2283;
  }

llvm_cbe_cond_true2312:
  *llvm_cbe_code105_16 = (((unsigned char )(llvm_cbe_tmp23132314 + ((unsigned char )34))));
  llvm_cbe_tmp2318 = &llvm_cbe_code105_16[((unsigned int )1)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_tmp2307;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2318;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_next2304:
  llvm_cbe_code105_16 = llvm_cbe_code105_16__PHI_TEMPORARY;
  llvm_cbe_tmp2307 = llvm_cbe_repeat_max_9 - llvm_cbe_repeat_min_9;
  llvm_cbe_tmp23132314 = ((unsigned char )llvm_cbe_tmp2130);
  if ((llvm_cbe_tmp2307 == ((unsigned int )1))) {
    goto llvm_cbe_cond_true2312;
  } else {
    goto llvm_cbe_cond_false2319;
  }

llvm_cbe_cond_true2283:
  *llvm_cbe_tmp2244 = (((unsigned char )llvm_cbe_c_3212739_1));
  llvm_cbe_tmp2288 = &llvm_cbe_previous_25708_1[((unsigned int )4)];
  if ((((signed int )llvm_cbe_prop_type_0) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true2293;
  } else {
    llvm_cbe_code105_16__PHI_TEMPORARY = llvm_cbe_tmp2288;   /* for PHI node */
    goto llvm_cbe_cond_next2304;
  }

llvm_cbe_cond_true2293:
  *llvm_cbe_tmp2288 = (((unsigned char )llvm_cbe_prop_type_0));
  *(&llvm_cbe_previous_25708_1[((unsigned int )5)]) = (((unsigned char )llvm_cbe_prop_value_0));
  llvm_cbe_tmp2303 = &llvm_cbe_previous_25708_1[((unsigned int )6)];
  llvm_cbe_code105_16__PHI_TEMPORARY = llvm_cbe_tmp2303;   /* for PHI node */
  goto llvm_cbe_cond_next2304;

llvm_cbe_cond_false2319:
  *llvm_cbe_code105_16 = (((unsigned char )(llvm_cbe_tmp23132314 + ((unsigned char )36))));
  *(&llvm_cbe_code105_16[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp2307) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_16[((unsigned int )2)]) = (((unsigned char )llvm_cbe_tmp2307));
  llvm_cbe_tmp2337 = &llvm_cbe_code105_16[((unsigned int )3)];
  llvm_cbe_repeat_max_11__PHI_TEMPORARY = llvm_cbe_tmp2307;   /* for PHI node */
  llvm_cbe_code105_14__PHI_TEMPORARY = llvm_cbe_tmp2337;   /* for PHI node */
  goto llvm_cbe_cond_next2342;

llvm_cbe_cond_true2363:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )0))) {
    llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
    llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_END_REPEAT;
  } else {
    goto llvm_cbe_cond_next2370;
  }

llvm_cbe_cond_false2348:
  if (((((unsigned char )(((unsigned char )(llvm_cbe_tmp2027 + ((unsigned char )-77))))) < ((unsigned char )((unsigned char )2))) | (llvm_cbe_tmp2027 == ((unsigned char )80)))) {
    goto llvm_cbe_cond_true2363;
  } else {
    goto llvm_cbe_cond_false2475;
  }

llvm_cbe_cond_true2547:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )0))) {
    llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
    llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_END_REPEAT;
  } else {
    goto llvm_cbe_cond_next2554;
  }

llvm_cbe_cond_next2542:
  llvm_cbe_ketoffset_7 = llvm_cbe_ketoffset_7__PHI_TEMPORARY;
  if ((llvm_cbe_repeat_min_9 == ((unsigned int )0))) {
    goto llvm_cbe_cond_true2547;
  } else {
    goto llvm_cbe_cond_false2628;
  }

llvm_cbe_cond_next2511:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )-1))) {
    llvm_cbe_ket_6_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb2518;
  } else {
    llvm_cbe_ketoffset_7__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_next2542;
  }

llvm_cbe_cond_next2504:
  if ((((signed int )llvm_cbe_tmp2488) > ((signed int )((unsigned int )30000)))) {
    goto llvm_cbe_cond_true2509;
  } else {
    goto llvm_cbe_cond_next2511;
  }

llvm_cbe_cond_true2482:
  llvm_cbe_tmp24842485 = ((unsigned int )(unsigned long)llvm_cbe_code105_10);
  llvm_cbe_tmp2488 = llvm_cbe_tmp24842485 - (((unsigned int )(unsigned long)llvm_cbe_previous_25708_1));
  if ((llvm_cbe_tmp2027 == ((unsigned char )95))) {
    goto llvm_cbe_cond_true2494;
  } else {
    goto llvm_cbe_cond_next2504;
  }

llvm_cbe_cond_false2475:
  if ((((unsigned char )(((unsigned char )(llvm_cbe_tmp2027 + ((unsigned char )-92))))) < ((unsigned char )((unsigned char )4)))) {
    goto llvm_cbe_cond_true2482;
  } else {
    goto llvm_cbe_cond_false3045;
  }

llvm_cbe_cond_true2494:
  llvm_cbe_tmp2497 = *(&llvm_cbe_previous_25708_1[((unsigned int )3)]);
  if ((llvm_cbe_tmp2497 == ((unsigned char )101))) {
    goto llvm_cbe_cond_true2501;
  } else {
    goto llvm_cbe_cond_next2504;
  }

llvm_cbe_bb2536:
  llvm_cbe_tmp2541 = llvm_cbe_tmp24842485 - (((unsigned int )(unsigned long)llvm_cbe_tmp2530));
  llvm_cbe_ketoffset_7__PHI_TEMPORARY = llvm_cbe_tmp2541;   /* for PHI node */
  goto llvm_cbe_cond_next2542;

  do {     /* Syntactic loop 'bb2518' to make GCC happy */
llvm_cbe_bb2518:
  llvm_cbe_ket_6_rec = llvm_cbe_ket_6_rec__PHI_TEMPORARY;
  llvm_cbe_tmp2521 = *(&llvm_cbe_previous_25708_1[(llvm_cbe_ket_6_rec + ((unsigned int )1))]);
  llvm_cbe_tmp2526 = *(&llvm_cbe_previous_25708_1[(llvm_cbe_ket_6_rec + ((unsigned int )2))]);
  llvm_cbe_tmp2530_rec = llvm_cbe_ket_6_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp2521)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp2526)));
  llvm_cbe_tmp2530 = &llvm_cbe_previous_25708_1[llvm_cbe_tmp2530_rec];
  llvm_cbe_tmp2532 = *llvm_cbe_tmp2530;
  if ((llvm_cbe_tmp2532 == ((unsigned char )84))) {
    goto llvm_cbe_bb2536;
  } else {
    llvm_cbe_ket_6_rec__PHI_TEMPORARY = llvm_cbe_tmp2530_rec;   /* for PHI node */
    goto llvm_cbe_bb2518;
  }

  } while (1); /* end of syntactic loop 'bb2518' */
llvm_cbe_cond_next3051:
  llvm_cbe_repeat_max_15 = llvm_cbe_repeat_max_15__PHI_TEMPORARY;
  llvm_cbe_save_hwm_4 = llvm_cbe_save_hwm_4__PHI_TEMPORARY;
  llvm_cbe_code105_12 = llvm_cbe_code105_12__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_8 = llvm_cbe_reqbyte103_8__PHI_TEMPORARY;
  if ((llvm_cbe_possessive_quantifier_512675_1 == ((unsigned int )0))) {
    llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_15;   /* for PHI node */
    llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_4;   /* for PHI node */
    llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_12;   /* for PHI node */
    llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_8;   /* for PHI node */
    goto llvm_cbe_END_REPEAT;
  } else {
    llvm_cbe_repeat_max_12__PHI_TEMPORARY = llvm_cbe_repeat_max_15;   /* for PHI node */
    llvm_cbe_save_hwm_413043_0__PHI_TEMPORARY = llvm_cbe_save_hwm_4;   /* for PHI node */
    llvm_cbe_code105_1213058_0__PHI_TEMPORARY = llvm_cbe_code105_12;   /* for PHI node */
    llvm_cbe_reqbyte103_813077_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_8;   /* for PHI node */
    goto llvm_cbe_cond_true3056;
  }

llvm_cbe_cond_next2390:
  *llvm_cbe_code105_10 = (((unsigned char )((((unsigned char )llvm_cbe_repeat_type_012701_1)) + ((unsigned char )69))));
  llvm_cbe_tmp2396 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_tmp2396;   /* for PHI node */
  llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
  goto llvm_cbe_cond_next3051;

llvm_cbe_cond_next2378:
  if (((llvm_cbe_repeat_min_9 == ((unsigned int )0)) & (llvm_cbe_repeat_max_9 == ((unsigned int )-1)))) {
    goto llvm_cbe_cond_next2390;
  } else {
    goto llvm_cbe_bb2397;
  }

llvm_cbe_cond_next2370:
  if ((llvm_cbe_repeat_max_9 == ((unsigned int )1))) {
    goto llvm_cbe_cond_next2378;
  } else {
    goto llvm_cbe_cond_true2375;
  }

llvm_cbe_cond_true2375:
  *llvm_cbe_tmp2377 = ((unsigned int )1);
  goto llvm_cbe_cond_next2378;

llvm_cbe_cond_next2409:
  *llvm_cbe_code105_10 = (((unsigned char )((((unsigned char )llvm_cbe_repeat_type_012701_1)) + ((unsigned char )71))));
  llvm_cbe_tmp2415 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_tmp2415;   /* for PHI node */
  llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
  goto llvm_cbe_cond_next3051;

llvm_cbe_bb2397:
  if (((llvm_cbe_repeat_min_9 == ((unsigned int )1)) & (llvm_cbe_repeat_max_9 == ((unsigned int )-1)))) {
    goto llvm_cbe_cond_next2409;
  } else {
    goto llvm_cbe_bb2416;
  }

llvm_cbe_cond_next2428:
  *llvm_cbe_code105_10 = (((unsigned char )(llvm_cbe_tmp24362437 + ((unsigned char )73))));
  llvm_cbe_tmp2434 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
  llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_tmp2434;   /* for PHI node */
  llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
  goto llvm_cbe_cond_next3051;

llvm_cbe_bb2416:
  llvm_cbe_tmp24362437 = ((unsigned char )llvm_cbe_repeat_type_012701_1);
  if (((llvm_cbe_repeat_min_9 == ((unsigned int )0)) & (llvm_cbe_repeat_max_9 == ((unsigned int )1)))) {
    goto llvm_cbe_cond_next2428;
  } else {
    goto llvm_cbe_bb2435;
  }

llvm_cbe_bb2435:
  *llvm_cbe_code105_10 = (((unsigned char )(llvm_cbe_tmp24362437 + ((unsigned char )75))));
  *(&llvm_cbe_code105_10[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_repeat_min_9) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )2)]) = (((unsigned char )llvm_cbe_repeat_min_9));
  llvm_cbe_repeat_max_14 = (((llvm_cbe_repeat_max_9 == ((unsigned int )-1))) ? (((unsigned int )0)) : (llvm_cbe_repeat_max_9));
  *(&llvm_cbe_code105_10[((unsigned int )3)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_repeat_max_14) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )llvm_cbe_repeat_max_14));
  llvm_cbe_tmp2471 = &llvm_cbe_code105_10[((unsigned int )5)];
  llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_14;   /* for PHI node */
  llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_tmp2471;   /* for PHI node */
  llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
  goto llvm_cbe_cond_next3051;

llvm_cbe_bb2970_preheader:
  llvm_cbe_bralink_11_ph = llvm_cbe_bralink_11_ph__PHI_TEMPORARY;
  llvm_cbe_save_hwm_9_ph = llvm_cbe_save_hwm_9_ph__PHI_TEMPORARY;
  llvm_cbe_code105_22_ph = llvm_cbe_code105_22_ph__PHI_TEMPORARY;
  if ((llvm_cbe_bralink_11_ph == ((unsigned char *)/*NULL*/0))) {
    llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_16;   /* for PHI node */
    llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_9_ph;   /* for PHI node */
    llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_code105_22_ph;   /* for PHI node */
    llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1213269_0;   /* for PHI node */
    goto llvm_cbe_cond_next3051;
  } else {
    llvm_cbe_indvar26616__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_bralink_1126488__PHI_TEMPORARY = llvm_cbe_bralink_11_ph;   /* for PHI node */
    goto llvm_cbe_bb2911;
  }

llvm_cbe_cond_next2763:
  llvm_cbe_tmp2765 = *llvm_cbe_iftmp_509_0;
  llvm_cbe_tmp2766 = *(&llvm_cbe_length_prevgroup);
  *llvm_cbe_iftmp_509_0 = ((llvm_cbe_tmp2765 + ((unsigned int )-6)) + ((llvm_cbe_tmp2766 + ((unsigned int )7)) * llvm_cbe_repeat_max_16));
  llvm_cbe_bralink_11_ph__PHI_TEMPORARY = llvm_cbe_bralink_813241_0;   /* for PHI node */
  llvm_cbe_save_hwm_9_ph__PHI_TEMPORARY = llvm_cbe_save_hwm_313252_0;   /* for PHI node */
  llvm_cbe_code105_22_ph__PHI_TEMPORARY = llvm_cbe_code105_1813259_0;   /* for PHI node */
  goto llvm_cbe_bb2970_preheader;

llvm_cbe_cond_true2751:
  llvm_cbe_repeat_max_16 = llvm_cbe_repeat_max_16__PHI_TEMPORARY;
  llvm_cbe_bralink_813241_0 = llvm_cbe_bralink_813241_0__PHI_TEMPORARY;
  llvm_cbe_save_hwm_313252_0 = llvm_cbe_save_hwm_313252_0__PHI_TEMPORARY;
  llvm_cbe_previous_913258_0 = llvm_cbe_previous_913258_0__PHI_TEMPORARY;
  llvm_cbe_code105_1813259_0 = llvm_cbe_code105_1813259_0__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_1213269_0 = llvm_cbe_reqbyte103_1213269_0__PHI_TEMPORARY;
  if (((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0)) | (((signed int )llvm_cbe_repeat_max_16) < ((signed int )((unsigned int )1))))) {
    goto llvm_cbe_bb2773;
  } else {
    goto llvm_cbe_cond_next2763;
  }

llvm_cbe_cond_next2625:
  llvm_cbe_bralink_7 = llvm_cbe_bralink_7__PHI_TEMPORARY;
  llvm_cbe_previous_8 = llvm_cbe_previous_8__PHI_TEMPORARY;
  llvm_cbe_code105_17 = llvm_cbe_code105_17__PHI_TEMPORARY;
  llvm_cbe_tmp2627 = llvm_cbe_repeat_max_9 + ((unsigned int )-1);
  if ((((signed int )llvm_cbe_tmp2627) > ((signed int )((unsigned int )-1)))) {
    llvm_cbe_repeat_max_16__PHI_TEMPORARY = llvm_cbe_tmp2627;   /* for PHI node */
    llvm_cbe_bralink_813241_0__PHI_TEMPORARY = llvm_cbe_bralink_7;   /* for PHI node */
    llvm_cbe_save_hwm_313252_0__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_previous_913258_0__PHI_TEMPORARY = llvm_cbe_previous_8;   /* for PHI node */
    llvm_cbe_code105_1813259_0__PHI_TEMPORARY = llvm_cbe_code105_17;   /* for PHI node */
    llvm_cbe_reqbyte103_1213269_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_cond_true2751;
  } else {
    llvm_cbe_repeat_max_17__PHI_TEMPORARY = llvm_cbe_tmp2627;   /* for PHI node */
    llvm_cbe_save_hwm_313252_1__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_1813259_1__PHI_TEMPORARY = llvm_cbe_code105_17;   /* for PHI node */
    llvm_cbe_reqbyte103_1213269_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_cond_false2976;
  }

llvm_cbe_cond_true2559:
  adjust_recurse(llvm_cbe_previous_25708_1, ((unsigned int )1), llvm_cbe_cd, llvm_cbe_save_hwm_79347_9);
  llvm_cbe_tmp2567 = &llvm_cbe_previous_25708_1[((unsigned int )1)];
  ltmp_8_1 = memmove(llvm_cbe_tmp2567, llvm_cbe_previous_25708_1, llvm_cbe_tmp2488);
  llvm_cbe_tmp2570 = &llvm_cbe_code105_10[((unsigned int )1)];
  *llvm_cbe_previous_25708_1 = (((unsigned char )((((unsigned char )llvm_cbe_repeat_type_012701_1)) + ((unsigned char )102))));
  llvm_cbe_bralink_7__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_previous_8__PHI_TEMPORARY = llvm_cbe_tmp2567;   /* for PHI node */
  llvm_cbe_code105_17__PHI_TEMPORARY = llvm_cbe_tmp2570;   /* for PHI node */
  goto llvm_cbe_cond_next2625;

llvm_cbe_cond_next2554:
  *llvm_cbe_code105_10 = ((unsigned char )0);
  if ((((signed int )llvm_cbe_repeat_max_9) < ((signed int )((unsigned int )2)))) {
    goto llvm_cbe_cond_true2559;
  } else {
    goto llvm_cbe_cond_false2577;
  }

llvm_cbe_cond_false2577:
  adjust_recurse(llvm_cbe_previous_25708_1, ((unsigned int )4), llvm_cbe_cd, llvm_cbe_save_hwm_79347_9);
  llvm_cbe_tmp2586 = &llvm_cbe_previous_25708_1[((unsigned int )4)];
  ltmp_9_1 = memmove(llvm_cbe_tmp2586, llvm_cbe_previous_25708_1, llvm_cbe_tmp2488);
  llvm_cbe_tmp2589 = &llvm_cbe_code105_10[((unsigned int )4)];
  *llvm_cbe_previous_25708_1 = (((unsigned char )((((unsigned char )llvm_cbe_repeat_type_012701_1)) + ((unsigned char )102))));
  *(&llvm_cbe_previous_25708_1[((unsigned int )1)]) = ((unsigned char )93);
  llvm_cbe_tmp2598 = &llvm_cbe_previous_25708_1[((unsigned int )2)];
  *llvm_cbe_tmp2598 = ((unsigned char )0);
  *(&llvm_cbe_previous_25708_1[((unsigned int )3)]) = ((unsigned char )0);
  llvm_cbe_bralink_7__PHI_TEMPORARY = llvm_cbe_tmp2598;   /* for PHI node */
  llvm_cbe_previous_8__PHI_TEMPORARY = llvm_cbe_tmp2586;   /* for PHI node */
  llvm_cbe_code105_17__PHI_TEMPORARY = llvm_cbe_tmp2589;   /* for PHI node */
  goto llvm_cbe_cond_next2625;

llvm_cbe_cond_next2746:
  llvm_cbe_repeat_max_18 = llvm_cbe_repeat_max_18__PHI_TEMPORARY;
  llvm_cbe_save_hwm_213284_1 = llvm_cbe_save_hwm_213284_1__PHI_TEMPORARY;
  llvm_cbe_code105_2013285_1 = llvm_cbe_code105_2013285_1__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_1113286_1 = llvm_cbe_reqbyte103_1113286_1__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_repeat_max_18) > ((signed int )((unsigned int )-1)))) {
    llvm_cbe_repeat_max_16__PHI_TEMPORARY = llvm_cbe_repeat_max_18;   /* for PHI node */
    llvm_cbe_bralink_813241_0__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    llvm_cbe_save_hwm_313252_0__PHI_TEMPORARY = llvm_cbe_save_hwm_213284_1;   /* for PHI node */
    llvm_cbe_previous_913258_0__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_code105_1813259_0__PHI_TEMPORARY = llvm_cbe_code105_2013285_1;   /* for PHI node */
    llvm_cbe_reqbyte103_1213269_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_1113286_1;   /* for PHI node */
    goto llvm_cbe_cond_true2751;
  } else {
    llvm_cbe_repeat_max_17__PHI_TEMPORARY = llvm_cbe_repeat_max_18;   /* for PHI node */
    llvm_cbe_save_hwm_313252_1__PHI_TEMPORARY = llvm_cbe_save_hwm_213284_1;   /* for PHI node */
    llvm_cbe_code105_1813259_1__PHI_TEMPORARY = llvm_cbe_code105_2013285_1;   /* for PHI node */
    llvm_cbe_reqbyte103_1213269_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_1113286_1;   /* for PHI node */
    goto llvm_cbe_cond_false2976;
  }

llvm_cbe_cond_true2638:
  llvm_cbe_tmp2640 = *llvm_cbe_iftmp_509_0;
  llvm_cbe_tmp2643 = *(&llvm_cbe_length_prevgroup);
  *llvm_cbe_iftmp_509_0 = (((llvm_cbe_repeat_min_9 + ((unsigned int )-1)) * llvm_cbe_tmp2643) + llvm_cbe_tmp2640);
  if ((((signed int )llvm_cbe_repeat_max_9) > ((signed int )((unsigned int )0)))) {
    llvm_cbe_save_hwm_213284_0__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_2013285_0__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_reqbyte103_1113286_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_cond_true2741;
  } else {
    llvm_cbe_repeat_max_18__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
    llvm_cbe_save_hwm_213284_1__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_2013285_1__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_reqbyte103_1113286_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_cond_next2746;
  }

llvm_cbe_cond_true2633:
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_false2647;
  } else {
    goto llvm_cbe_cond_true2638;
  }

llvm_cbe_cond_false2628:
  if ((((signed int )llvm_cbe_repeat_min_9) > ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true2633;
  } else {
    llvm_cbe_save_hwm_2__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_20__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_reqbyte103_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_1;   /* for PHI node */
    goto llvm_cbe_cond_next2736;
  }

llvm_cbe_cond_next2736:
  llvm_cbe_save_hwm_2 = llvm_cbe_save_hwm_2__PHI_TEMPORARY;
  llvm_cbe_code105_20 = llvm_cbe_code105_20__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_11 = llvm_cbe_reqbyte103_11__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_repeat_max_9) > ((signed int )((unsigned int )0)))) {
    llvm_cbe_save_hwm_213284_0__PHI_TEMPORARY = llvm_cbe_save_hwm_2;   /* for PHI node */
    llvm_cbe_code105_2013285_0__PHI_TEMPORARY = llvm_cbe_code105_20;   /* for PHI node */
    llvm_cbe_reqbyte103_1113286_0__PHI_TEMPORARY = llvm_cbe_reqbyte103_11;   /* for PHI node */
    goto llvm_cbe_cond_true2741;
  } else {
    llvm_cbe_repeat_max_18__PHI_TEMPORARY = llvm_cbe_repeat_max_9;   /* for PHI node */
    llvm_cbe_save_hwm_213284_1__PHI_TEMPORARY = llvm_cbe_save_hwm_2;   /* for PHI node */
    llvm_cbe_code105_2013285_1__PHI_TEMPORARY = llvm_cbe_code105_20;   /* for PHI node */
    llvm_cbe_reqbyte103_1113286_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_11;   /* for PHI node */
    goto llvm_cbe_cond_next2746;
  }

llvm_cbe_cond_false2647:
  llvm_cbe_reqbyte103_10 = ((((((signed int )llvm_cbe_reqbyte103_1) < ((signed int )((unsigned int )0))) & (llvm_cbe_groupsetfirstbyte_29395_9 != ((unsigned int )0)))) ? (llvm_cbe_firstbyte102_7) : (llvm_cbe_reqbyte103_1));
  if ((((signed int )llvm_cbe_repeat_min_9) > ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_bb2662_preheader;
  } else {
    llvm_cbe_save_hwm_2__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_code105_20__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_reqbyte103_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_10;   /* for PHI node */
    goto llvm_cbe_cond_next2736;
  }

llvm_cbe_cond_next2736_loopexit:
  llvm_cbe_tmp2725 = &llvm_cbe_code105_10[(llvm_cbe_code105_1913296_0_rec + llvm_cbe_tmp2488)];
  llvm_cbe_save_hwm_2__PHI_TEMPORARY = llvm_cbe_tmp2665;   /* for PHI node */
  llvm_cbe_code105_20__PHI_TEMPORARY = llvm_cbe_tmp2725;   /* for PHI node */
  llvm_cbe_reqbyte103_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_10;   /* for PHI node */
  goto llvm_cbe_cond_next2736;

  do {     /* Syntactic loop 'bb2662' to make GCC happy */
llvm_cbe_bb2662:
  llvm_cbe_indvar26622 = llvm_cbe_indvar26622__PHI_TEMPORARY;
  llvm_cbe_save_hwm_113294_0 = llvm_cbe_save_hwm_113294_0__PHI_TEMPORARY;
  llvm_cbe_code105_1913296_0_rec = llvm_cbe_indvar26622 * llvm_cbe_tmp2488;
  llvm_cbe_tmp2665 = *llvm_cbe_tmp203;
  ltmp_10_1 = memcpy((&llvm_cbe_code105_10[llvm_cbe_code105_1913296_0_rec]), llvm_cbe_previous_25708_1, llvm_cbe_tmp2488);
  if ((((unsigned char *)llvm_cbe_save_hwm_113294_0) < ((unsigned char *)llvm_cbe_tmp2665))) {
    llvm_cbe_indvar26619__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb2670;
  } else {
    goto llvm_cbe_bb2728;
  }

llvm_cbe_bb2728:
  llvm_cbe_indvar_next26623 = llvm_cbe_indvar26622 + ((unsigned int )1);
  if ((((signed int )(llvm_cbe_indvar26622 + ((unsigned int )2))) < ((signed int )llvm_cbe_repeat_min_9))) {
    llvm_cbe_indvar26622__PHI_TEMPORARY = llvm_cbe_indvar_next26623;   /* for PHI node */
    llvm_cbe_save_hwm_113294_0__PHI_TEMPORARY = llvm_cbe_tmp2665;   /* for PHI node */
    goto llvm_cbe_bb2662;
  } else {
    goto llvm_cbe_cond_next2736_loopexit;
  }

  do {     /* Syntactic loop 'bb2670' to make GCC happy */
llvm_cbe_bb2670:
  llvm_cbe_indvar26619 = llvm_cbe_indvar26619__PHI_TEMPORARY;
  llvm_cbe_hc_813305_0_rec = llvm_cbe_indvar26619 << ((unsigned int )1);
  llvm_cbe_tmp2673 = *llvm_cbe_tmp203;
  llvm_cbe_tmp2676 = *(&llvm_cbe_save_hwm_113294_0[llvm_cbe_hc_813305_0_rec]);
  llvm_cbe_tmp2680 = &llvm_cbe_save_hwm_113294_0[(llvm_cbe_hc_813305_0_rec | ((unsigned int )1))];
  llvm_cbe_tmp2681 = *llvm_cbe_tmp2680;
  *llvm_cbe_tmp2673 = (((unsigned char )(((unsigned int )(((unsigned int )((((((unsigned int )(unsigned char )llvm_cbe_tmp2676)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp2681))) + llvm_cbe_tmp2488)) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp2691 = *llvm_cbe_tmp203;
  llvm_cbe_tmp2700 = *llvm_cbe_tmp2680;
  *(&llvm_cbe_tmp2691[((unsigned int )1)]) = (((unsigned char )(llvm_cbe_tmp2700 + llvm_cbe_tmp27022703)));
  llvm_cbe_tmp2709 = *llvm_cbe_tmp203;
  *llvm_cbe_tmp203 = (&llvm_cbe_tmp2709[((unsigned int )2)]);
  if ((((unsigned char *)(&llvm_cbe_save_hwm_113294_0[(llvm_cbe_hc_813305_0_rec + ((unsigned int )2))])) < ((unsigned char *)llvm_cbe_tmp2665))) {
    llvm_cbe_indvar26619__PHI_TEMPORARY = (llvm_cbe_indvar26619 + ((unsigned int )1));   /* for PHI node */
    goto llvm_cbe_bb2670;
  } else {
    goto llvm_cbe_bb2728;
  }

  } while (1); /* end of syntactic loop 'bb2670' */
  } while (1); /* end of syntactic loop 'bb2662' */
llvm_cbe_bb2662_preheader:
  llvm_cbe_tmp27022703 = ((unsigned char )llvm_cbe_tmp2488);
  llvm_cbe_indvar26622__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_save_hwm_113294_0__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  goto llvm_cbe_bb2662;

llvm_cbe_cond_true2741:
  llvm_cbe_save_hwm_213284_0 = llvm_cbe_save_hwm_213284_0__PHI_TEMPORARY;
  llvm_cbe_code105_2013285_0 = llvm_cbe_code105_2013285_0__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_1113286_0 = llvm_cbe_reqbyte103_1113286_0__PHI_TEMPORARY;
  llvm_cbe_tmp2744 = llvm_cbe_repeat_max_9 - llvm_cbe_repeat_min_9;
  llvm_cbe_repeat_max_18__PHI_TEMPORARY = llvm_cbe_tmp2744;   /* for PHI node */
  llvm_cbe_save_hwm_213284_1__PHI_TEMPORARY = llvm_cbe_save_hwm_213284_0;   /* for PHI node */
  llvm_cbe_code105_2013285_1__PHI_TEMPORARY = llvm_cbe_code105_2013285_0;   /* for PHI node */
  llvm_cbe_reqbyte103_1113286_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_1113286_0;   /* for PHI node */
  goto llvm_cbe_cond_next2746;

  do {     /* Syntactic loop 'bb2905' to make GCC happy */
llvm_cbe_bb2905:
  llvm_cbe_indvar26613 = llvm_cbe_indvar26613__PHI_TEMPORARY;
  llvm_cbe_bralink_10 = llvm_cbe_bralink_10__PHI_TEMPORARY;
  llvm_cbe_save_hwm_8 = llvm_cbe_save_hwm_8__PHI_TEMPORARY;
  llvm_cbe_code105_21 = llvm_cbe_code105_21__PHI_TEMPORARY;
  llvm_cbe_i_9 = (llvm_cbe_repeat_max_16 - llvm_cbe_indvar26613) + ((unsigned int )-1);
  if ((((signed int )llvm_cbe_i_9) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_bb2776;
  } else {
    llvm_cbe_bralink_11_ph__PHI_TEMPORARY = llvm_cbe_bralink_10;   /* for PHI node */
    llvm_cbe_save_hwm_9_ph__PHI_TEMPORARY = llvm_cbe_save_hwm_8;   /* for PHI node */
    llvm_cbe_code105_22_ph__PHI_TEMPORARY = llvm_cbe_code105_21;   /* for PHI node */
    goto llvm_cbe_bb2970_preheader;
  }

llvm_cbe_bb2898:
  llvm_cbe_bralink_913313_1 = llvm_cbe_bralink_913313_1__PHI_TEMPORARY;
  llvm_cbe_code105_2313314_1 = llvm_cbe_code105_2313314_1__PHI_TEMPORARY;
  llvm_cbe_tmp2902 = &llvm_cbe_code105_2313314_1[llvm_cbe_tmp2488];
  llvm_cbe_indvar_next26614 = llvm_cbe_indvar26613 + ((unsigned int )1);
  llvm_cbe_indvar26613__PHI_TEMPORARY = llvm_cbe_indvar_next26614;   /* for PHI node */
  llvm_cbe_bralink_10__PHI_TEMPORARY = llvm_cbe_bralink_913313_1;   /* for PHI node */
  llvm_cbe_save_hwm_8__PHI_TEMPORARY = llvm_cbe_tmp2781;   /* for PHI node */
  llvm_cbe_code105_21__PHI_TEMPORARY = llvm_cbe_tmp2902;   /* for PHI node */
  goto llvm_cbe_bb2905;

llvm_cbe_cond_next2808:
  llvm_cbe_iftmp_312_0 = llvm_cbe_iftmp_312_0__PHI_TEMPORARY;
  *llvm_cbe_tmp2796 = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_iftmp_312_0) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_21[((unsigned int )3)]) = (((unsigned char )llvm_cbe_iftmp_312_0));
  llvm_cbe_tmp2822 = &llvm_cbe_code105_21[((unsigned int )4)];
  ltmp_11_1 = memcpy(llvm_cbe_tmp2822, llvm_cbe_previous_913258_0, llvm_cbe_tmp2488);
  if ((((unsigned char *)llvm_cbe_save_hwm_8) < ((unsigned char *)llvm_cbe_tmp2781))) {
    llvm_cbe_bralink_913313_0_ph__PHI_TEMPORARY = llvm_cbe_tmp2796;   /* for PHI node */
    llvm_cbe_code105_2313314_0_ph__PHI_TEMPORARY = llvm_cbe_tmp2822;   /* for PHI node */
    goto llvm_cbe_bb2828_preheader;
  } else {
    llvm_cbe_bralink_913313_1__PHI_TEMPORARY = llvm_cbe_tmp2796;   /* for PHI node */
    llvm_cbe_code105_2313314_1__PHI_TEMPORARY = llvm_cbe_tmp2822;   /* for PHI node */
    goto llvm_cbe_bb2898;
  }

llvm_cbe_cond_true2792:
  *llvm_cbe_tmp2787 = ((unsigned char )93);
  llvm_cbe_tmp2796 = &llvm_cbe_code105_21[((unsigned int )2)];
  if ((llvm_cbe_bralink_10 == ((unsigned char *)/*NULL*/0))) {
    llvm_cbe_iftmp_312_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_next2808;
  } else {
    goto llvm_cbe_cond_true2801;
  }

llvm_cbe_bb2776:
  llvm_cbe_tmp2781 = *llvm_cbe_tmp203;
  *llvm_cbe_code105_21 = llvm_cbe_tmp2784;
  llvm_cbe_tmp2787 = &llvm_cbe_code105_21[((unsigned int )1)];
  if ((llvm_cbe_i_9 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next2823;
  } else {
    goto llvm_cbe_cond_true2792;
  }

llvm_cbe_cond_true2801:
  llvm_cbe_tmp2806 = (((unsigned int )(unsigned long)llvm_cbe_tmp2796)) - (((unsigned int )(unsigned long)llvm_cbe_bralink_10));
  llvm_cbe_iftmp_312_0__PHI_TEMPORARY = llvm_cbe_tmp2806;   /* for PHI node */
  goto llvm_cbe_cond_next2808;

llvm_cbe_cond_next2823:
  ltmp_12_1 = memcpy(llvm_cbe_tmp2787, llvm_cbe_previous_913258_0, llvm_cbe_tmp2488);
  if ((((unsigned char *)llvm_cbe_save_hwm_8) < ((unsigned char *)llvm_cbe_tmp2781))) {
    llvm_cbe_bralink_913313_0_ph__PHI_TEMPORARY = llvm_cbe_bralink_10;   /* for PHI node */
    llvm_cbe_code105_2313314_0_ph__PHI_TEMPORARY = llvm_cbe_tmp2787;   /* for PHI node */
    goto llvm_cbe_bb2828_preheader;
  } else {
    llvm_cbe_bralink_913313_1__PHI_TEMPORARY = llvm_cbe_bralink_10;   /* for PHI node */
    llvm_cbe_code105_2313314_1__PHI_TEMPORARY = llvm_cbe_tmp2787;   /* for PHI node */
    goto llvm_cbe_bb2898;
  }

  do {     /* Syntactic loop 'bb2828' to make GCC happy */
llvm_cbe_bb2828:
  llvm_cbe_indvar = llvm_cbe_indvar__PHI_TEMPORARY;
  llvm_cbe_hc2777_713319_0_rec = llvm_cbe_indvar << ((unsigned int )1);
  llvm_cbe_tmp2832 = *llvm_cbe_tmp203;
  llvm_cbe_tmp2835 = *(&llvm_cbe_save_hwm_8[llvm_cbe_hc2777_713319_0_rec]);
  llvm_cbe_tmp2839 = &llvm_cbe_save_hwm_8[(llvm_cbe_hc2777_713319_0_rec | ((unsigned int )1))];
  llvm_cbe_tmp2840 = *llvm_cbe_tmp2839;
  *llvm_cbe_tmp2832 = (((unsigned char )(((unsigned int )(((unsigned int )(llvm_cbe_tmp2844 + (((((unsigned int )(unsigned char )llvm_cbe_tmp2835)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp2840))))) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp2859 = *llvm_cbe_tmp203;
  llvm_cbe_tmp2868 = *llvm_cbe_tmp2839;
  *(&llvm_cbe_tmp2859[((unsigned int )1)]) = (((unsigned char )(llvm_cbe_tmp2872 + llvm_cbe_tmp2868)));
  llvm_cbe_tmp2886 = *llvm_cbe_tmp203;
  *llvm_cbe_tmp203 = (&llvm_cbe_tmp2886[((unsigned int )2)]);
  if ((((unsigned char *)(&llvm_cbe_save_hwm_8[(llvm_cbe_hc2777_713319_0_rec + ((unsigned int )2))])) < ((unsigned char *)llvm_cbe_tmp2781))) {
    llvm_cbe_indvar__PHI_TEMPORARY = (llvm_cbe_indvar + ((unsigned int )1));   /* for PHI node */
    goto llvm_cbe_bb2828;
  } else {
    llvm_cbe_bralink_913313_1__PHI_TEMPORARY = llvm_cbe_bralink_913313_0_ph;   /* for PHI node */
    llvm_cbe_code105_2313314_1__PHI_TEMPORARY = llvm_cbe_code105_2313314_0_ph;   /* for PHI node */
    goto llvm_cbe_bb2898;
  }

  } while (1); /* end of syntactic loop 'bb2828' */
llvm_cbe_bb2828_preheader:
  llvm_cbe_bralink_913313_0_ph = llvm_cbe_bralink_913313_0_ph__PHI_TEMPORARY;
  llvm_cbe_code105_2313314_0_ph = llvm_cbe_code105_2313314_0_ph__PHI_TEMPORARY;
  llvm_cbe_tmp2844 = ((((llvm_cbe_i_9 == ((unsigned int )0))) ? (((unsigned int )1)) : (((unsigned int )4)))) + llvm_cbe_tmp2488;
  llvm_cbe_tmp2872 = ((unsigned char )(((((llvm_cbe_i_9 == ((unsigned int )0))) ? (((unsigned char )1)) : (((unsigned char )4)))) + llvm_cbe_tmp28702871));
  llvm_cbe_indvar__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb2828;

  } while (1); /* end of syntactic loop 'bb2905' */
llvm_cbe_bb2773:
  llvm_cbe_tmp2784 = ((unsigned char )((((unsigned char )llvm_cbe_repeat_type_012701_1)) + ((unsigned char )102)));
  llvm_cbe_tmp28702871 = ((unsigned char )llvm_cbe_tmp2488);
  llvm_cbe_indvar26613__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bralink_10__PHI_TEMPORARY = llvm_cbe_bralink_813241_0;   /* for PHI node */
  llvm_cbe_save_hwm_8__PHI_TEMPORARY = llvm_cbe_save_hwm_313252_0;   /* for PHI node */
  llvm_cbe_code105_21__PHI_TEMPORARY = llvm_cbe_code105_1813259_0;   /* for PHI node */
  goto llvm_cbe_bb2905;

llvm_cbe_cond_false2976:
  llvm_cbe_repeat_max_17 = llvm_cbe_repeat_max_17__PHI_TEMPORARY;
  llvm_cbe_save_hwm_313252_1 = llvm_cbe_save_hwm_313252_1__PHI_TEMPORARY;
  llvm_cbe_code105_1813259_1 = llvm_cbe_code105_1813259_1__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_1213269_1 = llvm_cbe_reqbyte103_1213269_1__PHI_TEMPORARY;
  llvm_cbe_tmp2980 = &llvm_cbe_code105_1813259_1[(-(llvm_cbe_ketoffset_7))];
  llvm_cbe_tmp2983 = *(&llvm_cbe_code105_1813259_1[(((unsigned int )1) - llvm_cbe_ketoffset_7)]);
  llvm_cbe_tmp2988 = *(&llvm_cbe_code105_1813259_1[(((unsigned int )2) - llvm_cbe_ketoffset_7)]);
  llvm_cbe_tmp2980_sum = (-((((((unsigned int )(unsigned char )llvm_cbe_tmp2983)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp2988))))) - llvm_cbe_ketoffset_7;
  llvm_cbe_tmp2993 = &llvm_cbe_code105_1813259_1[llvm_cbe_tmp2980_sum];
  *llvm_cbe_tmp2980 = (((unsigned char )((((unsigned char )llvm_cbe_repeat_type_012701_1)) + ((unsigned char )85))));
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_true3002;
  } else {
    llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_17;   /* for PHI node */
    llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_313252_1;   /* for PHI node */
    llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_code105_1813259_1;   /* for PHI node */
    llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1213269_1;   /* for PHI node */
    goto llvm_cbe_cond_next3051;
  }

llvm_cbe_cond_true3002:
  llvm_cbe_tmp3004 = *llvm_cbe_tmp2993;
  if ((llvm_cbe_tmp3004 == ((unsigned char )92))) {
    llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_17;   /* for PHI node */
    llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_313252_1;   /* for PHI node */
    llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_code105_1813259_1;   /* for PHI node */
    llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1213269_1;   /* for PHI node */
    goto llvm_cbe_cond_next3051;
  } else {
    llvm_cbe_scode_5_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3010;
  }

llvm_cbe_cond_true3018:
  llvm_cbe_tmp3020 = *llvm_cbe_tmp2993;
  *llvm_cbe_tmp2993 = (((unsigned char )(llvm_cbe_tmp3020 + ((unsigned char )3))));
  llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_17;   /* for PHI node */
  llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_313252_1;   /* for PHI node */
  llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_code105_1813259_1;   /* for PHI node */
  llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1213269_1;   /* for PHI node */
  goto llvm_cbe_cond_next3051;

  do {     /* Syntactic loop 'bb3010' to make GCC happy */
llvm_cbe_bb3010:
  llvm_cbe_scode_5_rec = llvm_cbe_scode_5_rec__PHI_TEMPORARY;
  llvm_cbe_tmp2993_sum26701 = llvm_cbe_tmp2980_sum + llvm_cbe_scode_5_rec;
  llvm_cbe_tmp3014 = could_be_empty_branch((&llvm_cbe_code105_1813259_1[llvm_cbe_tmp2993_sum26701]), llvm_cbe_tmp2980);
  if ((llvm_cbe_tmp3014 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next3023;
  } else {
    goto llvm_cbe_cond_true3018;
  }

llvm_cbe_cond_next3023:
  llvm_cbe_tmp3026 = *(&llvm_cbe_code105_1813259_1[(llvm_cbe_tmp2993_sum26701 + ((unsigned int )1))]);
  llvm_cbe_tmp3031 = *(&llvm_cbe_code105_1813259_1[(llvm_cbe_tmp2993_sum26701 + ((unsigned int )2))]);
  llvm_cbe_tmp3035_rec = llvm_cbe_scode_5_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp3026)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp3031)));
  llvm_cbe_tmp3037 = *(&llvm_cbe_code105_1813259_1[(llvm_cbe_tmp2980_sum + llvm_cbe_tmp3035_rec)]);
  if ((llvm_cbe_tmp3037 == ((unsigned char )83))) {
    llvm_cbe_scode_5_rec__PHI_TEMPORARY = llvm_cbe_tmp3035_rec;   /* for PHI node */
    goto llvm_cbe_bb3010;
  } else {
    llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_17;   /* for PHI node */
    llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_313252_1;   /* for PHI node */
    llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_code105_1813259_1;   /* for PHI node */
    llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1213269_1;   /* for PHI node */
    goto llvm_cbe_cond_next3051;
  }

  } while (1); /* end of syntactic loop 'bb3010' */
llvm_cbe_cond_next3051_loopexit:
  llvm_cbe_tmp2959 = &llvm_cbe_code105_22_ph[(llvm_cbe_code105_2226489_rec + ((unsigned int )3))];
  llvm_cbe_repeat_max_15__PHI_TEMPORARY = llvm_cbe_repeat_max_16;   /* for PHI node */
  llvm_cbe_save_hwm_4__PHI_TEMPORARY = llvm_cbe_save_hwm_9_ph;   /* for PHI node */
  llvm_cbe_code105_12__PHI_TEMPORARY = llvm_cbe_tmp2959;   /* for PHI node */
  llvm_cbe_reqbyte103_8__PHI_TEMPORARY = llvm_cbe_reqbyte103_1213269_0;   /* for PHI node */
  goto llvm_cbe_cond_next3051;

  do {     /* Syntactic loop 'bb2911' to make GCC happy */
llvm_cbe_bb2911:
  llvm_cbe_indvar26616 = llvm_cbe_indvar26616__PHI_TEMPORARY;
  llvm_cbe_bralink_1126488 = llvm_cbe_bralink_1126488__PHI_TEMPORARY;
  llvm_cbe_code105_2226489_rec = llvm_cbe_indvar26616 * ((unsigned int )3);
  llvm_cbe_code105_2226489 = &llvm_cbe_code105_22_ph[llvm_cbe_code105_2226489_rec];
  llvm_cbe_tmp2917 = (((unsigned int )(unsigned long)llvm_cbe_code105_2226489)) - (((unsigned int )(unsigned long)llvm_cbe_bralink_1126488));
  llvm_cbe_tmp2918 = llvm_cbe_tmp2917 + ((unsigned int )1);
  llvm_cbe_tmp2924 = &llvm_cbe_code105_22_ph[(llvm_cbe_code105_2226489_rec - llvm_cbe_tmp2917)];
  llvm_cbe_tmp2925 = *llvm_cbe_tmp2924;
  llvm_cbe_tmp2929 = &llvm_cbe_code105_22_ph[(llvm_cbe_code105_2226489_rec + (((unsigned int )1) - llvm_cbe_tmp2917))];
  llvm_cbe_tmp2930 = *llvm_cbe_tmp2929;
  llvm_cbe_tmp2932 = ((((unsigned int )(unsigned char )llvm_cbe_tmp2925)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp2930));
  if ((llvm_cbe_tmp2932 == ((unsigned int )0))) {
    llvm_cbe_iftmp_320_0__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    goto llvm_cbe_cond_next2943;
  } else {
    goto llvm_cbe_cond_true2937;
  }

llvm_cbe_cond_next2943:
  llvm_cbe_iftmp_320_0 = llvm_cbe_iftmp_320_0__PHI_TEMPORARY;
  *llvm_cbe_code105_2226489 = ((unsigned char )84);
  llvm_cbe_tmp29492950 = ((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp2918) >> ((unsigned int )((unsigned int )8))))));
  *(&llvm_cbe_code105_22_ph[(llvm_cbe_code105_2226489_rec + ((unsigned int )1))]) = llvm_cbe_tmp29492950;
  llvm_cbe_tmp29532954 = ((unsigned char )llvm_cbe_tmp2918);
  *(&llvm_cbe_code105_22_ph[(llvm_cbe_code105_2226489_rec + ((unsigned int )2))]) = llvm_cbe_tmp29532954;
  *llvm_cbe_tmp2924 = llvm_cbe_tmp29492950;
  *llvm_cbe_tmp2929 = llvm_cbe_tmp29532954;
  llvm_cbe_indvar_next26617 = llvm_cbe_indvar26616 + ((unsigned int )1);
  if ((llvm_cbe_iftmp_320_0 == ((unsigned char *)/*NULL*/0))) {
    goto llvm_cbe_cond_next3051_loopexit;
  } else {
    llvm_cbe_indvar26616__PHI_TEMPORARY = llvm_cbe_indvar_next26617;   /* for PHI node */
    llvm_cbe_bralink_1126488__PHI_TEMPORARY = llvm_cbe_iftmp_320_0;   /* for PHI node */
    goto llvm_cbe_bb2911;
  }

llvm_cbe_cond_true2937:
  llvm_cbe_tmp2941 = &llvm_cbe_bralink_1126488[(-(llvm_cbe_tmp2932))];
  llvm_cbe_iftmp_320_0__PHI_TEMPORARY = llvm_cbe_tmp2941;   /* for PHI node */
  goto llvm_cbe_cond_next2943;

  } while (1); /* end of syntactic loop 'bb2911' */
llvm_cbe_bb3087:
  llvm_cbe_tmp3090 = *(&llvm_cbe_tempcode);
  llvm_cbe_tmp3092 = (((unsigned int )(unsigned long)llvm_cbe_code105_1213058_0)) - (((unsigned int )(unsigned long)llvm_cbe_tmp3090));
  if ((((signed int )llvm_cbe_tmp3092) > ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true3097;
  } else {
    llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
    llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
    llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
    llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
    goto llvm_cbe_END_REPEAT;
  }

llvm_cbe_cond_true3056:
  llvm_cbe_repeat_max_12 = llvm_cbe_repeat_max_12__PHI_TEMPORARY;
  llvm_cbe_save_hwm_413043_0 = llvm_cbe_save_hwm_413043_0__PHI_TEMPORARY;
  llvm_cbe_code105_1213058_0 = llvm_cbe_code105_1213058_0__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_813077_0 = llvm_cbe_reqbyte103_813077_0__PHI_TEMPORARY;
  llvm_cbe_tmp3058 = *(&llvm_cbe_tempcode);
  llvm_cbe_tmp3059 = *llvm_cbe_tmp3058;
  switch (llvm_cbe_tmp3059) {
  default:
    goto llvm_cbe_bb3087;
;
  case ((unsigned char )38):
    goto llvm_cbe_bb3078;
    break;
  case ((unsigned char )64):
    goto llvm_cbe_bb3078;
    break;
  case ((unsigned char )51):
    goto llvm_cbe_bb3078;
    break;
  }
llvm_cbe_bb3078:
  llvm_cbe_tmp3084 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp3059))]);
  *(&llvm_cbe_tempcode) = (&llvm_cbe_tmp3058[(((unsigned int )(unsigned char )llvm_cbe_tmp3084))]);
  goto llvm_cbe_bb3087;

llvm_cbe_bb3101:
  *llvm_cbe_tmp3090 = ((unsigned char )39);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_cond_true3097:
  llvm_cbe_tmp3099 = *llvm_cbe_tmp3090;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp3099))) {
  default:
    goto llvm_cbe_bb3125;
;
  case ((unsigned int )30):
    goto llvm_cbe_bb3101;
    break;
  case ((unsigned int )32):
    goto llvm_cbe_bb3103;
  case ((unsigned int )34):
    goto llvm_cbe_bb3105;
  case ((unsigned int )36):
    goto llvm_cbe_bb3107;
  case ((unsigned int )43):
    goto llvm_cbe_bb3117;
  case ((unsigned int )45):
    goto llvm_cbe_bb3119;
  case ((unsigned int )47):
    goto llvm_cbe_bb3121;
  case ((unsigned int )49):
    goto llvm_cbe_bb3123;
  case ((unsigned int )56):
    goto llvm_cbe_bb3109;
  case ((unsigned int )58):
    goto llvm_cbe_bb3111;
  case ((unsigned int )60):
    goto llvm_cbe_bb3113;
  case ((unsigned int )62):
    goto llvm_cbe_bb3115;
  }
llvm_cbe_bb3103:
  *llvm_cbe_tmp3090 = ((unsigned char )40);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3105:
  *llvm_cbe_tmp3090 = ((unsigned char )41);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3107:
  *llvm_cbe_tmp3090 = ((unsigned char )42);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3109:
  *llvm_cbe_tmp3090 = ((unsigned char )65);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3111:
  *llvm_cbe_tmp3090 = ((unsigned char )66);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3113:
  *llvm_cbe_tmp3090 = ((unsigned char )67);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3115:
  *llvm_cbe_tmp3090 = ((unsigned char )68);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3117:
  *llvm_cbe_tmp3090 = ((unsigned char )52);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3119:
  *llvm_cbe_tmp3090 = ((unsigned char )53);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3121:
  *llvm_cbe_tmp3090 = ((unsigned char )54);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3123:
  *llvm_cbe_tmp3090 = ((unsigned char )55);
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_code105_1213058_0;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_bb3125:
  ltmp_13_1 = memmove((&llvm_cbe_tmp3090[((unsigned int )3)]), llvm_cbe_tmp3090, llvm_cbe_tmp3092);
  llvm_cbe_tmp3134 = llvm_cbe_tmp3092 + ((unsigned int )3);
  llvm_cbe_tmp3135 = *(&llvm_cbe_tempcode);
  *llvm_cbe_tmp3135 = ((unsigned char )92);
  *(&llvm_cbe_code105_1213058_0[((unsigned int )3)]) = ((unsigned char )84);
  llvm_cbe_tmp31413142 = ((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp3134) >> ((unsigned int )((unsigned int )8))))));
  *(&llvm_cbe_code105_1213058_0[((unsigned int )4)]) = llvm_cbe_tmp31413142;
  llvm_cbe_tmp31453146 = ((unsigned char )llvm_cbe_tmp3134);
  *(&llvm_cbe_code105_1213058_0[((unsigned int )5)]) = llvm_cbe_tmp31453146;
  llvm_cbe_tmp3151 = &llvm_cbe_code105_1213058_0[((unsigned int )6)];
  llvm_cbe_tmp3152 = *(&llvm_cbe_tempcode);
  *(&llvm_cbe_tmp3152[((unsigned int )1)]) = llvm_cbe_tmp31413142;
  llvm_cbe_tmp3157 = *(&llvm_cbe_tempcode);
  *(&llvm_cbe_tmp3157[((unsigned int )2)]) = llvm_cbe_tmp31453146;
  llvm_cbe_repeat_max_13__PHI_TEMPORARY = llvm_cbe_repeat_max_12;   /* for PHI node */
  llvm_cbe_save_hwm_5__PHI_TEMPORARY = llvm_cbe_save_hwm_413043_0;   /* for PHI node */
  llvm_cbe_code105_13__PHI_TEMPORARY = llvm_cbe_tmp3151;   /* for PHI node */
  llvm_cbe_reqbyte103_9__PHI_TEMPORARY = llvm_cbe_reqbyte103_813077_0;   /* for PHI node */
  goto llvm_cbe_END_REPEAT;

llvm_cbe_cond_next3774:
  *llvm_cbe_tmp3737 = (((unsigned char )llvm_cbe_n_5_lcssa));
  llvm_cbe_tmp3780 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3784 = *llvm_cbe_tmp3783;
  *(&llvm_cbe_code105_10[((unsigned int )2)]) = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)(&llvm_cbe_tmp3780[((unsigned int )1)]))) - (((unsigned int )(unsigned long)llvm_cbe_tmp3784)))) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp3792 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3797 = *llvm_cbe_tmp3783;
  *(&llvm_cbe_code105_10[((unsigned int )3)]) = (((unsigned char )((((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_tmp3792)) + ((unsigned char )1)))) - (((unsigned char )(unsigned long)llvm_cbe_tmp3797)))));
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = ((unsigned char )0);
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = ((unsigned char )0);
  llvm_cbe_tmp3810 = &llvm_cbe_code105_10[((unsigned int )6)];
  llvm_cbe_tmp519613366 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613366[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp3810;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next3767:
  if ((((signed int )llvm_cbe_n_5_lcssa) > ((signed int )((unsigned int )255)))) {
    goto llvm_cbe_cond_true3772;
  } else {
    goto llvm_cbe_cond_next3774;
  }

llvm_cbe_bb3759:
  llvm_cbe_n_5_lcssa = llvm_cbe_n_5_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp3760 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3761 = *llvm_cbe_tmp3760;
  if ((llvm_cbe_tmp3761 == ((unsigned char )41))) {
    goto llvm_cbe_cond_next3767;
  } else {
    goto llvm_cbe_cond_true3765;
  }

llvm_cbe_bb3733:
  *llvm_cbe_code105_10 = ((unsigned char )82);
  llvm_cbe_tmp3737 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_tmp374726477 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp374826478 = &llvm_cbe_tmp374726477[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp374826478;
  llvm_cbe_tmp375026479 = *llvm_cbe_tmp374826478;
  llvm_cbe_tmp375326482 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp375026479))]);
  if (((((unsigned char )(llvm_cbe_tmp375326482 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_n_5_lcssa__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3759;
  } else {
    llvm_cbe_n_526476__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3738;
  }

llvm_cbe_cond_true3184:
  llvm_cbe_tmp3187 = &llvm_cbe_tmp3177[((unsigned int )2)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3187;
  llvm_cbe_tmp3189 = *llvm_cbe_tmp3187;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp3189))) {
  default:
    goto llvm_cbe_OTHER_CHAR_AFTER_QUERY;
;
  case ((unsigned int )33):
    goto llvm_cbe_bb3695;
  case ((unsigned int )35):
    llvm_cbe_tmp3195_pn__PHI_TEMPORARY = llvm_cbe_tmp3187;   /* for PHI node */
    goto llvm_cbe_bb3197;
    break;
  case ((unsigned int )38):
    llvm_cbe_terminator_7__PHI_TEMPORARY = ((unsigned int )41);   /* for PHI node */
    llvm_cbe_is_recurse_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    llvm_cbe_save_hwm_10__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_zerofirstbyte_4__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_4__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_13__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    goto llvm_cbe_NAMED_REF_OR_RECURSE;
  case ((unsigned int )39):
    goto llvm_cbe_bb3840;
  case ((unsigned int )40):
    goto llvm_cbe_bb3224;
  case ((unsigned int )43):
    goto llvm_cbe_bb4177;
  case ((unsigned int )45):
    goto llvm_cbe_bb4177;
  case ((unsigned int )48):
    goto llvm_cbe_bb4177;
  case ((unsigned int )49):
    goto llvm_cbe_bb4177;
  case ((unsigned int )50):
    goto llvm_cbe_bb4177;
  case ((unsigned int )51):
    goto llvm_cbe_bb4177;
  case ((unsigned int )52):
    goto llvm_cbe_bb4177;
  case ((unsigned int )53):
    goto llvm_cbe_bb4177;
  case ((unsigned int )54):
    goto llvm_cbe_bb4177;
  case ((unsigned int )55):
    goto llvm_cbe_bb4177;
  case ((unsigned int )56):
    goto llvm_cbe_bb4177;
  case ((unsigned int )57):
    goto llvm_cbe_bb4177;
  case ((unsigned int )58):
    llvm_cbe_reset_bracount132_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3221;
  case ((unsigned int )60):
    goto llvm_cbe_bb3698;
  case ((unsigned int )61):
    goto llvm_cbe_bb3692;
  case ((unsigned int )62):
    goto llvm_cbe_bb3730;
  case ((unsigned int )67):
    goto llvm_cbe_bb3733;
  case ((unsigned int )80):
    goto llvm_cbe_bb3811;
  case ((unsigned int )82):
    goto llvm_cbe_bb4163;
  case ((unsigned int )124):
    goto llvm_cbe_bb3220;
  }
llvm_cbe_bb3172:
  llvm_cbe_tmp3176 = *llvm_cbe_tmp203;
  llvm_cbe_tmp3177 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3178 = &llvm_cbe_tmp3177[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3178;
  llvm_cbe_tmp3180 = *llvm_cbe_tmp3178;
  if ((llvm_cbe_tmp3180 == ((unsigned char )63))) {
    goto llvm_cbe_cond_true3184;
  } else {
    goto llvm_cbe_cond_false4629;
  }

  do {     /* Syntactic loop 'bb3738' to make GCC happy */
llvm_cbe_bb3738:
  llvm_cbe_n_526476 = llvm_cbe_n_526476__PHI_TEMPORARY;
  llvm_cbe_tmp3741 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3742 = *llvm_cbe_tmp3741;
  llvm_cbe_tmp3745 = ((llvm_cbe_n_526476 * ((unsigned int )10)) + ((unsigned int )-48)) + (((unsigned int )(unsigned char )llvm_cbe_tmp3742));
  llvm_cbe_tmp3748 = &llvm_cbe_tmp3741[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3748;
  llvm_cbe_tmp3750 = *llvm_cbe_tmp3748;
  llvm_cbe_tmp3753 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp3750))]);
  if (((((unsigned char )(llvm_cbe_tmp3753 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_n_5_lcssa__PHI_TEMPORARY = llvm_cbe_tmp3745;   /* for PHI node */
    goto llvm_cbe_bb3759;
  } else {
    llvm_cbe_n_526476__PHI_TEMPORARY = llvm_cbe_tmp3745;   /* for PHI node */
    goto llvm_cbe_bb3738;
  }

  } while (1); /* end of syntactic loop 'bb3738' */
llvm_cbe_cond_true4480:
  llvm_cbe_tmp519613479 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613479[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_11;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp4475;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_8;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_8;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next4435:
  llvm_cbe_called_6 = llvm_cbe_called_6__PHI_TEMPORARY;
  *llvm_cbe_code105_10 = ((unsigned char )92);
  *(&llvm_cbe_code105_10[((unsigned int )1)]) = ((unsigned char )0);
  *(&llvm_cbe_code105_10[((unsigned int )2)]) = ((unsigned char )6);
  *(&llvm_cbe_code105_10[((unsigned int )3)]) = ((unsigned char )81);
  llvm_cbe_tmp4448 = *llvm_cbe_tmp4563;
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)llvm_cbe_called_6)) - (((unsigned int )(unsigned long)llvm_cbe_tmp4448)))) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp4460 = *llvm_cbe_tmp4563;
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = (((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_called_6)) - (((unsigned char )(unsigned long)llvm_cbe_tmp4460)))));
  *(&llvm_cbe_code105_10[((unsigned int )6)]) = ((unsigned char )84);
  *(&llvm_cbe_code105_10[((unsigned int )7)]) = ((unsigned char )0);
  *(&llvm_cbe_code105_10[((unsigned int )8)]) = ((unsigned char )6);
  llvm_cbe_tmp4475 = &llvm_cbe_code105_10[((unsigned int )9)];
  *(&llvm_cbe_length_prevgroup) = ((unsigned int )9);
  if ((llvm_cbe_firstbyte102_15 == ((unsigned int )-2))) {
    goto llvm_cbe_cond_true4480;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_save_hwm_11;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_tmp4475;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_8;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_8;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_15;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_HANDLE_RECURSION:
  llvm_cbe_recno_12 = llvm_cbe_recno_12__PHI_TEMPORARY;
  llvm_cbe_save_hwm_11 = llvm_cbe_save_hwm_11__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_8 = llvm_cbe_zerofirstbyte_8__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_8 = llvm_cbe_zeroreqbyte_8__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_15 = llvm_cbe_firstbyte102_15__PHI_TEMPORARY;
  llvm_cbe_tmp4289 = *llvm_cbe_tmp4563;
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_true4294;
  } else {
    llvm_cbe_called_6__PHI_TEMPORARY = llvm_cbe_tmp4289;   /* for PHI node */
    goto llvm_cbe_cond_next4435;
  }

llvm_cbe_cond_next4158:
  llvm_cbe_recno_11 = llvm_cbe_recno_11__PHI_TEMPORARY;
  if ((llvm_cbe_is_recurse_013390_0_ph == ((unsigned int )0))) {
    llvm_cbe_recno_13__PHI_TEMPORARY = llvm_cbe_recno_11;   /* for PHI node */
    llvm_cbe_save_hwm_14__PHI_TEMPORARY = llvm_cbe_save_hwm_1013396_0_ph;   /* for PHI node */
    llvm_cbe_zerofirstbyte_5__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_413398_0_ph;   /* for PHI node */
    llvm_cbe_zeroreqbyte_5__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_413400_0_ph;   /* for PHI node */
    llvm_cbe_firstbyte102_14__PHI_TEMPORARY = llvm_cbe_firstbyte102_1313402_0_ph;   /* for PHI node */
    goto llvm_cbe_HANDLE_REFERENCE;
  } else {
    llvm_cbe_recno_12__PHI_TEMPORARY = llvm_cbe_recno_11;   /* for PHI node */
    llvm_cbe_save_hwm_11__PHI_TEMPORARY = llvm_cbe_save_hwm_1013396_0_ph;   /* for PHI node */
    llvm_cbe_zerofirstbyte_8__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_413398_0_ph;   /* for PHI node */
    llvm_cbe_zeroreqbyte_8__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_413400_0_ph;   /* for PHI node */
    llvm_cbe_firstbyte102_15__PHI_TEMPORARY = llvm_cbe_firstbyte102_1313402_0_ph;   /* for PHI node */
    goto llvm_cbe_HANDLE_RECURSION;
  }

llvm_cbe_cond_next4068:
  if ((((signed int )llvm_cbe_tmp4053) > ((signed int )((unsigned int )32)))) {
    goto llvm_cbe_cond_true4073;
  } else {
    llvm_cbe_recno_11__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_next4158;
  }

llvm_cbe_cond_true4058:
  llvm_cbe_tmp4059 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4060 = *llvm_cbe_tmp4059;
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp4060)) == llvm_cbe_terminator_713369_0_ph)) {
    goto llvm_cbe_cond_next4068;
  } else {
    goto llvm_cbe_cond_true4066;
  }

llvm_cbe_bb4048:
  llvm_cbe_tmp4032_lcssa = llvm_cbe_tmp4032_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp4053 = (((unsigned int )(unsigned long)llvm_cbe_tmp4032_lcssa)) - (((unsigned int )(unsigned long)llvm_cbe_tmp402913416_0_ph));
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_false4076;
  } else {
    goto llvm_cbe_cond_true4058;
  }

llvm_cbe_bb4034_preheader:
  llvm_cbe_terminator_713369_0_ph = llvm_cbe_terminator_713369_0_ph__PHI_TEMPORARY;
  llvm_cbe_is_recurse_013390_0_ph = llvm_cbe_is_recurse_013390_0_ph__PHI_TEMPORARY;
  llvm_cbe_save_hwm_1013396_0_ph = llvm_cbe_save_hwm_1013396_0_ph__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_413398_0_ph = llvm_cbe_zerofirstbyte_413398_0_ph__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_413400_0_ph = llvm_cbe_zeroreqbyte_413400_0_ph__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_1313402_0_ph = llvm_cbe_firstbyte102_1313402_0_ph__PHI_TEMPORARY;
  llvm_cbe_tmp402913416_0_ph = llvm_cbe_tmp402913416_0_ph__PHI_TEMPORARY;
  llvm_cbe_storemerge5542_ph = llvm_cbe_storemerge5542_ph__PHI_TEMPORARY;
  *(&llvm_cbe_ptr106) = llvm_cbe_storemerge5542_ph;
  llvm_cbe_tmp403726541 = *llvm_cbe_tmp435;
  llvm_cbe_tmp403926543 = *llvm_cbe_storemerge5542_ph;
  llvm_cbe_tmp404226546 = *(&llvm_cbe_tmp403726541[(((unsigned int )(unsigned char )llvm_cbe_tmp403926543))]);
  if (((((unsigned char )(llvm_cbe_tmp404226546 & ((unsigned char )16)))) == ((unsigned char )0))) {
    llvm_cbe_tmp4032_lcssa__PHI_TEMPORARY = llvm_cbe_storemerge5542_ph;   /* for PHI node */
    goto llvm_cbe_bb4048;
  } else {
    llvm_cbe_tmp403226550__PHI_TEMPORARY = llvm_cbe_storemerge5542_ph;   /* for PHI node */
    goto llvm_cbe_bb4031;
  }

llvm_cbe_bb3826:
  llvm_cbe_tmp38293830 = ((unsigned int )(bool )(llvm_cbe_tmp3815 == ((unsigned char )62)));
  llvm_cbe_tmp402913426 = &llvm_cbe_tmp3177[((unsigned int )4)];
  llvm_cbe_terminator_713369_0_ph__PHI_TEMPORARY = ((unsigned int )41);   /* for PHI node */
  llvm_cbe_is_recurse_013390_0_ph__PHI_TEMPORARY = llvm_cbe_tmp38293830;   /* for PHI node */
  llvm_cbe_save_hwm_1013396_0_ph__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
  llvm_cbe_zerofirstbyte_413398_0_ph__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_413400_0_ph__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_firstbyte102_1313402_0_ph__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_tmp402913416_0_ph__PHI_TEMPORARY = llvm_cbe_tmp402913426;   /* for PHI node */
  llvm_cbe_storemerge5542_ph__PHI_TEMPORARY = llvm_cbe_tmp402913426;   /* for PHI node */
  goto llvm_cbe_bb4034_preheader;

llvm_cbe_bb3811:
  llvm_cbe_tmp3813 = &llvm_cbe_tmp3177[((unsigned int )3)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3813;
  llvm_cbe_tmp3815 = *llvm_cbe_tmp3813;
  switch (llvm_cbe_tmp3815) {
  default:
    goto llvm_cbe_cond_true3837;
;
  case ((unsigned char )61):
    goto llvm_cbe_bb3826;
    break;
  case ((unsigned char )62):
    goto llvm_cbe_bb3826;
    break;
  case ((unsigned char )60):
    goto llvm_cbe_bb3840;
  }
llvm_cbe_NAMED_REF_OR_RECURSE:
  llvm_cbe_terminator_7 = llvm_cbe_terminator_7__PHI_TEMPORARY;
  llvm_cbe_is_recurse_0 = llvm_cbe_is_recurse_0__PHI_TEMPORARY;
  llvm_cbe_save_hwm_10 = llvm_cbe_save_hwm_10__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_4 = llvm_cbe_zerofirstbyte_4__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_4 = llvm_cbe_zeroreqbyte_4__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_13 = llvm_cbe_firstbyte102_13__PHI_TEMPORARY;
  llvm_cbe_tmp4028 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4029 = &llvm_cbe_tmp4028[((unsigned int )1)];
  llvm_cbe_terminator_713369_0_ph__PHI_TEMPORARY = llvm_cbe_terminator_7;   /* for PHI node */
  llvm_cbe_is_recurse_013390_0_ph__PHI_TEMPORARY = llvm_cbe_is_recurse_0;   /* for PHI node */
  llvm_cbe_save_hwm_1013396_0_ph__PHI_TEMPORARY = llvm_cbe_save_hwm_10;   /* for PHI node */
  llvm_cbe_zerofirstbyte_413398_0_ph__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_4;   /* for PHI node */
  llvm_cbe_zeroreqbyte_413400_0_ph__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_4;   /* for PHI node */
  llvm_cbe_firstbyte102_1313402_0_ph__PHI_TEMPORARY = llvm_cbe_firstbyte102_13;   /* for PHI node */
  llvm_cbe_tmp402913416_0_ph__PHI_TEMPORARY = llvm_cbe_tmp4029;   /* for PHI node */
  llvm_cbe_storemerge5542_ph__PHI_TEMPORARY = llvm_cbe_tmp4029;   /* for PHI node */
  goto llvm_cbe_bb4034_preheader;

llvm_cbe_bb4971:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp4950;
  llvm_cbe_tmp4975 = *llvm_cbe_tmp4950;
  switch (llvm_cbe_tmp4975) {
  default:
    goto llvm_cbe_cond_false4986;
;
  case ((unsigned char )60):
    llvm_cbe_terminator_7__PHI_TEMPORARY = ((unsigned int )62);   /* for PHI node */
    llvm_cbe_is_recurse_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_save_hwm_10__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_4__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
    llvm_cbe_zeroreqbyte_4__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_firstbyte102_13__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
    goto llvm_cbe_NAMED_REF_OR_RECURSE;
  case ((unsigned char )39):
    goto llvm_cbe_cond_true4979_NAMED_REF_OR_RECURSE_crit_edge;
    break;
  }
llvm_cbe_cond_true4948:
  llvm_cbe_tmp4949 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4950 = &llvm_cbe_tmp4949[((unsigned int )1)];
  llvm_cbe_tmp4951 = *llvm_cbe_tmp4950;
  switch (llvm_cbe_tmp4951) {
  default:
    goto llvm_cbe_cond_next4993;
;
  case ((unsigned char )39):
    goto llvm_cbe_bb4971;
    break;
  case ((unsigned char )60):
    goto llvm_cbe_bb4971;
    break;
  case ((unsigned char )123):
    goto llvm_cbe_bb4971;
    break;
  }
llvm_cbe_cond_next4941:
  llvm_cbe_firstbyte102_12 = llvm_cbe_firstbyte102_12__PHI_TEMPORARY;
  if ((llvm_cbe_tmp4877 == ((unsigned int )-26))) {
    goto llvm_cbe_cond_true4948;
  } else {
    goto llvm_cbe_cond_next4993;
  }

llvm_cbe_cond_next4921:
  if ((llvm_cbe_firstbyte102_39829_9 == ((unsigned int )-2))) {
    goto llvm_cbe_cond_true4926;
  } else {
    llvm_cbe_firstbyte102_12__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    goto llvm_cbe_cond_next4941;
  }

llvm_cbe_cond_true4889:
  switch (llvm_cbe_tmp4877) {
  default:
    goto llvm_cbe_cond_next4921;
;
  case ((unsigned int )-25):
    goto llvm_cbe_cond_true4894;
    break;
  case ((unsigned int )-24):
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }
llvm_cbe_cond_next4884:
  if ((((signed int )llvm_cbe_tmp4877) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true4889;
  } else {
    llvm_cbe_storemerge26707_in__PHI_TEMPORARY = llvm_cbe_tmp4877;   /* for PHI node */
    llvm_cbe_last_code_15771_4__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
    llvm_cbe_options_addr_25789_11__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_79347_11__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_11__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_options104_39466_11__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_11__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_11__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_39909_11__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    llvm_cbe_previous_callout_3__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_inescq_6__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_8__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_after_manual_callout_3__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
    goto llvm_cbe_ONE_CHAR;
  }

llvm_cbe_bb4870:
  llvm_cbe_tmp4871 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_tempptr) = llvm_cbe_tmp4871;
  llvm_cbe_tmp4874 = *llvm_cbe_tmp24;
  llvm_cbe_tmp4877 = check_escape((&llvm_cbe_ptr106), llvm_cbe_errorcodeptr, llvm_cbe_tmp4874, llvm_cbe_options104_39466_9, ((unsigned int )0));
  llvm_cbe_tmp4879 = *llvm_cbe_errorcodeptr;
  if ((llvm_cbe_tmp4879 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next4884;
  } else {
    llvm_cbe_slot_913437_0_us61_12__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_12__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    goto llvm_cbe_FAILED;
  }

llvm_cbe_cond_true4926:
  if ((((unsigned int )(((unsigned int )-6) - llvm_cbe_tmp4877)) < ((unsigned int )((unsigned int )16)))) {
    goto llvm_cbe_cond_true4938;
  } else {
    llvm_cbe_firstbyte102_12__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    goto llvm_cbe_cond_next4941;
  }

llvm_cbe_cond_true4938:
  llvm_cbe_firstbyte102_12__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  goto llvm_cbe_cond_next4941;

llvm_cbe_cond_true4979_NAMED_REF_OR_RECURSE_crit_edge:
  llvm_cbe_terminator_7__PHI_TEMPORARY = ((unsigned int )39);   /* for PHI node */
  llvm_cbe_is_recurse_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_save_hwm_10__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_4__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  llvm_cbe_zeroreqbyte_4__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_13__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  goto llvm_cbe_NAMED_REF_OR_RECURSE;

llvm_cbe_cond_false4986:
  llvm_cbe_terminator_7__PHI_TEMPORARY = ((unsigned int )125);   /* for PHI node */
  llvm_cbe_is_recurse_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_save_hwm_10__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_4__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  llvm_cbe_zeroreqbyte_4__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_13__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  goto llvm_cbe_NAMED_REF_OR_RECURSE;

  do {     /* Syntactic loop 'bb4031' to make GCC happy */
llvm_cbe_bb4031:
  llvm_cbe_tmp403226550 = llvm_cbe_tmp403226550__PHI_TEMPORARY;
  llvm_cbe_tmp4033 = &llvm_cbe_tmp403226550[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp4033;
  llvm_cbe_tmp4039 = *llvm_cbe_tmp4033;
  llvm_cbe_tmp4042 = *(&llvm_cbe_tmp403726541[(((unsigned int )(unsigned char )llvm_cbe_tmp4039))]);
  if (((((unsigned char )(llvm_cbe_tmp4042 & ((unsigned char )16)))) == ((unsigned char )0))) {
    llvm_cbe_tmp4032_lcssa__PHI_TEMPORARY = llvm_cbe_tmp4033;   /* for PHI node */
    goto llvm_cbe_bb4048;
  } else {
    llvm_cbe_tmp403226550__PHI_TEMPORARY = llvm_cbe_tmp4033;   /* for PHI node */
    goto llvm_cbe_bb4031;
  }

  } while (1); /* end of syntactic loop 'bb4031' */
llvm_cbe_cond_true4128:
  llvm_cbe_tmp4131 = *llvm_cbe_slot_1313461_1;
  llvm_cbe_tmp4136 = *(&llvm_cbe_slot_1313461_1[((unsigned int )1)]);
  llvm_cbe_tmp4138 = ((((unsigned int )(unsigned char )llvm_cbe_tmp4131)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp4136));
  llvm_cbe_recno_11__PHI_TEMPORARY = llvm_cbe_tmp4138;   /* for PHI node */
  goto llvm_cbe_cond_next4158;

llvm_cbe_bb4120:
  llvm_cbe_i3185_1413455_1 = llvm_cbe_i3185_1413455_1__PHI_TEMPORARY;
  llvm_cbe_slot_1313461_1 = llvm_cbe_slot_1313461_1__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_tmp411513469) > ((signed int )llvm_cbe_i3185_1413455_1))) {
    goto llvm_cbe_cond_true4128;
  } else {
    goto llvm_cbe_cond_false4139;
  }

llvm_cbe_cond_false4076:
  llvm_cbe_tmp4079 = *llvm_cbe_tmp3478;
  llvm_cbe_tmp411513469 = *llvm_cbe_tmp351413356;
  if ((((signed int )llvm_cbe_tmp411513469) > ((signed int )((unsigned int )0)))) {
    llvm_cbe_i3185_1413455_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_slot_1313461_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_false4092;
  } else {
    llvm_cbe_i3185_1413455_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_slot_1313461_1__PHI_TEMPORARY = llvm_cbe_tmp4079;   /* for PHI node */
    goto llvm_cbe_bb4120;
  }

  do {     /* Syntactic loop 'cond_false4092' to make GCC happy */
llvm_cbe_cond_false4092:
  llvm_cbe_i3185_1413455_0 = llvm_cbe_i3185_1413455_0__PHI_TEMPORARY;
  llvm_cbe_slot_1313461_0_rec = llvm_cbe_slot_1313461_0_rec__PHI_TEMPORARY;
  llvm_cbe_slot_1313461_0 = &llvm_cbe_tmp4079[llvm_cbe_slot_1313461_0_rec];
  llvm_cbe_tmp4097 = strncmp(llvm_cbe_tmp402913416_0_ph, (&llvm_cbe_tmp4079[(llvm_cbe_slot_1313461_0_rec + ((unsigned int )2))]), llvm_cbe_tmp4053);
  if ((llvm_cbe_tmp4097 == ((unsigned int )0))) {
    llvm_cbe_i3185_1413455_1__PHI_TEMPORARY = llvm_cbe_i3185_1413455_0;   /* for PHI node */
    llvm_cbe_slot_1313461_1__PHI_TEMPORARY = llvm_cbe_slot_1313461_0;   /* for PHI node */
    goto llvm_cbe_bb4120;
  } else {
    goto llvm_cbe_bb4112;
  }

llvm_cbe_bb4112:
  llvm_cbe_tmp4107 = *llvm_cbe_tmp3506;
  llvm_cbe_tmp4109_rec = llvm_cbe_slot_1313461_0_rec + llvm_cbe_tmp4107;
  llvm_cbe_tmp4109 = &llvm_cbe_tmp4079[llvm_cbe_tmp4109_rec];
  llvm_cbe_tmp4111 = llvm_cbe_i3185_1413455_0 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp411513469) > ((signed int )llvm_cbe_tmp4111))) {
    llvm_cbe_i3185_1413455_0__PHI_TEMPORARY = llvm_cbe_tmp4111;   /* for PHI node */
    llvm_cbe_slot_1313461_0_rec__PHI_TEMPORARY = llvm_cbe_tmp4109_rec;   /* for PHI node */
    goto llvm_cbe_cond_false4092;
  } else {
    llvm_cbe_i3185_1413455_1__PHI_TEMPORARY = llvm_cbe_tmp4111;   /* for PHI node */
    llvm_cbe_slot_1313461_1__PHI_TEMPORARY = llvm_cbe_tmp4109;   /* for PHI node */
    goto llvm_cbe_bb4120;
  }

  } while (1); /* end of syntactic loop 'cond_false4092' */
llvm_cbe_cond_false4139:
  llvm_cbe_tmp4145 = *llvm_cbe_tmp24;
  llvm_cbe_tmp4146 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4149 = find_parens(llvm_cbe_tmp4146, llvm_cbe_tmp4145, llvm_cbe_tmp402913416_0_ph, llvm_cbe_tmp4053, ((((unsigned int )(((unsigned int )llvm_cbe_options104_39466_9) >> ((unsigned int )((unsigned int )3))))) & ((unsigned int )1)));
  if ((((signed int )llvm_cbe_tmp4149) < ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true4154;
  } else {
    llvm_cbe_recno_11__PHI_TEMPORARY = llvm_cbe_tmp4149;   /* for PHI node */
    goto llvm_cbe_cond_next4158;
  }

llvm_cbe_cond_next4240:
  switch (llvm_cbe_tmp4179) {
  default:
    llvm_cbe_recno_12__PHI_TEMPORARY = llvm_cbe_recno_14_lcssa;   /* for PHI node */
    llvm_cbe_save_hwm_11__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_zerofirstbyte_8__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_8__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_15__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    goto llvm_cbe_HANDLE_RECURSION;
;
  case ((unsigned char )45):
    goto llvm_cbe_cond_true4245;
    break;
  case ((unsigned char )43):
    goto llvm_cbe_cond_true4271;
  }
llvm_cbe_bb4232:
  llvm_cbe_recno_14_lcssa = llvm_cbe_recno_14_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp4233 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4234 = *llvm_cbe_tmp4233;
  if ((llvm_cbe_tmp4234 == ((unsigned char )41))) {
    goto llvm_cbe_cond_next4240;
  } else {
    goto llvm_cbe_cond_true4238;
  }

llvm_cbe_bb4221_preheader:
  llvm_cbe_tmp422226553 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp422326554 = *llvm_cbe_tmp422226553;
  llvm_cbe_tmp422626557 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp422326554))]);
  if (((((unsigned char )(llvm_cbe_tmp422626557 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_recno_14_lcssa__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb4232;
  } else {
    llvm_cbe_recno_1426552__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb4212;
  }

llvm_cbe_bb4177:
  llvm_cbe_tmp4178 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4179 = *llvm_cbe_tmp4178;
  switch (llvm_cbe_tmp4179) {
  default:
    goto llvm_cbe_bb4221_preheader;
;
  case ((unsigned char )43):
    goto llvm_cbe_cond_true4185;
    break;
  case ((unsigned char )45):
    goto llvm_cbe_cond_true4193;
  }
llvm_cbe_bb4163:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3177[((unsigned int )3)]);
  goto llvm_cbe_bb4177;

llvm_cbe_cond_true4185:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp4178[((unsigned int )1)]);
  goto llvm_cbe_bb4221_preheader;

llvm_cbe_cond_next4207:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp4195;
  goto llvm_cbe_bb4221_preheader;

llvm_cbe_cond_true4193:
  llvm_cbe_tmp4195 = &llvm_cbe_tmp4178[((unsigned int )1)];
  llvm_cbe_tmp4196 = *llvm_cbe_tmp4195;
  llvm_cbe_tmp4199 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp4196))]);
  if (((((unsigned char )(llvm_cbe_tmp4199 & ((unsigned char )4)))) == ((unsigned char )0))) {
    goto llvm_cbe_OTHER_CHAR_AFTER_QUERY;
  } else {
    goto llvm_cbe_cond_next4207;
  }

  do {     /* Syntactic loop 'bb4212' to make GCC happy */
llvm_cbe_bb4212:
  llvm_cbe_recno_1426552 = llvm_cbe_recno_1426552__PHI_TEMPORARY;
  llvm_cbe_tmp4215 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4216 = *llvm_cbe_tmp4215;
  llvm_cbe_tmp4219 = ((llvm_cbe_recno_1426552 * ((unsigned int )10)) + ((unsigned int )-48)) + (((unsigned int )(unsigned char )llvm_cbe_tmp4216));
  llvm_cbe_tmp4220 = &llvm_cbe_tmp4215[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp4220;
  llvm_cbe_tmp4223 = *llvm_cbe_tmp4220;
  llvm_cbe_tmp4226 = *(&digitab[(((unsigned int )(unsigned char )llvm_cbe_tmp4223))]);
  if (((((unsigned char )(llvm_cbe_tmp4226 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_recno_14_lcssa__PHI_TEMPORARY = llvm_cbe_tmp4219;   /* for PHI node */
    goto llvm_cbe_bb4232;
  } else {
    llvm_cbe_recno_1426552__PHI_TEMPORARY = llvm_cbe_tmp4219;   /* for PHI node */
    goto llvm_cbe_bb4212;
  }

  } while (1); /* end of syntactic loop 'bb4212' */
llvm_cbe_cond_next4252:
  llvm_cbe_tmp4255 = *llvm_cbe_tmp24;
  llvm_cbe_tmp4258 = (llvm_cbe_tmp4255 - llvm_cbe_recno_14_lcssa) + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp4258) < ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true4263;
  } else {
    llvm_cbe_recno_12__PHI_TEMPORARY = llvm_cbe_tmp4258;   /* for PHI node */
    llvm_cbe_save_hwm_11__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_zerofirstbyte_8__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_8__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_15__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    goto llvm_cbe_HANDLE_RECURSION;
  }

llvm_cbe_cond_true4245:
  if ((llvm_cbe_recno_14_lcssa == ((unsigned int )0))) {
    goto llvm_cbe_cond_true4250;
  } else {
    goto llvm_cbe_cond_next4252;
  }

llvm_cbe_cond_next4278:
  llvm_cbe_tmp4281 = *llvm_cbe_tmp24;
  llvm_cbe_tmp4283 = llvm_cbe_tmp4281 + llvm_cbe_recno_14_lcssa;
  llvm_cbe_recno_12__PHI_TEMPORARY = llvm_cbe_tmp4283;   /* for PHI node */
  llvm_cbe_save_hwm_11__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
  llvm_cbe_zerofirstbyte_8__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_8__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_firstbyte102_15__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  goto llvm_cbe_HANDLE_RECURSION;

llvm_cbe_cond_true4271:
  if ((llvm_cbe_recno_14_lcssa == ((unsigned int )0))) {
    goto llvm_cbe_cond_true4276;
  } else {
    goto llvm_cbe_cond_next4278;
  }

llvm_cbe_cond_next4327:
  llvm_cbe_tmp4330 = *llvm_cbe_tmp4563;
  llvm_cbe_tmp4332 = &llvm_cbe_tmp4330[llvm_cbe_recno_12];
  llvm_cbe_tmp4335 = *llvm_cbe_tmp203;
  llvm_cbe_tmp4338 = &llvm_cbe_code105_10[((unsigned int )4)];
  *llvm_cbe_tmp4335 = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)llvm_cbe_tmp4338)) - (((unsigned int )(unsigned long)llvm_cbe_tmp4330)))) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp4350 = *llvm_cbe_tmp203;
  llvm_cbe_tmp4358 = *llvm_cbe_tmp4563;
  *(&llvm_cbe_tmp4350[((unsigned int )1)]) = (((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_tmp4338)) - (((unsigned char )(unsigned long)llvm_cbe_tmp4358)))));
  llvm_cbe_tmp4366 = *llvm_cbe_tmp203;
  *llvm_cbe_tmp203 = (&llvm_cbe_tmp4366[((unsigned int )2)]);
  llvm_cbe_called_6__PHI_TEMPORARY = llvm_cbe_tmp4332;   /* for PHI node */
  goto llvm_cbe_cond_next4435;

llvm_cbe_cond_true4312:
  llvm_cbe_tmp4318 = *llvm_cbe_tmp24;
  llvm_cbe_tmp4319 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4321 = find_parens(llvm_cbe_tmp4319, llvm_cbe_tmp4318, ((unsigned char *)/*NULL*/0), llvm_cbe_recno_12, ((((unsigned int )(((unsigned int )llvm_cbe_options104_39466_9) >> ((unsigned int )((unsigned int )3))))) & ((unsigned int )1)));
  if ((((signed int )llvm_cbe_tmp4321) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true4325;
  } else {
    goto llvm_cbe_cond_next4327;
  }

llvm_cbe_cond_next4307:
  llvm_cbe_called_5 = llvm_cbe_called_5__PHI_TEMPORARY;
  if ((llvm_cbe_called_5 == ((unsigned char *)/*NULL*/0))) {
    goto llvm_cbe_cond_true4312;
  } else {
    goto llvm_cbe_cond_false4370;
  }

llvm_cbe_cond_true4294:
  *llvm_cbe_code105_10 = ((unsigned char )0);
  if ((llvm_cbe_recno_12 == ((unsigned int )0))) {
    llvm_cbe_called_5__PHI_TEMPORARY = llvm_cbe_tmp4289;   /* for PHI node */
    goto llvm_cbe_cond_next4307;
  } else {
    goto llvm_cbe_cond_true4300;
  }

llvm_cbe_cond_true4300:
  llvm_cbe_tmp4303 = *llvm_cbe_tmp4563;
  llvm_cbe_tmp4306 = find_bracket(llvm_cbe_tmp4303, llvm_cbe_recno_12);
  llvm_cbe_called_5__PHI_TEMPORARY = llvm_cbe_tmp4306;   /* for PHI node */
  goto llvm_cbe_cond_next4307;

llvm_cbe_cond_false4370:
  llvm_cbe_tmp4373 = *(&llvm_cbe_called_5[((unsigned int )1)]);
  llvm_cbe_tmp4378 = *(&llvm_cbe_called_5[((unsigned int )2)]);
  if (((((((unsigned int )(unsigned char )llvm_cbe_tmp4373)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp4378))) == ((unsigned int )0))) {
    llvm_cbe_bcptr4386_6__PHI_TEMPORARY = (&llvm_cbe_bc);   /* for PHI node */
    goto llvm_cbe_bb4409;
  } else {
    llvm_cbe_called_6__PHI_TEMPORARY = llvm_cbe_called_5;   /* for PHI node */
    goto llvm_cbe_cond_next4435;
  }

  do {     /* Syntactic loop 'bb4409' to make GCC happy */
llvm_cbe_bb4409:
  llvm_cbe_bcptr4386_6 = llvm_cbe_bcptr4386_6__PHI_TEMPORARY;
  if ((llvm_cbe_bcptr4386_6 == ((struct l_struct_2E_branch_chain *)/*NULL*/0))) {
    goto llvm_cbe_cond_true4430;
  } else {
    goto llvm_cbe_cond_next4415;
  }

llvm_cbe_cond_next4405:
  llvm_cbe_tmp4408 = *(&llvm_cbe_bcptr4386_6->field0);
  llvm_cbe_bcptr4386_6__PHI_TEMPORARY = llvm_cbe_tmp4408;   /* for PHI node */
  goto llvm_cbe_bb4409;

llvm_cbe_bb4393:
  llvm_cbe_tmp4399 = could_be_empty_branch(llvm_cbe_tmp4418, llvm_cbe_code105_10);
  if ((llvm_cbe_tmp4399 == ((unsigned int )0))) {
    llvm_cbe_called_6__PHI_TEMPORARY = llvm_cbe_called_5;   /* for PHI node */
    goto llvm_cbe_cond_next4435;
  } else {
    goto llvm_cbe_cond_next4405;
  }

llvm_cbe_cond_next4415:
  llvm_cbe_tmp4418 = *(&llvm_cbe_bcptr4386_6->field1);
  if ((((unsigned char *)llvm_cbe_tmp4418) < ((unsigned char *)llvm_cbe_called_5))) {
    goto llvm_cbe_cond_true4430;
  } else {
    goto llvm_cbe_bb4393;
  }

  } while (1); /* end of syntactic loop 'bb4409' */
llvm_cbe_bb4584:
  *llvm_cbe_tmp4496 = llvm_cbe_tmp4555;
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp4537[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_tmp4555;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next4572:
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_bb4584;
  } else {
    goto llvm_cbe_cond_next4578;
  }

llvm_cbe_cond_true4561:
  llvm_cbe_tmp4564 = *llvm_cbe_tmp4563;
  if (((&llvm_cbe_tmp4564[((unsigned int )3)]) == llvm_cbe_code105_10)) {
    goto llvm_cbe_cond_next4572;
  } else {
    goto llvm_cbe_bb4589;
  }

llvm_cbe_bb4550:
  llvm_cbe_tmp4551 = *(&llvm_cbe_set);
  llvm_cbe_tmp4554 = *(&llvm_cbe_unset);
  llvm_cbe_tmp4555 = (llvm_cbe_tmp4551 | llvm_cbe_options104_39466_9) & (llvm_cbe_tmp4554 ^ ((unsigned int )-1));
  if ((llvm_cbe_tmp4538 == ((unsigned char )41))) {
    goto llvm_cbe_cond_true4561;
  } else {
    goto llvm_cbe_cond_next4625;
  }

  do {     /* Syntactic loop 'bb4536' to make GCC happy */
llvm_cbe_bb4536:
  llvm_cbe_optset_6 = llvm_cbe_optset_6__PHI_TEMPORARY;
  llvm_cbe_tmp4537 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4538 = *llvm_cbe_tmp4537;
  switch (llvm_cbe_tmp4538) {
  default:
    goto llvm_cbe_bb4484;
;
  case ((unsigned char )41):
    goto llvm_cbe_bb4550;
    break;
  case ((unsigned char )58):
    goto llvm_cbe_bb4550;
    break;
  }
llvm_cbe_bb4484:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp4537[((unsigned int )1)]);
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp4538))) {
  default:
    goto llvm_cbe_bb4531;
;
  case ((unsigned int )45):
    llvm_cbe_optset_6__PHI_TEMPORARY = (&llvm_cbe_unset);   /* for PHI node */
    goto llvm_cbe_bb4536;
  case ((unsigned int )74):
    goto llvm_cbe_bb4490;
    break;
  case ((unsigned int )85):
    goto llvm_cbe_bb4521;
  case ((unsigned int )88):
    goto llvm_cbe_bb4526;
  case ((unsigned int )105):
    goto llvm_cbe_bb4501;
  case ((unsigned int )109):
    goto llvm_cbe_bb4506;
  case ((unsigned int )115):
    goto llvm_cbe_bb4511;
  case ((unsigned int )120):
    goto llvm_cbe_bb4516;
  }
llvm_cbe_bb4490:
  llvm_cbe_tmp4492 = *llvm_cbe_optset_6;
  *llvm_cbe_optset_6 = (llvm_cbe_tmp4492 | ((unsigned int )524288));
  llvm_cbe_tmp4497 = *llvm_cbe_tmp4496;
  *llvm_cbe_tmp4496 = (llvm_cbe_tmp4497 | ((unsigned int )134217728));
  llvm_cbe_optset_6__PHI_TEMPORARY = llvm_cbe_optset_6;   /* for PHI node */
  goto llvm_cbe_bb4536;

llvm_cbe_bb4501:
  llvm_cbe_tmp4503 = *llvm_cbe_optset_6;
  *llvm_cbe_optset_6 = (llvm_cbe_tmp4503 | ((unsigned int )1));
  llvm_cbe_optset_6__PHI_TEMPORARY = llvm_cbe_optset_6;   /* for PHI node */
  goto llvm_cbe_bb4536;

llvm_cbe_bb4506:
  llvm_cbe_tmp4508 = *llvm_cbe_optset_6;
  *llvm_cbe_optset_6 = (llvm_cbe_tmp4508 | ((unsigned int )2));
  llvm_cbe_optset_6__PHI_TEMPORARY = llvm_cbe_optset_6;   /* for PHI node */
  goto llvm_cbe_bb4536;

llvm_cbe_bb4511:
  llvm_cbe_tmp4513 = *llvm_cbe_optset_6;
  *llvm_cbe_optset_6 = (llvm_cbe_tmp4513 | ((unsigned int )4));
  llvm_cbe_optset_6__PHI_TEMPORARY = llvm_cbe_optset_6;   /* for PHI node */
  goto llvm_cbe_bb4536;

llvm_cbe_bb4516:
  llvm_cbe_tmp4518 = *llvm_cbe_optset_6;
  *llvm_cbe_optset_6 = (llvm_cbe_tmp4518 | ((unsigned int )8));
  llvm_cbe_optset_6__PHI_TEMPORARY = llvm_cbe_optset_6;   /* for PHI node */
  goto llvm_cbe_bb4536;

llvm_cbe_bb4521:
  llvm_cbe_tmp4523 = *llvm_cbe_optset_6;
  *llvm_cbe_optset_6 = (llvm_cbe_tmp4523 | ((unsigned int )512));
  llvm_cbe_optset_6__PHI_TEMPORARY = llvm_cbe_optset_6;   /* for PHI node */
  goto llvm_cbe_bb4536;

llvm_cbe_bb4526:
  llvm_cbe_tmp4528 = *llvm_cbe_optset_6;
  *llvm_cbe_optset_6 = (llvm_cbe_tmp4528 | ((unsigned int )64));
  llvm_cbe_optset_6__PHI_TEMPORARY = llvm_cbe_optset_6;   /* for PHI node */
  goto llvm_cbe_bb4536;

  } while (1); /* end of syntactic loop 'bb4536' */
llvm_cbe_OTHER_CHAR_AFTER_QUERY:
  *(&llvm_cbe_unset) = ((unsigned int )0);
  *(&llvm_cbe_set) = ((unsigned int )0);
  llvm_cbe_optset_6__PHI_TEMPORARY = (&llvm_cbe_set);   /* for PHI node */
  goto llvm_cbe_bb4536;

llvm_cbe_cond_next4578:
  llvm_cbe_tmp4580 = *llvm_cbe_iftmp_509_0;
  if ((llvm_cbe_tmp4580 == ((unsigned int )6))) {
    goto llvm_cbe_bb4584;
  } else {
    goto llvm_cbe_bb4589;
  }

llvm_cbe_cond_false4621:
  llvm_cbe_tmp519613486 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613486[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_tmp4555;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_24;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_tmp4555;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_tmp4615;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_tmp4613;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next4607:
  llvm_cbe_code105_24 = llvm_cbe_code105_24__PHI_TEMPORARY;
  llvm_cbe_tmp4613 = (((unsigned int )(((unsigned int )llvm_cbe_tmp4555) >> ((unsigned int )((unsigned int )9))))) & ((unsigned int )1);
  llvm_cbe_tmp4615 = llvm_cbe_tmp4613 ^ ((unsigned int )1);
  if (((llvm_cbe_tmp4555 & ((unsigned int )1)) == ((unsigned int )0))) {
    goto llvm_cbe_cond_false4621;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_tmp4555;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_24;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_tmp4555;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = ((unsigned int )256);   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_tmp4615;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_tmp4613;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_bb4589:
  if ((((llvm_cbe_tmp4555 ^ llvm_cbe_options104_39466_9) & ((unsigned int )7)) == ((unsigned int )0))) {
    llvm_cbe_code105_24__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    goto llvm_cbe_cond_next4607;
  } else {
    goto llvm_cbe_cond_true4597;
  }

llvm_cbe_cond_true4597:
  *llvm_cbe_code105_10 = ((unsigned char )24);
  *(&llvm_cbe_code105_10[((unsigned int )1)]) = (((unsigned char )((((unsigned char )llvm_cbe_tmp4555)) & ((unsigned char )7))));
  llvm_cbe_tmp4606 = &llvm_cbe_code105_10[((unsigned int )2)];
  llvm_cbe_code105_24__PHI_TEMPORARY = llvm_cbe_tmp4606;   /* for PHI node */
  goto llvm_cbe_cond_next4607;

llvm_cbe_cond_true4852:
  llvm_cbe_tmp519613492 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613492[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_iftmp_467_0;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_4;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_26;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_3;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_tmp4848;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_11;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next4847:
  llvm_cbe_groupsetfirstbyte_4 = llvm_cbe_groupsetfirstbyte_4__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_3 = llvm_cbe_zerofirstbyte_3__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_11 = llvm_cbe_firstbyte102_11__PHI_TEMPORARY;
  llvm_cbe_tmp4848 = *(&llvm_cbe_subreqbyte);
  if ((((signed int )llvm_cbe_tmp4848) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true4852;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_iftmp_467_0;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_4;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_26;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_3;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_11;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_cond_true4822:
  if ((((signed int )llvm_cbe_tmp4823) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true4827;
  } else {
    llvm_cbe_groupsetfirstbyte_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_zerofirstbyte_3__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_firstbyte102_11__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_cond_next4847;
  }

llvm_cbe_cond_true4817:
  llvm_cbe_tmp4823 = *(&llvm_cbe_subfirstbyte);
  if ((llvm_cbe_firstbyte102_39829_9 == ((unsigned int )-2))) {
    goto llvm_cbe_cond_true4822;
  } else {
    goto llvm_cbe_cond_false4831;
  }

llvm_cbe_cond_next4810:
  if ((((signed int )llvm_cbe_bravalue_9) > ((signed int )((unsigned int )91)))) {
    goto llvm_cbe_cond_true4817;
  } else {
    goto llvm_cbe_cond_false4855;
  }

llvm_cbe_cond_next4804:
  llvm_cbe_code105_26 = llvm_cbe_code105_26__PHI_TEMPORARY;
  if ((llvm_cbe_bravalue_9 == ((unsigned int )101))) {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_iftmp_467_0;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_26;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  } else {
    goto llvm_cbe_cond_next4810;
  }

llvm_cbe_cond_true4778:
  llvm_cbe_tmp4780 = *llvm_cbe_iftmp_509_0;
  llvm_cbe_tmp4781 = *(&llvm_cbe_length_prevgroup);
  *llvm_cbe_iftmp_509_0 = ((llvm_cbe_tmp4780 + ((unsigned int )-6)) + llvm_cbe_tmp4781);
  *(&llvm_cbe_code105_10[((unsigned int )1)]) = ((unsigned char )0);
  *(&llvm_cbe_code105_10[((unsigned int )2)]) = ((unsigned char )3);
  *(&llvm_cbe_code105_10[((unsigned int )3)]) = ((unsigned char )84);
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = ((unsigned char )0);
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = ((unsigned char )3);
  llvm_cbe_tmp4801 = &llvm_cbe_code105_10[((unsigned int )6)];
  llvm_cbe_code105_26__PHI_TEMPORARY = llvm_cbe_tmp4801;   /* for PHI node */
  goto llvm_cbe_cond_next4804;

llvm_cbe_cond_next4773:
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_false4802;
  } else {
    goto llvm_cbe_cond_true4778;
  }

llvm_cbe_cond_next4765:
  llvm_cbe_bravalue_9 = llvm_cbe_bravalue_9__PHI_TEMPORARY;
  llvm_cbe_tmp4766 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4767 = *llvm_cbe_tmp4766;
  if ((llvm_cbe_tmp4767 == ((unsigned char )41))) {
    goto llvm_cbe_cond_next4773;
  } else {
    goto llvm_cbe_cond_true4771;
  }

llvm_cbe_cond_next4700:
  if (((llvm_cbe_bravalue_8 == ((unsigned int )95)) & (llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0)))) {
    llvm_cbe_condcount_6__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_tc_6_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb4714;
  } else {
    llvm_cbe_bravalue_9__PHI_TEMPORARY = llvm_cbe_bravalue_8;   /* for PHI node */
    goto llvm_cbe_cond_next4765;
  }

llvm_cbe_cond_next4658:
  llvm_cbe_slot_913437_0_us61_3 = llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_3 = llvm_cbe_slot_913437_059_3__PHI_TEMPORARY;
  llvm_cbe_skipbytes133_8 = llvm_cbe_skipbytes133_8__PHI_TEMPORARY;
  llvm_cbe_newoptions_8 = llvm_cbe_newoptions_8__PHI_TEMPORARY;
  llvm_cbe_reset_bracount132_9 = llvm_cbe_reset_bracount132_9__PHI_TEMPORARY;
  llvm_cbe_bravalue_8 = llvm_cbe_bravalue_8__PHI_TEMPORARY;
  llvm_cbe_iftmp_467_0 = (((((signed int )llvm_cbe_bravalue_8) > ((signed int )((unsigned int )91)))) ? (llvm_cbe_code105_10) : (((unsigned char *)/*NULL*/0)));
  *llvm_cbe_code105_10 = (((unsigned char )llvm_cbe_bravalue_8));
  *(&llvm_cbe_tempcode) = llvm_cbe_code105_10;
  llvm_cbe_tmp4674 = *llvm_cbe_tmp1993;
  *(&llvm_cbe_length_prevgroup) = ((unsigned int )0);
  llvm_cbe_tmp4695 = compile_regex(llvm_cbe_newoptions_8, (llvm_cbe_options104_39466_9 & ((unsigned int )7)), (&llvm_cbe_tempcode), (&llvm_cbe_ptr106), llvm_cbe_errorcodeptr, (((unsigned int )(bool )(((unsigned int )(llvm_cbe_bravalue_8 + ((unsigned int )-89))) < ((unsigned int )((unsigned int )2))))), llvm_cbe_reset_bracount132_9, llvm_cbe_skipbytes133_8, (&llvm_cbe_subfirstbyte), (&llvm_cbe_subreqbyte), (&llvm_cbe_bc), llvm_cbe_cd, llvm_cbe_iftmp_468_0);
  if ((llvm_cbe_tmp4695 == ((unsigned int )0))) {
    llvm_cbe_slot_913437_0_us61_12__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
    llvm_cbe_slot_913437_059_12__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
    goto llvm_cbe_FAILED;
  } else {
    goto llvm_cbe_cond_next4700;
  }

llvm_cbe_bb3221:
  llvm_cbe_reset_bracount132_8 = llvm_cbe_reset_bracount132_8__PHI_TEMPORARY;
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3177[((unsigned int )3)]);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = llvm_cbe_reset_bracount132_8;   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )93);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_bb3220:
  llvm_cbe_reset_bracount132_8__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  goto llvm_cbe_bb3221;

llvm_cbe_cond_true3231:
  llvm_cbe_tmp3234 = *(&llvm_cbe_tmp3177[((unsigned int )4)]);
  switch (llvm_cbe_tmp3234) {
  default:
    goto llvm_cbe_cond_next3256;
;
  case ((unsigned char )33):
    llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
    goto llvm_cbe_cond_next4658;
  case ((unsigned char )61):
    llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
    goto llvm_cbe_cond_next4658;
  case ((unsigned char )60):
    llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
    goto llvm_cbe_cond_next4658;
  }
llvm_cbe_bb3224:
  llvm_cbe_tmp3227 = *(&llvm_cbe_tmp3177[((unsigned int )3)]);
  if ((llvm_cbe_tmp3227 == ((unsigned char )63))) {
    goto llvm_cbe_cond_true3231;
  } else {
    goto llvm_cbe_cond_next3256;
  }

llvm_cbe_bb3422:
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_next3428;
  } else {
    llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )3);   /* for PHI node */
    llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
    goto llvm_cbe_cond_next4658;
  }

llvm_cbe_bb3411:
  llvm_cbe_tmp3412 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3413 = *llvm_cbe_tmp3412;
  llvm_cbe_tmp3416 = &llvm_cbe_tmp3412[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3416;
  if ((llvm_cbe_tmp3413 == ((unsigned char )41))) {
    goto llvm_cbe_bb3422;
  } else {
    goto llvm_cbe_bb3418;
  }

llvm_cbe_bb3391:
  llvm_cbe_recno_7_lcssa = llvm_cbe_recno_7_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp3392 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3396 = (((unsigned int )(unsigned long)llvm_cbe_tmp3392)) - (((unsigned int )(unsigned long)llvm_cbe_tmp3328));
  if ((((signed int )llvm_cbe_terminator_0) < ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_bb3411;
  } else {
    goto llvm_cbe_cond_next3402;
  }

llvm_cbe_bb3377_preheader:
  llvm_cbe_tmp338226532 = *llvm_cbe_tmp3328;
  llvm_cbe_tmp338526535 = *(&llvm_cbe_tmp3326[(((unsigned int )(unsigned char )llvm_cbe_tmp338226532))]);
  if (((((unsigned char )(llvm_cbe_tmp338526535 & ((unsigned char )16)))) == ((unsigned char )0))) {
    llvm_cbe_recno_7_lcssa__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3391;
  } else {
    llvm_cbe_recno_726527__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3347;
  }

llvm_cbe_bb3323:
  llvm_cbe_terminator_0 = llvm_cbe_terminator_0__PHI_TEMPORARY;
  llvm_cbe_refsign_6 = llvm_cbe_refsign_6__PHI_TEMPORARY;
  llvm_cbe_tmp3326 = *llvm_cbe_tmp435;
  llvm_cbe_tmp3327 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3328 = &llvm_cbe_tmp3327[((unsigned int )1)];
  llvm_cbe_tmp3329 = *llvm_cbe_tmp3328;
  llvm_cbe_tmp3332 = *(&llvm_cbe_tmp3326[(((unsigned int )(unsigned char )llvm_cbe_tmp3329))]);
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3328;
  if (((((unsigned char )(llvm_cbe_tmp3332 & ((unsigned char )16)))) == ((unsigned char )0))) {
    goto llvm_cbe_cond_true3339;
  } else {
    goto llvm_cbe_bb3377_preheader;
  }

llvm_cbe_cond_next3256:
  llvm_cbe_tmp3258 = &llvm_cbe_code105_10[((unsigned int )3)];
  *llvm_cbe_tmp3258 = ((unsigned char )99);
  llvm_cbe_tmp3259 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3260 = &llvm_cbe_tmp3259[((unsigned int )1)];
  llvm_cbe_tmp3261 = *llvm_cbe_tmp3260;
  switch (llvm_cbe_tmp3261) {
  default:
    llvm_cbe_terminator_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_refsign_6__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_bb3323;
;
  case ((unsigned char )82):
    goto llvm_cbe_cond_next3266;
    break;
  case ((unsigned char )60):
    goto llvm_cbe_cond_true3286;
  case ((unsigned char )39):
    goto llvm_cbe_cond_true3296;
  case ((unsigned char )43):
    goto llvm_cbe_cond_true3314;
  case ((unsigned char )45):
    goto llvm_cbe_cond_true3314;
  }
llvm_cbe_cond_next3274:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3268;
  *llvm_cbe_tmp3258 = ((unsigned char )100);
  llvm_cbe_terminator_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  llvm_cbe_refsign_6__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  goto llvm_cbe_bb3323;

llvm_cbe_cond_next3266:
  llvm_cbe_tmp3268 = &llvm_cbe_tmp3259[((unsigned int )2)];
  llvm_cbe_tmp3269 = *llvm_cbe_tmp3268;
  if ((llvm_cbe_tmp3269 == ((unsigned char )38))) {
    goto llvm_cbe_cond_next3274;
  } else {
    goto llvm_cbe_bb3279;
  }

llvm_cbe_bb3279:
  switch (llvm_cbe_tmp3261) {
  default:
    llvm_cbe_terminator_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_refsign_6__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_bb3323;
;
  case ((unsigned char )60):
    goto llvm_cbe_cond_true3286;
    break;
  case ((unsigned char )39):
    goto llvm_cbe_cond_true3296;
  case ((unsigned char )43):
    goto llvm_cbe_cond_true3314;
  case ((unsigned char )45):
    goto llvm_cbe_cond_true3314;
  }
llvm_cbe_cond_true3286:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3260;
  llvm_cbe_terminator_0__PHI_TEMPORARY = ((unsigned int )62);   /* for PHI node */
  llvm_cbe_refsign_6__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  goto llvm_cbe_bb3323;

llvm_cbe_cond_true3296:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3260;
  llvm_cbe_terminator_0__PHI_TEMPORARY = ((unsigned int )39);   /* for PHI node */
  llvm_cbe_refsign_6__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  goto llvm_cbe_bb3323;

llvm_cbe_cond_true3314:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3260;
  llvm_cbe_tmp3318 = *llvm_cbe_tmp3260;
  llvm_cbe_tmp33183319 = ((unsigned int )(unsigned char )llvm_cbe_tmp3318);
  llvm_cbe_terminator_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_refsign_6__PHI_TEMPORARY = llvm_cbe_tmp33183319;   /* for PHI node */
  goto llvm_cbe_bb3323;

  do {     /* Syntactic loop 'bb3347' to make GCC happy */
llvm_cbe_bb3347:
  llvm_cbe_recno_726527 = llvm_cbe_recno_726527__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_recno_726527) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true3352;
  } else {
    llvm_cbe_recno_6__PHI_TEMPORARY = llvm_cbe_recno_726527;   /* for PHI node */
    goto llvm_cbe_cond_next3374;
  }

llvm_cbe_cond_next3374:
  llvm_cbe_recno_6 = llvm_cbe_recno_6__PHI_TEMPORARY;
  llvm_cbe_tmp3375 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3376 = &llvm_cbe_tmp3375[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3376;
  llvm_cbe_tmp3382 = *llvm_cbe_tmp3376;
  llvm_cbe_tmp3385 = *(&llvm_cbe_tmp3326[(((unsigned int )(unsigned char )llvm_cbe_tmp3382))]);
  if (((((unsigned char )(llvm_cbe_tmp3385 & ((unsigned char )16)))) == ((unsigned char )0))) {
    llvm_cbe_recno_7_lcssa__PHI_TEMPORARY = llvm_cbe_recno_6;   /* for PHI node */
    goto llvm_cbe_bb3391;
  } else {
    llvm_cbe_recno_726527__PHI_TEMPORARY = llvm_cbe_recno_6;   /* for PHI node */
    goto llvm_cbe_bb3347;
  }

llvm_cbe_cond_true3352:
  llvm_cbe_tmp3353 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3354 = *llvm_cbe_tmp3353;
  llvm_cbe_tmp33543355 = ((unsigned int )(unsigned char )llvm_cbe_tmp3354);
  llvm_cbe_tmp3357 = *(&digitab[llvm_cbe_tmp33543355]);
  if (((((unsigned char )(llvm_cbe_tmp3357 & ((unsigned char )4)))) == ((unsigned char )0))) {
    llvm_cbe_recno_6__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_cond_next3374;
  } else {
    goto llvm_cbe_cond_true3363;
  }

llvm_cbe_cond_true3363:
  llvm_cbe_tmp3370 = ((llvm_cbe_recno_726527 * ((unsigned int )10)) + ((unsigned int )-48)) + llvm_cbe_tmp33543355;
  llvm_cbe_recno_6__PHI_TEMPORARY = llvm_cbe_tmp3370;   /* for PHI node */
  goto llvm_cbe_cond_next3374;

  } while (1); /* end of syntactic loop 'bb3347' */
llvm_cbe_cond_next3402:
  llvm_cbe_tmp3404 = *llvm_cbe_tmp3392;
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3392[((unsigned int )1)]);
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp3404)) == llvm_cbe_terminator_0)) {
    goto llvm_cbe_bb3411;
  } else {
    goto llvm_cbe_bb3418;
  }

llvm_cbe_cond_next3465:
  llvm_cbe_recno_8 = llvm_cbe_recno_8__PHI_TEMPORARY;
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_recno_8) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = (((unsigned char )llvm_cbe_recno_8));
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )3);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_cond_true3445:
  llvm_cbe_tmp3451 = (llvm_cbe_tmp3448 - llvm_cbe_recno_7_lcssa) + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp3451) < ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true3456;
  } else {
    llvm_cbe_recno_8__PHI_TEMPORARY = llvm_cbe_tmp3451;   /* for PHI node */
    goto llvm_cbe_cond_next3465;
  }

llvm_cbe_cond_next3440:
  llvm_cbe_tmp3448 = *llvm_cbe_tmp24;
  if ((llvm_cbe_refsign_6 == ((unsigned int )45))) {
    goto llvm_cbe_cond_true3445;
  } else {
    goto llvm_cbe_cond_false3459;
  }

llvm_cbe_cond_true3433:
  if ((((signed int )llvm_cbe_recno_7_lcssa) < ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true3438;
  } else {
    goto llvm_cbe_cond_next3440;
  }

llvm_cbe_cond_next3428:
  if ((((signed int )llvm_cbe_refsign_6) > ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true3433;
  } else {
    goto llvm_cbe_cond_next3476;
  }

llvm_cbe_cond_false3459:
  llvm_cbe_tmp3464 = llvm_cbe_tmp3448 + llvm_cbe_recno_7_lcssa;
  llvm_cbe_recno_8__PHI_TEMPORARY = llvm_cbe_tmp3464;   /* for PHI node */
  goto llvm_cbe_cond_next3465;

llvm_cbe_cond_true3528:
  llvm_cbe_tmp3531 = *llvm_cbe_slot_613344_1;
  llvm_cbe_tmp3536 = *(&llvm_cbe_slot_613344_1[((unsigned int )1)]);
  llvm_cbe_tmp3538 = ((((unsigned int )(unsigned char )llvm_cbe_tmp3531)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp3536));
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp3538) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = (((unsigned char )llvm_cbe_tmp3538));
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )3);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_bb3520:
  llvm_cbe_i3185_613340_1 = llvm_cbe_i3185_613340_1__PHI_TEMPORARY;
  llvm_cbe_slot_613344_1 = llvm_cbe_slot_613344_1__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_tmp351513357) > ((signed int )llvm_cbe_i3185_613340_1))) {
    goto llvm_cbe_cond_true3528;
  } else {
    goto llvm_cbe_cond_false3549;
  }

llvm_cbe_cond_next3476:
  llvm_cbe_tmp3479 = *llvm_cbe_tmp3478;
  llvm_cbe_tmp351513357 = *llvm_cbe_tmp351413356;
  if ((((signed int )llvm_cbe_tmp351513357) > ((signed int )((unsigned int )0)))) {
    llvm_cbe_i3185_613340_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_slot_613344_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_cond_false3492;
  } else {
    llvm_cbe_i3185_613340_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_slot_613344_1__PHI_TEMPORARY = llvm_cbe_tmp3479;   /* for PHI node */
    goto llvm_cbe_bb3520;
  }

  do {     /* Syntactic loop 'cond_false3492' to make GCC happy */
llvm_cbe_cond_false3492:
  llvm_cbe_i3185_613340_0 = llvm_cbe_i3185_613340_0__PHI_TEMPORARY;
  llvm_cbe_slot_613344_0_rec = llvm_cbe_slot_613344_0_rec__PHI_TEMPORARY;
  llvm_cbe_slot_613344_0 = &llvm_cbe_tmp3479[llvm_cbe_slot_613344_0_rec];
  llvm_cbe_tmp3497 = strncmp(llvm_cbe_tmp3328, (&llvm_cbe_tmp3479[(llvm_cbe_slot_613344_0_rec + ((unsigned int )2))]), llvm_cbe_tmp3396);
  if ((llvm_cbe_tmp3497 == ((unsigned int )0))) {
    llvm_cbe_i3185_613340_1__PHI_TEMPORARY = llvm_cbe_i3185_613340_0;   /* for PHI node */
    llvm_cbe_slot_613344_1__PHI_TEMPORARY = llvm_cbe_slot_613344_0;   /* for PHI node */
    goto llvm_cbe_bb3520;
  } else {
    goto llvm_cbe_bb3512;
  }

llvm_cbe_bb3512:
  llvm_cbe_tmp3507 = *llvm_cbe_tmp3506;
  llvm_cbe_tmp3509_rec = llvm_cbe_slot_613344_0_rec + llvm_cbe_tmp3507;
  llvm_cbe_tmp3509 = &llvm_cbe_tmp3479[llvm_cbe_tmp3509_rec];
  llvm_cbe_tmp3511 = llvm_cbe_i3185_613340_0 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp351513357) > ((signed int )llvm_cbe_tmp3511))) {
    llvm_cbe_i3185_613340_0__PHI_TEMPORARY = llvm_cbe_tmp3511;   /* for PHI node */
    llvm_cbe_slot_613344_0_rec__PHI_TEMPORARY = llvm_cbe_tmp3509_rec;   /* for PHI node */
    goto llvm_cbe_cond_false3492;
  } else {
    llvm_cbe_i3185_613340_1__PHI_TEMPORARY = llvm_cbe_tmp3511;   /* for PHI node */
    llvm_cbe_slot_613344_1__PHI_TEMPORARY = llvm_cbe_tmp3509;   /* for PHI node */
    goto llvm_cbe_bb3520;
  }

  } while (1); /* end of syntactic loop 'cond_false3492' */
llvm_cbe_cond_true3564:
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp3559) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = (((unsigned char )llvm_cbe_tmp3559));
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )3);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_cond_false3549:
  llvm_cbe_tmp3555 = *llvm_cbe_tmp24;
  llvm_cbe_tmp3559 = find_parens(llvm_cbe_tmp3416, llvm_cbe_tmp3555, llvm_cbe_tmp3328, llvm_cbe_tmp3396, ((((unsigned int )(((unsigned int )llvm_cbe_options104_39466_9) >> ((unsigned int )((unsigned int )3))))) & ((unsigned int )1)));
  if ((((signed int )llvm_cbe_tmp3559) > ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true3564;
  } else {
    goto llvm_cbe_cond_false3575;
  }

llvm_cbe_bb3623:
  llvm_cbe_recno_10 = (((llvm_cbe_recno_9 == ((unsigned int )0))) ? (((unsigned int )65535)) : (llvm_cbe_recno_9));
  *llvm_cbe_tmp3258 = ((unsigned char )100);
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_recno_10) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = (((unsigned char )llvm_cbe_recno_10));
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )3);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

  do {     /* Syntactic loop 'bb3617' to make GCC happy */
llvm_cbe_bb3617:
  llvm_cbe_indvar26632 = llvm_cbe_indvar26632__PHI_TEMPORARY;
  llvm_cbe_recno_9 = llvm_cbe_recno_9__PHI_TEMPORARY;
  llvm_cbe_i3185_7 = llvm_cbe_indvar26632 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_i3185_7) < ((signed int )llvm_cbe_tmp3396))) {
    goto llvm_cbe_bb3589;
  } else {
    goto llvm_cbe_bb3623;
  }

llvm_cbe_cond_next3605:
  llvm_cbe_tmp3614 = ((llvm_cbe_recno_9 * ((unsigned int )10)) + ((unsigned int )-48)) + llvm_cbe_tmp35933594;
  llvm_cbe_indvar26632__PHI_TEMPORARY = llvm_cbe_i3185_7;   /* for PHI node */
  llvm_cbe_recno_9__PHI_TEMPORARY = llvm_cbe_tmp3614;   /* for PHI node */
  goto llvm_cbe_bb3617;

llvm_cbe_bb3589:
  llvm_cbe_tmp3593 = *(&llvm_cbe_tmp3327[(llvm_cbe_indvar26632 + ((unsigned int )2))]);
  llvm_cbe_tmp35933594 = ((unsigned int )(unsigned char )llvm_cbe_tmp3593);
  llvm_cbe_tmp3596 = *(&digitab[llvm_cbe_tmp35933594]);
  if (((((unsigned char )(llvm_cbe_tmp3596 & ((unsigned char )4)))) == ((unsigned char )0))) {
    goto llvm_cbe_cond_true3603;
  } else {
    goto llvm_cbe_cond_next3605;
  }

  } while (1); /* end of syntactic loop 'bb3617' */
llvm_cbe_cond_false3582:
  llvm_cbe_tmp3584 = *llvm_cbe_tmp3328;
  if ((llvm_cbe_tmp3584 == ((unsigned char )82))) {
    llvm_cbe_indvar26632__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_recno_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3617;
  } else {
    goto llvm_cbe_cond_false3643;
  }

llvm_cbe_cond_false3575:
  if ((llvm_cbe_terminator_0 == ((unsigned int )0))) {
    goto llvm_cbe_cond_false3582;
  } else {
    goto llvm_cbe_cond_true3580;
  }

llvm_cbe_cond_next3657:
  *llvm_cbe_tmp3258 = ((unsigned char )101);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_cond_next3649:
  llvm_cbe_tmp3652 = strncmp(llvm_cbe_tmp3328, (&(_2E_str74[((unsigned int )0)])), ((unsigned int )6));
  if ((llvm_cbe_tmp3652 == ((unsigned int )0))) {
    goto llvm_cbe_cond_next3657;
  } else {
    goto llvm_cbe_bb3660;
  }

llvm_cbe_cond_false3643:
  if ((llvm_cbe_tmp3396 == ((unsigned int )6))) {
    goto llvm_cbe_cond_next3649;
  } else {
    goto llvm_cbe_bb3660;
  }

llvm_cbe_cond_true3665:
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_recno_7_lcssa) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )5)]) = (((unsigned char )llvm_cbe_recno_7_lcssa));
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )3);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )95);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_bb3660:
  if ((((signed int )llvm_cbe_recno_7_lcssa) > ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true3665;
  } else {
    goto llvm_cbe_cond_false3676;
  }

llvm_cbe_bb3692:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3177[((unsigned int )3)]);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )87);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_bb3695:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3177[((unsigned int )3)]);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )88);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_bb3703:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3177[((unsigned int )4)]);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )89);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_bb3698:
  llvm_cbe_tmp3700 = &llvm_cbe_tmp3177[((unsigned int )3)];
  llvm_cbe_tmp3701 = *llvm_cbe_tmp3700;
  llvm_cbe_tmp37013702 = ((unsigned int )(unsigned char )llvm_cbe_tmp3701);
  switch (llvm_cbe_tmp37013702) {
  default:
    goto llvm_cbe_bb3709;
;
  case ((unsigned int )33):
    goto llvm_cbe_bb3706;
  case ((unsigned int )61):
    goto llvm_cbe_bb3703;
    break;
  }
llvm_cbe_bb3706:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3177[((unsigned int )4)]);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )90);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_bb3730:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp3177[((unsigned int )3)]);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )92);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_cond_next4625:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp4537[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_tmp4555;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )93);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_cond_false4629:
  if (((llvm_cbe_options104_39466_9 & ((unsigned int )4096)) == ((unsigned int )0))) {
    llvm_cbe_slot_913437_0_us61_2__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_2__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    goto llvm_cbe_NUMBERED_GROUP;
  } else {
    llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )93);   /* for PHI node */
    goto llvm_cbe_cond_next4658;
  }

llvm_cbe_NUMBERED_GROUP:
  llvm_cbe_slot_913437_0_us61_2 = llvm_cbe_slot_913437_0_us61_2__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_2 = llvm_cbe_slot_913437_059_2__PHI_TEMPORARY;
  llvm_cbe_tmp4639 = *llvm_cbe_tmp24;
  llvm_cbe_tmp4640 = llvm_cbe_tmp4639 + ((unsigned int )1);
  *llvm_cbe_tmp24 = llvm_cbe_tmp4640;
  *(&llvm_cbe_code105_10[((unsigned int )3)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp4640) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp4652 = *llvm_cbe_tmp24;
  *(&llvm_cbe_code105_10[((unsigned int )4)]) = (((unsigned char )llvm_cbe_tmp4652));
  llvm_cbe_slot_913437_0_us61_3__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_2;   /* for PHI node */
  llvm_cbe_slot_913437_059_3__PHI_TEMPORARY = llvm_cbe_slot_913437_059_2;   /* for PHI node */
  llvm_cbe_skipbytes133_8__PHI_TEMPORARY = ((unsigned int )2);   /* for PHI node */
  llvm_cbe_newoptions_8__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_reset_bracount132_9__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_bravalue_8__PHI_TEMPORARY = ((unsigned int )94);   /* for PHI node */
  goto llvm_cbe_cond_next4658;

llvm_cbe_cond_next4018:
  llvm_cbe_slot_913437_0_us61_1 = llvm_cbe_slot_913437_0_us61_1__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_1 = llvm_cbe_slot_913437_059_1__PHI_TEMPORARY;
  llvm_cbe_tmp4019 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp4019[((unsigned int )1)]);
  llvm_cbe_tmp4023 = *llvm_cbe_tmp351413356;
  *llvm_cbe_tmp351413356 = (llvm_cbe_tmp4023 + ((unsigned int )1));
  llvm_cbe_slot_913437_0_us61_2__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_1;   /* for PHI node */
  llvm_cbe_slot_913437_059_2__PHI_TEMPORARY = llvm_cbe_slot_913437_059_1;   /* for PHI node */
  goto llvm_cbe_NUMBERED_GROUP;

llvm_cbe_cond_next3899:
  llvm_cbe_tmp3901 = llvm_cbe_tmp3875 + ((unsigned int )3);
  llvm_cbe_tmp3904 = *llvm_cbe_tmp3506;
  if ((((signed int )llvm_cbe_tmp3901) > ((signed int )llvm_cbe_tmp3904))) {
    goto llvm_cbe_cond_true3908;
  } else {
    llvm_cbe_slot_913437_0_us61_1__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_1__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    goto llvm_cbe_cond_next4018;
  }

llvm_cbe_cond_next3890:
  llvm_cbe_tmp3893 = *llvm_cbe_tmp351413356;
  if ((((signed int )llvm_cbe_tmp3893) > ((signed int )((unsigned int )9999)))) {
    goto llvm_cbe_cond_true3897;
  } else {
    goto llvm_cbe_cond_next3899;
  }

llvm_cbe_cond_true3880:
  llvm_cbe_tmp3881 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3882 = *llvm_cbe_tmp3881;
  if (((((unsigned int )(unsigned char )llvm_cbe_tmp3882)) == llvm_cbe_iftmp_415_0)) {
    goto llvm_cbe_cond_next3890;
  } else {
    goto llvm_cbe_cond_true3888;
  }

llvm_cbe_bb3870:
  llvm_cbe_tmp3854_lcssa = llvm_cbe_tmp3854_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp3875 = (((unsigned int )(unsigned long)llvm_cbe_tmp3854_lcssa)) - (((unsigned int )(unsigned long)llvm_cbe_tmp3851));
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_false3921;
  } else {
    goto llvm_cbe_cond_true3880;
  }

llvm_cbe_bb3840:
  llvm_cbe_tmp3841 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3842 = *llvm_cbe_tmp3841;
  llvm_cbe_iftmp_415_0 = (((llvm_cbe_tmp3842 == ((unsigned char )60))) ? (((unsigned int )62)) : (((unsigned int )39)));
  llvm_cbe_tmp3851 = &llvm_cbe_tmp3841[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3851;
  llvm_cbe_tmp385926515 = *llvm_cbe_tmp435;
  llvm_cbe_tmp386126517 = *llvm_cbe_tmp3851;
  llvm_cbe_tmp386426520 = *(&llvm_cbe_tmp385926515[(((unsigned int )(unsigned char )llvm_cbe_tmp386126517))]);
  if (((((unsigned char )(llvm_cbe_tmp386426520 & ((unsigned char )16)))) == ((unsigned char )0))) {
    llvm_cbe_tmp3854_lcssa__PHI_TEMPORARY = llvm_cbe_tmp3851;   /* for PHI node */
    goto llvm_cbe_bb3870;
  } else {
    llvm_cbe_tmp385426524__PHI_TEMPORARY = llvm_cbe_tmp3851;   /* for PHI node */
    goto llvm_cbe_bb3853;
  }

llvm_cbe_bb3709:
  llvm_cbe_tmp3712 = *llvm_cbe_tmp435;
  llvm_cbe_tmp3718 = *(&llvm_cbe_tmp3712[llvm_cbe_tmp37013702]);
  if (((((unsigned char )(llvm_cbe_tmp3718 & ((unsigned char )16)))) == ((unsigned char )0))) {
    goto llvm_cbe_cond_next3725;
  } else {
    goto llvm_cbe_bb3840;
  }

  do {     /* Syntactic loop 'bb3853' to make GCC happy */
llvm_cbe_bb3853:
  llvm_cbe_tmp385426524 = llvm_cbe_tmp385426524__PHI_TEMPORARY;
  llvm_cbe_tmp3855 = &llvm_cbe_tmp385426524[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3855;
  llvm_cbe_tmp3861 = *llvm_cbe_tmp3855;
  llvm_cbe_tmp3864 = *(&llvm_cbe_tmp385926515[(((unsigned int )(unsigned char )llvm_cbe_tmp3861))]);
  if (((((unsigned char )(llvm_cbe_tmp3864 & ((unsigned char )16)))) == ((unsigned char )0))) {
    llvm_cbe_tmp3854_lcssa__PHI_TEMPORARY = llvm_cbe_tmp3855;   /* for PHI node */
    goto llvm_cbe_bb3870;
  } else {
    llvm_cbe_tmp385426524__PHI_TEMPORARY = llvm_cbe_tmp3855;   /* for PHI node */
    goto llvm_cbe_bb3853;
  }

  } while (1); /* end of syntactic loop 'bb3853' */
llvm_cbe_cond_true3908:
  *llvm_cbe_tmp3506 = llvm_cbe_tmp3901;
  if ((((signed int )llvm_cbe_tmp3875) > ((signed int )((unsigned int )32)))) {
    goto llvm_cbe_cond_true3917;
  } else {
    llvm_cbe_slot_913437_0_us61_1__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_1__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    goto llvm_cbe_cond_next4018;
  }

llvm_cbe_bb3993:
  llvm_cbe_slot_913437_0_us61_0 = llvm_cbe_slot_913437_0_us61_0__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_0 = llvm_cbe_slot_913437_059_0__PHI_TEMPORARY;
  llvm_cbe_slot_913437_1 = llvm_cbe_slot_913437_1__PHI_TEMPORARY;
  llvm_cbe_tmp3996 = *llvm_cbe_tmp24;
  *llvm_cbe_slot_913437_1 = (((unsigned char )(((unsigned int )(((unsigned int )(llvm_cbe_tmp3996 + ((unsigned int )1))) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp4004 = *llvm_cbe_tmp24;
  *(&llvm_cbe_slot_913437_1[((unsigned int )1)]) = (((unsigned char )((((unsigned char )llvm_cbe_tmp4004)) + ((unsigned char )1))));
  ltmp_15_1 = memcpy((&llvm_cbe_slot_913437_1[((unsigned int )2)]), llvm_cbe_tmp3851, llvm_cbe_tmp3875);
  *(&llvm_cbe_slot_913437_1[(llvm_cbe_tmp3875 + ((unsigned int )2))]) = ((unsigned char )0);
  llvm_cbe_slot_913437_0_us61_1__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_0;   /* for PHI node */
  llvm_cbe_slot_913437_059_1__PHI_TEMPORARY = llvm_cbe_slot_913437_059_0;   /* for PHI node */
  goto llvm_cbe_cond_next4018;

llvm_cbe_cond_false3921:
  llvm_cbe_tmp3924 = *llvm_cbe_tmp3478;
  llvm_cbe_tmp398813449 = *llvm_cbe_tmp351413356;
  if ((((signed int )llvm_cbe_tmp398813449) > ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_bb3925_preheader;
  } else {
    llvm_cbe_slot_913437_0_us61_0__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_0__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_slot_913437_1__PHI_TEMPORARY = llvm_cbe_tmp3924;   /* for PHI node */
    goto llvm_cbe_bb3993;
  }

llvm_cbe_cond_true3961_split:
  llvm_cbe_slot_913437_0_us61_14 = llvm_cbe_slot_913437_0_us61_14__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_14 = llvm_cbe_slot_913437_059_14__PHI_TEMPORARY;
  llvm_cbe_slot_913437_0_lcssa14144_us_lcssa = llvm_cbe_slot_913437_0_lcssa14144_us_lcssa__PHI_TEMPORARY;
  llvm_cbe_i3185_1013432_0_lcssa14142_us_lcssa = llvm_cbe_i3185_1013432_0_lcssa14142_us_lcssa__PHI_TEMPORARY;
  llvm_cbe_tmp3969 = *llvm_cbe_tmp3506;
  ltmp_14_1 = memmove((&llvm_cbe_slot_913437_0_lcssa14144_us_lcssa[llvm_cbe_tmp3969]), llvm_cbe_slot_913437_0_lcssa14144_us_lcssa, ((llvm_cbe_tmp398813449 - llvm_cbe_i3185_1013432_0_lcssa14142_us_lcssa) * llvm_cbe_tmp3969));
  llvm_cbe_slot_913437_0_us61_0__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_14;   /* for PHI node */
  llvm_cbe_slot_913437_059_0__PHI_TEMPORARY = llvm_cbe_slot_913437_059_14;   /* for PHI node */
  llvm_cbe_slot_913437_1__PHI_TEMPORARY = llvm_cbe_slot_913437_0_lcssa14144_us_lcssa;   /* for PHI node */
  goto llvm_cbe_bb3993;

llvm_cbe_cond_true3961_split_loopexit:
  llvm_cbe_slot_913437_0_us_le = &llvm_cbe_tmp3924[llvm_cbe_slot_913437_0_us_rec];
  llvm_cbe_slot_913437_0_us61_14__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us_le;   /* for PHI node */
  llvm_cbe_slot_913437_059_14__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_slot_913437_0_lcssa14144_us_lcssa__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us_le;   /* for PHI node */
  llvm_cbe_i3185_1013432_0_lcssa14142_us_lcssa__PHI_TEMPORARY = llvm_cbe_i3185_1013432_0_us;   /* for PHI node */
  goto llvm_cbe_cond_true3961_split;

  do {     /* Syntactic loop 'bb3925.us' to make GCC happy */
llvm_cbe_bb3925_us:
  llvm_cbe_i3185_1013432_0_us = llvm_cbe_i3185_1013432_0_us__PHI_TEMPORARY;
  llvm_cbe_slot_913437_0_us_rec = llvm_cbe_slot_913437_0_us_rec__PHI_TEMPORARY;
  llvm_cbe_tmp3930_us = memcmp(llvm_cbe_tmp3851, (&llvm_cbe_tmp3924[(llvm_cbe_slot_913437_0_us_rec + ((unsigned int )2))]), llvm_cbe_tmp3875);
  if ((llvm_cbe_tmp3930_us == ((unsigned int )0))) {
    goto llvm_cbe_cond_true3935_us;
  } else {
    llvm_cbe_crc_10_us__PHI_TEMPORARY = llvm_cbe_tmp3930_us;   /* for PHI node */
    goto llvm_cbe_cond_next3956_us;
  }

llvm_cbe_bb3985_us:
  llvm_cbe_tmp3980_us = *llvm_cbe_tmp3506;
  llvm_cbe_tmp3982_us_rec = llvm_cbe_slot_913437_0_us_rec + llvm_cbe_tmp3980_us;
  llvm_cbe_tmp3984_us = llvm_cbe_i3185_1013432_0_us + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp398813449) > ((signed int )llvm_cbe_tmp3984_us))) {
    llvm_cbe_i3185_1013432_0_us__PHI_TEMPORARY = llvm_cbe_tmp3984_us;   /* for PHI node */
    llvm_cbe_slot_913437_0_us_rec__PHI_TEMPORARY = llvm_cbe_tmp3982_us_rec;   /* for PHI node */
    goto llvm_cbe_bb3925_us;
  } else {
    goto llvm_cbe_bb3993_loopexit;
  }

llvm_cbe_cond_next3956_us:
  llvm_cbe_crc_10_us = llvm_cbe_crc_10_us__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_crc_10_us) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true3961_split_loopexit;
  } else {
    goto llvm_cbe_bb3985_us;
  }

llvm_cbe_cond_true3935_us:
  llvm_cbe_tmp3940_us = *(&llvm_cbe_tmp3924[(llvm_cbe_slot_913437_0_us_rec + llvm_cbe_tmp3937)]);
  if ((llvm_cbe_tmp3940_us == ((unsigned char )0))) {
    goto llvm_cbe_cond_true3951_split;
  } else {
    llvm_cbe_crc_10_us__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_cond_next3956_us;
  }

  } while (1); /* end of syntactic loop 'bb3925.us' */
llvm_cbe_bb3925_preheader:
  llvm_cbe_tmp3937 = llvm_cbe_tmp3875 + ((unsigned int )2);
  if (((llvm_cbe_options104_39466_9 & ((unsigned int )524288)) == ((unsigned int )0))) {
    llvm_cbe_i3185_1013432_0_us__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_slot_913437_0_us_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3925_us;
  } else {
    llvm_cbe_i3185_1013432_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_slot_913437_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb3925;
  }

llvm_cbe_cond_true3961_split_loopexit56:
  llvm_cbe_slot_913437_0_le = &llvm_cbe_tmp3924[llvm_cbe_slot_913437_0_rec];
  llvm_cbe_slot_913437_0_us61_14__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_14__PHI_TEMPORARY = llvm_cbe_slot_913437_0_le;   /* for PHI node */
  llvm_cbe_slot_913437_0_lcssa14144_us_lcssa__PHI_TEMPORARY = llvm_cbe_slot_913437_0_le;   /* for PHI node */
  llvm_cbe_i3185_1013432_0_lcssa14142_us_lcssa__PHI_TEMPORARY = llvm_cbe_i3185_1013432_0;   /* for PHI node */
  goto llvm_cbe_cond_true3961_split;

  do {     /* Syntactic loop 'bb3925' to make GCC happy */
llvm_cbe_bb3925:
  llvm_cbe_i3185_1013432_0 = llvm_cbe_i3185_1013432_0__PHI_TEMPORARY;
  llvm_cbe_slot_913437_0_rec = llvm_cbe_slot_913437_0_rec__PHI_TEMPORARY;
  llvm_cbe_tmp3930 = memcmp(llvm_cbe_tmp3851, (&llvm_cbe_tmp3924[(llvm_cbe_slot_913437_0_rec + ((unsigned int )2))]), llvm_cbe_tmp3875);
  if ((llvm_cbe_tmp3930 == ((unsigned int )0))) {
    goto llvm_cbe_cond_true3935;
  } else {
    llvm_cbe_crc_10__PHI_TEMPORARY = llvm_cbe_tmp3930;   /* for PHI node */
    goto llvm_cbe_cond_next3956;
  }

llvm_cbe_bb3985:
  llvm_cbe_tmp3980 = *llvm_cbe_tmp3506;
  llvm_cbe_tmp3982_rec = llvm_cbe_slot_913437_0_rec + llvm_cbe_tmp3980;
  llvm_cbe_tmp3984 = llvm_cbe_i3185_1013432_0 + ((unsigned int )1);
  if ((((signed int )llvm_cbe_tmp398813449) > ((signed int )llvm_cbe_tmp3984))) {
    llvm_cbe_i3185_1013432_0__PHI_TEMPORARY = llvm_cbe_tmp3984;   /* for PHI node */
    llvm_cbe_slot_913437_0_rec__PHI_TEMPORARY = llvm_cbe_tmp3982_rec;   /* for PHI node */
    goto llvm_cbe_bb3925;
  } else {
    goto llvm_cbe_bb3993_loopexit55;
  }

llvm_cbe_cond_next3956:
  llvm_cbe_crc_10 = llvm_cbe_crc_10__PHI_TEMPORARY;
  if ((((signed int )llvm_cbe_crc_10) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true3961_split_loopexit56;
  } else {
    goto llvm_cbe_bb3985;
  }

llvm_cbe_cond_true3935:
  llvm_cbe_tmp3940 = *(&llvm_cbe_tmp3924[(llvm_cbe_slot_913437_0_rec + llvm_cbe_tmp3937)]);
  if ((llvm_cbe_tmp3940 == ((unsigned char )0))) {
    goto llvm_cbe_cond_true3944;
  } else {
    llvm_cbe_crc_10__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_cond_next3956;
  }

llvm_cbe_cond_true3944:
  llvm_cbe_crc_10__PHI_TEMPORARY = llvm_cbe_tmp3930;   /* for PHI node */
  goto llvm_cbe_cond_next3956;

  } while (1); /* end of syntactic loop 'bb3925' */
llvm_cbe_bb3993_loopexit:
  llvm_cbe_slot_913437_0_us = &llvm_cbe_tmp3924[llvm_cbe_slot_913437_0_us_rec];
  llvm_cbe_tmp3982_us = &llvm_cbe_tmp3924[llvm_cbe_tmp3982_us_rec];
  llvm_cbe_slot_913437_0_us61_0__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us;   /* for PHI node */
  llvm_cbe_slot_913437_059_0__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_slot_913437_1__PHI_TEMPORARY = llvm_cbe_tmp3982_us;   /* for PHI node */
  goto llvm_cbe_bb3993;

llvm_cbe_bb3993_loopexit55:
  llvm_cbe_slot_913437_0 = &llvm_cbe_tmp3924[llvm_cbe_slot_913437_0_rec];
  llvm_cbe_tmp3982 = &llvm_cbe_tmp3924[llvm_cbe_tmp3982_rec];
  llvm_cbe_slot_913437_0_us61_0__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_0__PHI_TEMPORARY = llvm_cbe_slot_913437_0;   /* for PHI node */
  llvm_cbe_slot_913437_1__PHI_TEMPORARY = llvm_cbe_tmp3982;   /* for PHI node */
  goto llvm_cbe_bb3993;

llvm_cbe_cond_true4741:
  if ((((signed int )llvm_cbe_tmp4716) > ((signed int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true4746;
  } else {
    llvm_cbe_bravalue_9__PHI_TEMPORARY = ((unsigned int )101);   /* for PHI node */
    goto llvm_cbe_cond_next4765;
  }

llvm_cbe_bb4734:
  llvm_cbe_tmp4737 = *(&llvm_cbe_code105_10[((unsigned int )3)]);
  if ((llvm_cbe_tmp4737 == ((unsigned char )101))) {
    goto llvm_cbe_cond_true4741;
  } else {
    goto llvm_cbe_cond_false4749;
  }

  do {     /* Syntactic loop 'bb4714' to make GCC happy */
llvm_cbe_bb4714:
  llvm_cbe_condcount_6 = llvm_cbe_condcount_6__PHI_TEMPORARY;
  llvm_cbe_tc_6_rec = llvm_cbe_tc_6_rec__PHI_TEMPORARY;
  llvm_cbe_tmp4716 = llvm_cbe_condcount_6 + ((unsigned int )1);
  llvm_cbe_tmp4719 = *(&llvm_cbe_code105_10[(llvm_cbe_tc_6_rec + ((unsigned int )1))]);
  llvm_cbe_tmp4724 = *(&llvm_cbe_code105_10[(llvm_cbe_tc_6_rec + ((unsigned int )2))]);
  llvm_cbe_tmp4728_rec = llvm_cbe_tc_6_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp4719)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp4724)));
  llvm_cbe_tmp4730 = *(&llvm_cbe_code105_10[llvm_cbe_tmp4728_rec]);
  if ((llvm_cbe_tmp4730 == ((unsigned char )84))) {
    goto llvm_cbe_bb4734;
  } else {
    llvm_cbe_condcount_6__PHI_TEMPORARY = llvm_cbe_tmp4716;   /* for PHI node */
    llvm_cbe_tc_6_rec__PHI_TEMPORARY = llvm_cbe_tmp4728_rec;   /* for PHI node */
    goto llvm_cbe_bb4714;
  }

  } while (1); /* end of syntactic loop 'bb4714' */
llvm_cbe_cond_next4756:
  if ((llvm_cbe_tmp4716 == ((unsigned int )1))) {
    goto llvm_cbe_cond_true4761;
  } else {
    llvm_cbe_bravalue_9__PHI_TEMPORARY = llvm_cbe_bravalue_8;   /* for PHI node */
    goto llvm_cbe_cond_next4765;
  }

llvm_cbe_cond_false4749:
  if ((((signed int )llvm_cbe_tmp4716) > ((signed int )((unsigned int )2)))) {
    goto llvm_cbe_cond_true4754;
  } else {
    goto llvm_cbe_cond_next4756;
  }

llvm_cbe_cond_true4761:
  *(&llvm_cbe_subreqbyte) = ((unsigned int )-1);
  *(&llvm_cbe_subfirstbyte) = ((unsigned int )-1);
  llvm_cbe_bravalue_9__PHI_TEMPORARY = llvm_cbe_bravalue_8;   /* for PHI node */
  goto llvm_cbe_cond_next4765;

llvm_cbe_cond_false4802:
  llvm_cbe_tmp4803 = *(&llvm_cbe_tempcode);
  llvm_cbe_code105_26__PHI_TEMPORARY = llvm_cbe_tmp4803;   /* for PHI node */
  goto llvm_cbe_cond_next4804;

llvm_cbe_cond_true4827:
  llvm_cbe_groupsetfirstbyte_4__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
  llvm_cbe_zerofirstbyte_3__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  llvm_cbe_firstbyte102_11__PHI_TEMPORARY = llvm_cbe_tmp4823;   /* for PHI node */
  goto llvm_cbe_cond_next4847;

llvm_cbe_cond_false4831:
  if ((((signed int )llvm_cbe_tmp4823) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true4836;
  } else {
    llvm_cbe_groupsetfirstbyte_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_zerofirstbyte_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_firstbyte102_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    goto llvm_cbe_cond_next4847;
  }

llvm_cbe_cond_true4836:
  llvm_cbe_tmp4837 = *(&llvm_cbe_subreqbyte);
  if ((((signed int )llvm_cbe_tmp4837) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true4841;
  } else {
    llvm_cbe_groupsetfirstbyte_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_zerofirstbyte_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_firstbyte102_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    goto llvm_cbe_cond_next4847;
  }

llvm_cbe_cond_true4841:
  *(&llvm_cbe_subreqbyte) = (llvm_cbe_tmp4823 | llvm_cbe_tmp4674);
  llvm_cbe_groupsetfirstbyte_4__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_zerofirstbyte_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_firstbyte102_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  goto llvm_cbe_cond_next4847;

llvm_cbe_cond_true4865:
  llvm_cbe_tmp519613495 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613495[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_iftmp_467_0;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_26;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_tmp4861;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_true4860:
  llvm_cbe_tmp4861 = *(&llvm_cbe_subreqbyte);
  if ((((signed int )llvm_cbe_tmp4861) > ((signed int )((unsigned int )-1)))) {
    goto llvm_cbe_cond_true4865;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_iftmp_467_0;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_26;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_cond_false4855:
  if ((llvm_cbe_bravalue_9 == ((unsigned int )87))) {
    goto llvm_cbe_cond_true4860;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_iftmp_467_0;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_26;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_cond_next4910:
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp4895[((unsigned int )3)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next4902:
  llvm_cbe_tmp4905 = *(&llvm_cbe_tmp4895[((unsigned int )2)]);
  if ((llvm_cbe_tmp4905 == ((unsigned char )69))) {
    goto llvm_cbe_cond_next4910;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_cond_true4894:
  llvm_cbe_tmp4895 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4897 = *(&llvm_cbe_tmp4895[((unsigned int )1)]);
  if ((llvm_cbe_tmp4897 == ((unsigned char )92))) {
    goto llvm_cbe_cond_next4902;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_cond_true5048:
  *llvm_cbe_tmp5042 = llvm_cbe_recno_13;
  llvm_cbe_tmp519613501 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613501[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_14;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp5024;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_5;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_5;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_16;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_HANDLE_REFERENCE:
  llvm_cbe_recno_13 = llvm_cbe_recno_13__PHI_TEMPORARY;
  llvm_cbe_save_hwm_14 = llvm_cbe_save_hwm_14__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_5 = llvm_cbe_zerofirstbyte_5__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_5 = llvm_cbe_zeroreqbyte_5__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_14 = llvm_cbe_firstbyte102_14__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_16 = (((llvm_cbe_firstbyte102_14 == ((unsigned int )-2))) ? (((unsigned int )-1)) : (llvm_cbe_firstbyte102_14));
  *llvm_cbe_code105_10 = ((unsigned char )80);
  *(&llvm_cbe_code105_10[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_recno_13) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code105_10[((unsigned int )2)]) = (((unsigned char )llvm_cbe_recno_13));
  llvm_cbe_tmp5024 = &llvm_cbe_code105_10[((unsigned int )3)];
  llvm_cbe_tmp5027 = *llvm_cbe_tmp5026;
  *llvm_cbe_tmp5026 = (llvm_cbe_tmp5027 | ((((((signed int )llvm_cbe_recno_13) < ((signed int )((unsigned int )32)))) ? ((((unsigned int )1) << llvm_cbe_recno_13)) : (((unsigned int )1)))));
  llvm_cbe_tmp5043 = *llvm_cbe_tmp5042;
  if ((((signed int )llvm_cbe_tmp5043) < ((signed int )llvm_cbe_recno_13))) {
    goto llvm_cbe_cond_true5048;
  } else {
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_save_hwm_14;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_tmp5024;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_5;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_5;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_16;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }

llvm_cbe_cond_true4999:
  llvm_cbe_tmp5002 = ((unsigned int )-27) - llvm_cbe_tmp4877;
  llvm_cbe_recno_13__PHI_TEMPORARY = llvm_cbe_tmp5002;   /* for PHI node */
  llvm_cbe_save_hwm_14__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_5__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  llvm_cbe_zeroreqbyte_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_14__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  goto llvm_cbe_HANDLE_REFERENCE;

llvm_cbe_cond_next4993:
  if ((((signed int )(-(llvm_cbe_tmp4877))) > ((signed int )((unsigned int )26)))) {
    goto llvm_cbe_cond_true4999;
  } else {
    goto llvm_cbe_cond_false5053;
  }

llvm_cbe_bb5073:
  llvm_cbe_iftmp_490_0 = (((((unsigned int )(llvm_cbe_tmp4877 + ((unsigned int )21))) < ((unsigned int )((unsigned int )16)))) ? (llvm_cbe_code105_10) : (((unsigned char *)/*NULL*/0)));
  *llvm_cbe_code105_10 = (((unsigned char )(-((((unsigned char )llvm_cbe_tmp4877))))));
  llvm_cbe_tmp5089 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_tmp519613505 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613505[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_iftmp_490_0;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_tmp5089;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_12;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_false5053:
  switch (llvm_cbe_tmp4877) {
  default:
    goto llvm_cbe_bb5073;
;
  case ((unsigned int )-21):
    goto llvm_cbe_bb5071;
    break;
  case ((unsigned int )-15):
    goto llvm_cbe_bb5071;
    break;
  case ((unsigned int )-14):
    goto llvm_cbe_bb5071;
    break;
  }
llvm_cbe_cond_true5144:
  llvm_cbe_tmp5146 = *llvm_cbe_tmp1755;
  llvm_cbe_tmp5149 = (((unsigned int )(unsigned char )llvm_cbe_tmp5146)) | llvm_cbe_req_caseopt_39708_14;
  llvm_cbe_tmp519613508 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613508[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_14;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_14;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_312578_0;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_code105_812582_0;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_14;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_612580_0;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_11;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_27;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_312586_0;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_14;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_14;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_14;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_14;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_tmp5149;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_14;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_14;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_bb5126:
  if ((llvm_cbe_firstbyte102_39829_14 == ((unsigned int )-2))) {
    goto llvm_cbe_cond_true5144;
  } else {
    goto llvm_cbe_cond_true5181;
  }

llvm_cbe_bb5120:
  llvm_cbe_last_code_15771_11 = llvm_cbe_last_code_15771_11__PHI_TEMPORARY;
  llvm_cbe_options_addr_25789_14 = llvm_cbe_options_addr_25789_14__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_14 = llvm_cbe_save_hwm_79347_14__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_14 = llvm_cbe_groupsetfirstbyte_29395_14__PHI_TEMPORARY;
  llvm_cbe_options104_39466_14 = llvm_cbe_options104_39466_14__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_14 = llvm_cbe_req_caseopt_39708_14__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_14 = llvm_cbe_reqbyte103_59784_14__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_14 = llvm_cbe_firstbyte102_39829_14__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_14 = llvm_cbe_greedy_non_default_39879_14__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_14 = llvm_cbe_greedy_default_39909_14__PHI_TEMPORARY;
  llvm_cbe_previous_callout_312578_0 = llvm_cbe_previous_callout_312578_0__PHI_TEMPORARY;
  llvm_cbe_inescq_612580_0 = llvm_cbe_inescq_612580_0__PHI_TEMPORARY;
  llvm_cbe_code105_812582_0 = llvm_cbe_code105_812582_0__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_312586_0 = llvm_cbe_after_manual_callout_312586_0__PHI_TEMPORARY;
  llvm_cbe_code105_8_pn = llvm_cbe_code105_8_pn__PHI_TEMPORARY;
  llvm_cbe_c_38 = llvm_cbe_c_38__PHI_TEMPORARY;
  llvm_cbe_code105_27 = &llvm_cbe_code105_8_pn[((unsigned int )1)];
  if ((((signed int )llvm_cbe_c_38) < ((signed int )((unsigned int )1)))) {
    llvm_cbe_last_code_15771_5__PHI_TEMPORARY = llvm_cbe_last_code_15771_11;   /* for PHI node */
    llvm_cbe_options_addr_25789_12__PHI_TEMPORARY = llvm_cbe_options_addr_25789_14;   /* for PHI node */
    llvm_cbe_save_hwm_79347_12__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_14;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_12__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_14;   /* for PHI node */
    llvm_cbe_options104_39466_12__PHI_TEMPORARY = llvm_cbe_options104_39466_14;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_12__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_14;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_12__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_14;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_12__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_14;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_12__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_14;   /* for PHI node */
    llvm_cbe_greedy_default_39909_12__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_14;   /* for PHI node */
    llvm_cbe_previous_callout_312578_1__PHI_TEMPORARY = llvm_cbe_previous_callout_312578_0;   /* for PHI node */
    llvm_cbe_inescq_612580_1__PHI_TEMPORARY = llvm_cbe_inescq_612580_0;   /* for PHI node */
    llvm_cbe_code105_812582_1__PHI_TEMPORARY = llvm_cbe_code105_812582_0;   /* for PHI node */
    llvm_cbe_after_manual_callout_312586_1__PHI_TEMPORARY = llvm_cbe_after_manual_callout_312586_0;   /* for PHI node */
    llvm_cbe_c_3812612_0__PHI_TEMPORARY = llvm_cbe_c_38;   /* for PHI node */
    llvm_cbe_code105_2712615_0__PHI_TEMPORARY = llvm_cbe_code105_27;   /* for PHI node */
    goto llvm_cbe_bb5111;
  } else {
    goto llvm_cbe_bb5126;
  }

llvm_cbe_ONE_CHAR:
  llvm_cbe_storemerge26707_in = llvm_cbe_storemerge26707_in__PHI_TEMPORARY;
  llvm_cbe_last_code_15771_4 = llvm_cbe_last_code_15771_4__PHI_TEMPORARY;
  llvm_cbe_options_addr_25789_11 = llvm_cbe_options_addr_25789_11__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_11 = llvm_cbe_save_hwm_79347_11__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_11 = llvm_cbe_groupsetfirstbyte_29395_11__PHI_TEMPORARY;
  llvm_cbe_options104_39466_11 = llvm_cbe_options104_39466_11__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_11 = llvm_cbe_req_caseopt_39708_11__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_11 = llvm_cbe_reqbyte103_59784_11__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_11 = llvm_cbe_firstbyte102_39829_11__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_11 = llvm_cbe_greedy_non_default_39879_11__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_11 = llvm_cbe_greedy_default_39909_11__PHI_TEMPORARY;
  llvm_cbe_previous_callout_3 = llvm_cbe_previous_callout_3__PHI_TEMPORARY;
  llvm_cbe_inescq_6 = llvm_cbe_inescq_6__PHI_TEMPORARY;
  llvm_cbe_code105_8 = llvm_cbe_code105_8__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_3 = llvm_cbe_after_manual_callout_3__PHI_TEMPORARY;
  *llvm_cbe_tmp1755 = (((unsigned char )llvm_cbe_storemerge26707_in));
  *llvm_cbe_code105_8 = (((((llvm_cbe_options104_39466_11 & ((unsigned int )1)) == ((unsigned int )0))) ? (((unsigned char )27)) : (((unsigned char )28))));
  llvm_cbe_last_code_15771_11__PHI_TEMPORARY = llvm_cbe_last_code_15771_4;   /* for PHI node */
  llvm_cbe_options_addr_25789_14__PHI_TEMPORARY = llvm_cbe_options_addr_25789_11;   /* for PHI node */
  llvm_cbe_save_hwm_79347_14__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_11;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_29395_14__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_11;   /* for PHI node */
  llvm_cbe_options104_39466_14__PHI_TEMPORARY = llvm_cbe_options104_39466_11;   /* for PHI node */
  llvm_cbe_req_caseopt_39708_14__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_11;   /* for PHI node */
  llvm_cbe_reqbyte103_59784_14__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_11;   /* for PHI node */
  llvm_cbe_firstbyte102_39829_14__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_11;   /* for PHI node */
  llvm_cbe_greedy_non_default_39879_14__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_11;   /* for PHI node */
  llvm_cbe_greedy_default_39909_14__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_11;   /* for PHI node */
  llvm_cbe_previous_callout_312578_0__PHI_TEMPORARY = llvm_cbe_previous_callout_3;   /* for PHI node */
  llvm_cbe_inescq_612580_0__PHI_TEMPORARY = llvm_cbe_inescq_6;   /* for PHI node */
  llvm_cbe_code105_812582_0__PHI_TEMPORARY = llvm_cbe_code105_8;   /* for PHI node */
  llvm_cbe_after_manual_callout_312586_0__PHI_TEMPORARY = llvm_cbe_after_manual_callout_3;   /* for PHI node */
  llvm_cbe_code105_8_pn__PHI_TEMPORARY = llvm_cbe_code105_8;   /* for PHI node */
  llvm_cbe_c_38__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb5120;

llvm_cbe_cond_next259:
  if (((llvm_cbe_options104_39466_8 & ((unsigned int )16384)) == ((unsigned int )0))) {
    llvm_cbe_storemerge26707_in__PHI_TEMPORARY = llvm_cbe_tmp13513612358_8;   /* for PHI node */
    llvm_cbe_last_code_15771_4__PHI_TEMPORARY = llvm_cbe_last_code_15771_0;   /* for PHI node */
    llvm_cbe_options_addr_25789_11__PHI_TEMPORARY = llvm_cbe_options_addr_25789_8;   /* for PHI node */
    llvm_cbe_save_hwm_79347_11__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_8;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_29395_11__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_8;   /* for PHI node */
    llvm_cbe_options104_39466_11__PHI_TEMPORARY = llvm_cbe_options104_39466_8;   /* for PHI node */
    llvm_cbe_req_caseopt_39708_11__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_8;   /* for PHI node */
    llvm_cbe_reqbyte103_59784_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_8;   /* for PHI node */
    llvm_cbe_firstbyte102_39829_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_8;   /* for PHI node */
    llvm_cbe_greedy_non_default_39879_11__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_8;   /* for PHI node */
    llvm_cbe_greedy_default_39909_11__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_8;   /* for PHI node */
    llvm_cbe_previous_callout_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
    llvm_cbe_inescq_6__PHI_TEMPORARY = llvm_cbe_inescq_29422_8;   /* for PHI node */
    llvm_cbe_code105_8__PHI_TEMPORARY = llvm_cbe_code105_25773_0;   /* for PHI node */
    llvm_cbe_after_manual_callout_3__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_8;   /* for PHI node */
    goto llvm_cbe_ONE_CHAR;
  } else {
    goto llvm_cbe_cond_true265;
  }

llvm_cbe_bb244:
  if (((llvm_cbe_previous_callout_59381_8 == ((unsigned char *)/*NULL*/0)) | llvm_cbe_tmp138_not)) {
    goto llvm_cbe_cond_next259;
  } else {
    goto llvm_cbe_cond_true254;
  }

llvm_cbe_cond_true254:
  llvm_cbe_tmp255 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp4_i = *llvm_cbe_tmp3783;
  llvm_cbe_tmp9_i = *(&llvm_cbe_previous_callout_59381_8[((unsigned int )2)]);
  llvm_cbe_tmp14_i = *(&llvm_cbe_previous_callout_59381_8[((unsigned int )3)]);
  llvm_cbe_tmp17_i = ((((unsigned int )(unsigned long)llvm_cbe_tmp255)) - (((unsigned int )(unsigned long)llvm_cbe_tmp4_i))) - (((((unsigned int )(unsigned char )llvm_cbe_tmp9_i)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp14_i)));
  *(&llvm_cbe_previous_callout_59381_8[((unsigned int )4)]) = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp17_i) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_previous_callout_59381_8[((unsigned int )5)]) = (((unsigned char )llvm_cbe_tmp17_i));
  goto llvm_cbe_cond_next259;

llvm_cbe_cond_true265:
  llvm_cbe_tmp267 = *(&llvm_cbe_ptr106);
  *llvm_cbe_code105_25773_0 = ((unsigned char )82);
  *(&llvm_cbe_code105_25773_0[((unsigned int )1)]) = ((unsigned char )-1);
  llvm_cbe_tmp11_i4 = *llvm_cbe_tmp3783;
  *(&llvm_cbe_code105_25773_0[((unsigned int )2)]) = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)llvm_cbe_tmp267)) - (((unsigned int )(unsigned long)llvm_cbe_tmp11_i4)))) >> ((unsigned int )((unsigned int )8)))))));
  llvm_cbe_tmp23_i = *llvm_cbe_tmp3783;
  *(&llvm_cbe_code105_25773_0[((unsigned int )3)]) = (((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_tmp267)) - (((unsigned char )(unsigned long)llvm_cbe_tmp23_i)))));
  *(&llvm_cbe_code105_25773_0[((unsigned int )4)]) = ((unsigned char )0);
  *(&llvm_cbe_code105_25773_0[((unsigned int )5)]) = ((unsigned char )0);
  llvm_cbe_tmp35_i = &llvm_cbe_code105_25773_0[((unsigned int )6)];
  llvm_cbe_storemerge26707_in__PHI_TEMPORARY = llvm_cbe_tmp13513612358_8;   /* for PHI node */
  llvm_cbe_last_code_15771_4__PHI_TEMPORARY = llvm_cbe_last_code_15771_0;   /* for PHI node */
  llvm_cbe_options_addr_25789_11__PHI_TEMPORARY = llvm_cbe_options_addr_25789_8;   /* for PHI node */
  llvm_cbe_save_hwm_79347_11__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_8;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_29395_11__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_8;   /* for PHI node */
  llvm_cbe_options104_39466_11__PHI_TEMPORARY = llvm_cbe_options104_39466_8;   /* for PHI node */
  llvm_cbe_req_caseopt_39708_11__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_8;   /* for PHI node */
  llvm_cbe_reqbyte103_59784_11__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_8;   /* for PHI node */
  llvm_cbe_firstbyte102_39829_11__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_8;   /* for PHI node */
  llvm_cbe_greedy_non_default_39879_11__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_8;   /* for PHI node */
  llvm_cbe_greedy_default_39909_11__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_8;   /* for PHI node */
  llvm_cbe_previous_callout_3__PHI_TEMPORARY = llvm_cbe_code105_25773_0;   /* for PHI node */
  llvm_cbe_inescq_6__PHI_TEMPORARY = llvm_cbe_inescq_29422_8;   /* for PHI node */
  llvm_cbe_code105_8__PHI_TEMPORARY = llvm_cbe_tmp35_i;   /* for PHI node */
  llvm_cbe_after_manual_callout_3__PHI_TEMPORARY = llvm_cbe_after_manual_callout_79460_8;   /* for PHI node */
  goto llvm_cbe_ONE_CHAR;

llvm_cbe_bb5111:
  llvm_cbe_last_code_15771_5 = llvm_cbe_last_code_15771_5__PHI_TEMPORARY;
  llvm_cbe_options_addr_25789_12 = llvm_cbe_options_addr_25789_12__PHI_TEMPORARY;
  llvm_cbe_save_hwm_79347_12 = llvm_cbe_save_hwm_79347_12__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_29395_12 = llvm_cbe_groupsetfirstbyte_29395_12__PHI_TEMPORARY;
  llvm_cbe_options104_39466_12 = llvm_cbe_options104_39466_12__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_39708_12 = llvm_cbe_req_caseopt_39708_12__PHI_TEMPORARY;
  llvm_cbe_reqbyte103_59784_12 = llvm_cbe_reqbyte103_59784_12__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_39829_12 = llvm_cbe_firstbyte102_39829_12__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_39879_12 = llvm_cbe_greedy_non_default_39879_12__PHI_TEMPORARY;
  llvm_cbe_greedy_default_39909_12 = llvm_cbe_greedy_default_39909_12__PHI_TEMPORARY;
  llvm_cbe_previous_callout_312578_1 = llvm_cbe_previous_callout_312578_1__PHI_TEMPORARY;
  llvm_cbe_inescq_612580_1 = llvm_cbe_inescq_612580_1__PHI_TEMPORARY;
  llvm_cbe_code105_812582_1 = llvm_cbe_code105_812582_1__PHI_TEMPORARY;
  llvm_cbe_after_manual_callout_312586_1 = llvm_cbe_after_manual_callout_312586_1__PHI_TEMPORARY;
  llvm_cbe_c_3812612_0 = llvm_cbe_c_3812612_0__PHI_TEMPORARY;
  llvm_cbe_code105_2712615_0 = llvm_cbe_code105_2712615_0__PHI_TEMPORARY;
  llvm_cbe_tmp5114 = *(&llvm_cbe_mcbuffer[llvm_cbe_c_3812612_0]);
  *llvm_cbe_code105_2712615_0 = llvm_cbe_tmp5114;
  llvm_cbe_tmp5119 = llvm_cbe_c_3812612_0 + ((unsigned int )1);
  llvm_cbe_last_code_15771_11__PHI_TEMPORARY = llvm_cbe_last_code_15771_5;   /* for PHI node */
  llvm_cbe_options_addr_25789_14__PHI_TEMPORARY = llvm_cbe_options_addr_25789_12;   /* for PHI node */
  llvm_cbe_save_hwm_79347_14__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_12;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_29395_14__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_12;   /* for PHI node */
  llvm_cbe_options104_39466_14__PHI_TEMPORARY = llvm_cbe_options104_39466_12;   /* for PHI node */
  llvm_cbe_req_caseopt_39708_14__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_12;   /* for PHI node */
  llvm_cbe_reqbyte103_59784_14__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_12;   /* for PHI node */
  llvm_cbe_firstbyte102_39829_14__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_12;   /* for PHI node */
  llvm_cbe_greedy_non_default_39879_14__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_12;   /* for PHI node */
  llvm_cbe_greedy_default_39909_14__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_12;   /* for PHI node */
  llvm_cbe_previous_callout_312578_0__PHI_TEMPORARY = llvm_cbe_previous_callout_312578_1;   /* for PHI node */
  llvm_cbe_inescq_612580_0__PHI_TEMPORARY = llvm_cbe_inescq_612580_1;   /* for PHI node */
  llvm_cbe_code105_812582_0__PHI_TEMPORARY = llvm_cbe_code105_812582_1;   /* for PHI node */
  llvm_cbe_after_manual_callout_312586_0__PHI_TEMPORARY = llvm_cbe_after_manual_callout_312586_1;   /* for PHI node */
  llvm_cbe_code105_8_pn__PHI_TEMPORARY = llvm_cbe_code105_2712615_0;   /* for PHI node */
  llvm_cbe_c_38__PHI_TEMPORARY = llvm_cbe_tmp5119;   /* for PHI node */
  goto llvm_cbe_bb5120;

llvm_cbe_cond_next1752:
  *llvm_cbe_tmp1755 = (((unsigned char )llvm_cbe_class_lastchar_8));
  *llvm_cbe_code105_10 = (((((llvm_cbe_options104_39466_9 & ((unsigned int )1)) == ((unsigned int )0))) ? (((unsigned char )27)) : (((unsigned char )28))));
  llvm_cbe_code105_2712620 = &llvm_cbe_code105_10[((unsigned int )1)];
  llvm_cbe_last_code_15771_5__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_options_addr_25789_12__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_save_hwm_79347_12__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_9;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_29395_12__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
  llvm_cbe_options104_39466_12__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
  llvm_cbe_req_caseopt_39708_12__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
  llvm_cbe_reqbyte103_59784_12__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_39829_12__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_greedy_non_default_39879_12__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
  llvm_cbe_greedy_default_39909_12__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
  llvm_cbe_previous_callout_312578_1__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
  llvm_cbe_inescq_612580_1__PHI_TEMPORARY = llvm_cbe_inescq_5;   /* for PHI node */
  llvm_cbe_code105_812582_1__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_after_manual_callout_312586_1__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_c_3812612_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_code105_2712615_0__PHI_TEMPORARY = llvm_cbe_code105_2712620;   /* for PHI node */
  goto llvm_cbe_bb5111;

llvm_cbe_cond_true5181:
  llvm_cbe_tmp5184 = *llvm_cbe_code105_8_pn;
  llvm_cbe_tmp5190 = *llvm_cbe_tmp1993;
  llvm_cbe_tmp5191 = (llvm_cbe_tmp5190 | llvm_cbe_req_caseopt_39708_14) | (((unsigned int )(unsigned char )llvm_cbe_tmp5184));
  llvm_cbe_tmp519613511 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp519613511[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_25789_14;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_79347_14;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_312578_0;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_code105_812582_0;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_14;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_612580_0;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_11;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_27;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_312586_0;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_39466_14;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_14;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_14;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_14;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_tmp5191;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_14;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_14;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_14;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_bb5195:
  llvm_cbe_slot_913437_0_us61_5 = llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_5 = llvm_cbe_slot_913437_059_5__PHI_TEMPORARY;
  llvm_cbe_options_addr_1 = llvm_cbe_options_addr_1__PHI_TEMPORARY;
  llvm_cbe_save_hwm_6 = llvm_cbe_save_hwm_6__PHI_TEMPORARY;
  llvm_cbe_previous_callout_4 = llvm_cbe_previous_callout_4__PHI_TEMPORARY;
  llvm_cbe_previous_6 = llvm_cbe_previous_6__PHI_TEMPORARY;
  llvm_cbe_groupsetfirstbyte_1 = llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY;
  llvm_cbe_inescq_1 = llvm_cbe_inescq_1__PHI_TEMPORARY;
  llvm_cbe_code105_9 = llvm_cbe_code105_9__PHI_TEMPORARY;
  llvm_cbe_options104_2 = llvm_cbe_options104_2__PHI_TEMPORARY;
  llvm_cbe_req_caseopt_2 = llvm_cbe_req_caseopt_2__PHI_TEMPORARY;
  llvm_cbe_zerofirstbyte_1 = llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY;
  llvm_cbe_zeroreqbyte_1 = llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY;
  llvm_cbe_firstbyte102_2 = llvm_cbe_firstbyte102_2__PHI_TEMPORARY;
  llvm_cbe_greedy_non_default_2 = llvm_cbe_greedy_non_default_2__PHI_TEMPORARY;
  llvm_cbe_greedy_default_2 = llvm_cbe_greedy_default_2__PHI_TEMPORARY;
  llvm_cbe_tmp5196 = *(&llvm_cbe_ptr106);
  *(&llvm_cbe_ptr106) = (&llvm_cbe_tmp5196[((unsigned int )1)]);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_5;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_5;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = llvm_cbe_repeat_max_10;   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = llvm_cbe_repeat_min_10;   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_1;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = llvm_cbe_save_hwm_6;   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = llvm_cbe_previous_callout_4;   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = llvm_cbe_previous_6;   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_1;   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = llvm_cbe_inescq_1;   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_last_code_15771_1;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code105_9;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = llvm_cbe_after_manual_callout_1;   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options104_2;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_2;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_1;   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_1;   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = llvm_cbe_firstbyte102_2;   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_greedy_non_default_2;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_greedy_default_2;   /* for PHI node */
  goto llvm_cbe_bb131;

  do {     /* Syntactic loop 'bb3197' to make GCC happy */
llvm_cbe_bb3197:
  llvm_cbe_tmp3195_pn = llvm_cbe_tmp3195_pn__PHI_TEMPORARY;
  llvm_cbe_storemerge = &llvm_cbe_tmp3195_pn[((unsigned int )1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_storemerge;
  llvm_cbe_tmp3199 = *llvm_cbe_storemerge;
  switch (llvm_cbe_tmp3199) {
  default:
    llvm_cbe_tmp3195_pn__PHI_TEMPORARY = llvm_cbe_storemerge;   /* for PHI node */
    goto llvm_cbe_bb3197;
;
  case ((unsigned char )0):
    goto llvm_cbe_cond_true3217;
    break;
  case ((unsigned char )41):
    llvm_cbe_slot_913437_0_us61_5__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_5__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_1__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_save_hwm_6__PHI_TEMPORARY = llvm_cbe_tmp3176;   /* for PHI node */
    llvm_cbe_previous_callout_4__PHI_TEMPORARY = llvm_cbe_previous_callout_9;   /* for PHI node */
    llvm_cbe_previous_6__PHI_TEMPORARY = llvm_cbe_previous_25708_1;   /* for PHI node */
    llvm_cbe_groupsetfirstbyte_1__PHI_TEMPORARY = llvm_cbe_groupsetfirstbyte_29395_9;   /* for PHI node */
    llvm_cbe_inescq_1__PHI_TEMPORARY = llvm_cbe_inescq_29422_9;   /* for PHI node */
    llvm_cbe_code105_9__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_options104_2__PHI_TEMPORARY = llvm_cbe_options104_39466_9;   /* for PHI node */
    llvm_cbe_req_caseopt_2__PHI_TEMPORARY = llvm_cbe_req_caseopt_39708_9;   /* for PHI node */
    llvm_cbe_zerofirstbyte_1__PHI_TEMPORARY = llvm_cbe_zerofirstbyte_29740_9;   /* for PHI node */
    llvm_cbe_zeroreqbyte_1__PHI_TEMPORARY = llvm_cbe_zeroreqbyte_29762_9;   /* for PHI node */
    llvm_cbe_firstbyte102_2__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_greedy_non_default_2__PHI_TEMPORARY = llvm_cbe_greedy_non_default_39879_9;   /* for PHI node */
    llvm_cbe_greedy_default_2__PHI_TEMPORARY = llvm_cbe_greedy_default_39909_9;   /* for PHI node */
    goto llvm_cbe_bb5195;
  }
  } while (1); /* end of syntactic loop 'bb3197' */
  } while (1); /* end of syntactic loop 'bb131' */
llvm_cbe_cond_next66:
  llvm_cbe_code_2 = llvm_cbe_code_2__PHI_TEMPORARY;
  llvm_cbe_reverse_count_0 = llvm_cbe_reverse_count_0__PHI_TEMPORARY;
  *(&llvm_cbe_length_prevgroup) = ((unsigned int )0);
  *(&llvm_cbe_ptr106) = llvm_cbe_ptr_013604_1;
  llvm_cbe_tmp117 = (((unsigned int )(((unsigned int )llvm_cbe_options_addr_313605_1) >> ((unsigned int )((unsigned int )9))))) & ((unsigned int )1);
  llvm_cbe_tmp119 = llvm_cbe_tmp117 ^ ((unsigned int )1);
  llvm_cbe_req_caseopt_3_ph = ((((llvm_cbe_options_addr_313605_1 & ((unsigned int )1)) == ((unsigned int )0))) ? (((unsigned int )0)) : (((unsigned int )256)));
  llvm_cbe_tmp188189 = ((unsigned int )(unsigned long)llvm_cbe_code_2);
  llvm_cbe_slot_913437_0_us61_13__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_9;   /* for PHI node */
  llvm_cbe_slot_913437_059_13__PHI_TEMPORARY = llvm_cbe_slot_913437_059_9;   /* for PHI node */
  llvm_cbe_repeat_max_10__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_repeat_min_10__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_options_addr_2__PHI_TEMPORARY = llvm_cbe_options_addr_313605_1;   /* for PHI node */
  llvm_cbe_save_hwm_7__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_previous_callout_5__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_previous_3__PHI_TEMPORARY = ((unsigned char *)/*NULL*/0);   /* for PHI node */
  llvm_cbe_groupsetfirstbyte_2__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_inescq_2__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_last_code_2__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_code105_3__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_after_manual_callout_7__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_options104_3__PHI_TEMPORARY = llvm_cbe_options_addr_313605_1;   /* for PHI node */
  llvm_cbe_req_caseopt_3__PHI_TEMPORARY = llvm_cbe_req_caseopt_3_ph;   /* for PHI node */
  llvm_cbe_zerofirstbyte_2__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
  llvm_cbe_zeroreqbyte_2__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
  llvm_cbe_reqbyte103_5__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
  llvm_cbe_firstbyte102_3__PHI_TEMPORARY = ((unsigned int )-2);   /* for PHI node */
  llvm_cbe_greedy_non_default_3__PHI_TEMPORARY = llvm_cbe_tmp119;   /* for PHI node */
  llvm_cbe_greedy_default_3__PHI_TEMPORARY = llvm_cbe_tmp117;   /* for PHI node */
  goto llvm_cbe_bb131;

llvm_cbe_cond_next49:
  llvm_cbe_code_0 = llvm_cbe_code_0__PHI_TEMPORARY;
  if ((llvm_cbe_lookbehind == ((unsigned int )0))) {
    llvm_cbe_code_2__PHI_TEMPORARY = llvm_cbe_code_0;   /* for PHI node */
    llvm_cbe_reverse_count_0__PHI_TEMPORARY = llvm_cbe_reverse_count_113916_1;   /* for PHI node */
    goto llvm_cbe_cond_next66;
  } else {
    goto llvm_cbe_cond_true54;
  }

llvm_cbe_cond_true39:
  *llvm_cbe_code_113600_1 = ((unsigned char )24);
  *(&llvm_cbe_code_113600_1[((unsigned int )1)]) = (((unsigned char )((((unsigned char )llvm_cbe_options_addr_313605_1)) & ((unsigned char )7))));
  llvm_cbe_tmp46 = &llvm_cbe_code_113600_1[((unsigned int )2)];
  llvm_cbe_tmp47 = *(&llvm_cbe_length);
  *(&llvm_cbe_length) = (llvm_cbe_tmp47 + ((unsigned int )2));
  llvm_cbe_code_0__PHI_TEMPORARY = llvm_cbe_tmp46;   /* for PHI node */
  goto llvm_cbe_cond_next49;

llvm_cbe_cond_true54:
  *llvm_cbe_code_0 = ((unsigned char )91);
  llvm_cbe_tmp56 = &llvm_cbe_code_0[((unsigned int )1)];
  *llvm_cbe_tmp56 = ((unsigned char )0);
  *(&llvm_cbe_code_0[((unsigned int )2)]) = ((unsigned char )0);
  llvm_cbe_tmp63 = &llvm_cbe_code_0[((unsigned int )3)];
  llvm_cbe_tmp64 = *(&llvm_cbe_length);
  *(&llvm_cbe_length) = (llvm_cbe_tmp64 + ((unsigned int )3));
  llvm_cbe_code_2__PHI_TEMPORARY = llvm_cbe_tmp63;   /* for PHI node */
  llvm_cbe_reverse_count_0__PHI_TEMPORARY = llvm_cbe_tmp56;   /* for PHI node */
  goto llvm_cbe_cond_next66;

llvm_cbe_bb590:
  llvm_cbe_tmp597 = *(&llvm_cbe_ptr106);
  if ((llvm_cbe_iftmp_509_0 == ((unsigned int *)/*NULL*/0))) {
    llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
    llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
    llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
    llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
    llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
    llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
    llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp597;   /* for PHI node */
    llvm_cbe_tmp_0__PHI_TEMPORARY = 0;   /* for PHI node */
    goto llvm_cbe_bb5201;
  } else {
    goto llvm_cbe_cond_true603;
  }

llvm_cbe_cond_true603:
  llvm_cbe_tmp605 = *llvm_cbe_iftmp_509_0;
  *llvm_cbe_iftmp_509_0 = (((((unsigned int )(unsigned long)llvm_cbe_code105_10)) - (((unsigned int )(unsigned long)llvm_cbe_last_code_15771_1))) + llvm_cbe_tmp605);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_reqbyte103_59784_9;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_firstbyte102_39829_9;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code105_10;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp597;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 0;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true680:
  llvm_cbe_tmp681 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp683 = *(&llvm_cbe_tmp681[((unsigned int )1)]);
  *llvm_cbe_errorcodeptr = ((((llvm_cbe_tmp683 == ((unsigned char )58))) ? (((unsigned int )13)) : (((unsigned int )31))));
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp681;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true784:
  *llvm_cbe_errorcodeptr = ((unsigned int )31);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp778;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true861:
  *llvm_cbe_errorcodeptr = ((unsigned int )30);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp800;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true1402:
  *llvm_cbe_errorcodeptr = ((unsigned int )7);
  llvm_cbe_tmp519812479 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519812479;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true1582:
  *llvm_cbe_errorcodeptr = ((unsigned int )8);
  llvm_cbe_tmp519812520 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519812520;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true1723:
  *llvm_cbe_errorcodeptr = ((unsigned int )6);
  llvm_cbe_tmp519812532 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519812532;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true1928:
  *llvm_cbe_errorcodeptr = ((unsigned int )9);
  llvm_cbe_tmp519812674 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519812674;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true2501:
  *llvm_cbe_errorcodeptr = ((unsigned int )55);
  llvm_cbe_tmp519813229 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813229;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true2509:
  *llvm_cbe_errorcodeptr = ((unsigned int )50);
  llvm_cbe_tmp519813230 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813230;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_false3045:
  *llvm_cbe_errorcodeptr = ((unsigned int )11);
  llvm_cbe_tmp519813331 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813331;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3217:
  *llvm_cbe_errorcodeptr = ((unsigned int )18);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_storemerge;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3339:
  *llvm_cbe_errorcodeptr = ((unsigned int )28);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3328;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_bb3418:
  llvm_cbe_tmp3419 = *(&llvm_cbe_ptr106);
  llvm_cbe_tmp3420 = &llvm_cbe_tmp3419[((unsigned int )-1)];
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3420;
  *llvm_cbe_errorcodeptr = ((unsigned int )26);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3420;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3438:
  *llvm_cbe_errorcodeptr = ((unsigned int )58);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3416;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3456:
  *llvm_cbe_errorcodeptr = ((unsigned int )15);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3416;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3580:
  *llvm_cbe_errorcodeptr = ((unsigned int )15);
  llvm_cbe_tmp519813360 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813360;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3603:
  *llvm_cbe_errorcodeptr = ((unsigned int )15);
  llvm_cbe_tmp519813361 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813361;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_false3676:
  *llvm_cbe_errorcodeptr = ((((llvm_cbe_recno_7_lcssa == ((unsigned int )0))) ? (((unsigned int )35)) : (((unsigned int )15))));
  llvm_cbe_tmp519813362 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813362;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_next3725:
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp3700;
  *llvm_cbe_errorcodeptr = ((unsigned int )24);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3700;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3765:
  *llvm_cbe_errorcodeptr = ((unsigned int )39);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3760;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3772:
  *llvm_cbe_errorcodeptr = ((unsigned int )38);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3760;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3837:
  *llvm_cbe_errorcodeptr = ((unsigned int )41);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3813;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3888:
  *llvm_cbe_errorcodeptr = ((unsigned int )42);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3881;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3897:
  *llvm_cbe_errorcodeptr = ((unsigned int )49);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3881;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3917:
  *llvm_cbe_errorcodeptr = ((unsigned int )48);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp3881;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true3951_split:
  llvm_cbe_slot_913437_0_us_le62 = &llvm_cbe_tmp3924[llvm_cbe_slot_913437_0_us_rec];
  *llvm_cbe_errorcodeptr = ((unsigned int )43);
  llvm_cbe_tmp519813452 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us_le62;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813452;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4066:
  *llvm_cbe_errorcodeptr = ((unsigned int )42);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4059;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4073:
  *llvm_cbe_errorcodeptr = ((unsigned int )48);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4059;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4154:
  *llvm_cbe_errorcodeptr = ((unsigned int )15);
  llvm_cbe_tmp519813472 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813472;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4238:
  *llvm_cbe_errorcodeptr = ((unsigned int )29);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4233;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4250:
  *llvm_cbe_errorcodeptr = ((unsigned int )58);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4233;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4263:
  *llvm_cbe_errorcodeptr = ((unsigned int )15);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4233;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4276:
  *llvm_cbe_errorcodeptr = ((unsigned int )58);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4233;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4325:
  *llvm_cbe_errorcodeptr = ((unsigned int )15);
  llvm_cbe_tmp519813477 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813477;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4430:
  *llvm_cbe_errorcodeptr = ((unsigned int )40);
  llvm_cbe_tmp519813478 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813478;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_bb4531:
  *llvm_cbe_errorcodeptr = ((unsigned int )12);
  *(&llvm_cbe_ptr106) = llvm_cbe_tmp4537;
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4537;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4746:
  *llvm_cbe_errorcodeptr = ((unsigned int )54);
  llvm_cbe_tmp519813489 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813489;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4754:
  *llvm_cbe_errorcodeptr = ((unsigned int )27);
  llvm_cbe_tmp519813490 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813490;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_true4771:
  *llvm_cbe_errorcodeptr = ((unsigned int )14);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_3;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_3;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp4766;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_bb5071:
  *llvm_cbe_errorcodeptr = ((unsigned int )45);
  llvm_cbe_tmp519813504 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_13;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_13;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp519813504;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_FAILED:
  llvm_cbe_slot_913437_0_us61_12 = llvm_cbe_slot_913437_0_us61_12__PHI_TEMPORARY;
  llvm_cbe_slot_913437_059_12 = llvm_cbe_slot_913437_059_12__PHI_TEMPORARY;
  llvm_cbe_tmp5198 = *(&llvm_cbe_ptr106);
  llvm_cbe_slot_913437_0_us61_7__PHI_TEMPORARY = llvm_cbe_slot_913437_0_us61_12;   /* for PHI node */
  llvm_cbe_slot_913437_059_7__PHI_TEMPORARY = llvm_cbe_slot_913437_059_12;   /* for PHI node */
  llvm_cbe_options_addr_25789_6__PHI_TEMPORARY = llvm_cbe_options_addr_25789_9;   /* for PHI node */
  llvm_cbe_branchreqbyte_5__PHI_TEMPORARY = llvm_cbe_branchreqbyte_413514_1;   /* for PHI node */
  llvm_cbe_branchfirstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_113557_1;   /* for PHI node */
  llvm_cbe_code_6__PHI_TEMPORARY = llvm_cbe_code_2;   /* for PHI node */
  llvm_cbe_ptr_1__PHI_TEMPORARY = llvm_cbe_tmp5198;   /* for PHI node */
  llvm_cbe_tmp_0__PHI_TEMPORARY = 1;   /* for PHI node */
  goto llvm_cbe_bb5201;

llvm_cbe_cond_next5289:
  llvm_cbe_branchreqbyte_1 = llvm_cbe_branchreqbyte_1__PHI_TEMPORARY;
  llvm_cbe_reqbyte_0 = llvm_cbe_reqbyte_0__PHI_TEMPORARY;
  llvm_cbe_firstbyte_0 = llvm_cbe_firstbyte_0__PHI_TEMPORARY;
  if ((llvm_cbe_lookbehind == ((unsigned int )0))) {
    llvm_cbe_branchreqbyte_2__PHI_TEMPORARY = llvm_cbe_branchreqbyte_1;   /* for PHI node */
    llvm_cbe_reqbyte_1__PHI_TEMPORARY = llvm_cbe_reqbyte_0;   /* for PHI node */
    llvm_cbe_firstbyte_1__PHI_TEMPORARY = llvm_cbe_firstbyte_0;   /* for PHI node */
    goto llvm_cbe_cond_next5328;
  } else {
    goto llvm_cbe_cond_true5294;
  }

llvm_cbe_cond_true5227:
  llvm_cbe_tmp5229 = *llvm_cbe_last_branch_413917_1;
  if ((llvm_cbe_tmp5229 == ((unsigned char )83))) {
    goto llvm_cbe_cond_false5236;
  } else {
    llvm_cbe_branchreqbyte_1__PHI_TEMPORARY = llvm_cbe_branchreqbyte_5;   /* for PHI node */
    llvm_cbe_reqbyte_0__PHI_TEMPORARY = llvm_cbe_branchreqbyte_5;   /* for PHI node */
    llvm_cbe_firstbyte_0__PHI_TEMPORARY = llvm_cbe_branchfirstbyte_0;   /* for PHI node */
    goto llvm_cbe_cond_next5289;
  }

llvm_cbe_cond_next5256:
  llvm_cbe_reqbyte_5 = llvm_cbe_reqbyte_5__PHI_TEMPORARY;
  llvm_cbe_firstbyte_4 = llvm_cbe_firstbyte_4__PHI_TEMPORARY;
  llvm_cbe_branchreqbyte_0 = (((((((signed int )llvm_cbe_branchfirstbyte_0) > ((signed int )((unsigned int )-1))) & (((signed int )llvm_cbe_branchreqbyte_5) < ((signed int )((unsigned int )0)))) & (((signed int )llvm_cbe_firstbyte_4) < ((signed int )((unsigned int )0))))) ? (llvm_cbe_branchfirstbyte_0) : (llvm_cbe_branchreqbyte_5));
  if ((((llvm_cbe_branchreqbyte_0 ^ llvm_cbe_reqbyte_5) & ((unsigned int )-513)) == ((unsigned int )0))) {
    goto llvm_cbe_cond_false5284;
  } else {
    llvm_cbe_branchreqbyte_1__PHI_TEMPORARY = llvm_cbe_branchreqbyte_0;   /* for PHI node */
    llvm_cbe_reqbyte_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    llvm_cbe_firstbyte_0__PHI_TEMPORARY = llvm_cbe_firstbyte_4;   /* for PHI node */
    goto llvm_cbe_cond_next5289;
  }

llvm_cbe_cond_false5236:
  if (((llvm_cbe_firstbyte_313911_1 == llvm_cbe_branchfirstbyte_0) | (((signed int )llvm_cbe_firstbyte_313911_1) < ((signed int )((unsigned int )0))))) {
    llvm_cbe_reqbyte_5__PHI_TEMPORARY = llvm_cbe_reqbyte_313907_1;   /* for PHI node */
    llvm_cbe_firstbyte_4__PHI_TEMPORARY = llvm_cbe_firstbyte_313911_1;   /* for PHI node */
    goto llvm_cbe_cond_next5256;
  } else {
    goto llvm_cbe_cond_true5247;
  }

llvm_cbe_cond_true5247:
  if ((((signed int )llvm_cbe_reqbyte_313907_1) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true5252;
  } else {
    llvm_cbe_reqbyte_5__PHI_TEMPORARY = llvm_cbe_reqbyte_313907_1;   /* for PHI node */
    llvm_cbe_firstbyte_4__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_cond_next5256;
  }

llvm_cbe_cond_true5252:
  llvm_cbe_reqbyte_5__PHI_TEMPORARY = llvm_cbe_firstbyte_313911_1;   /* for PHI node */
  llvm_cbe_firstbyte_4__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  goto llvm_cbe_cond_next5256;

llvm_cbe_cond_false5284:
  llvm_cbe_tmp5287 = llvm_cbe_branchreqbyte_0 | llvm_cbe_reqbyte_5;
  llvm_cbe_branchreqbyte_1__PHI_TEMPORARY = llvm_cbe_branchreqbyte_0;   /* for PHI node */
  llvm_cbe_reqbyte_0__PHI_TEMPORARY = llvm_cbe_tmp5287;   /* for PHI node */
  llvm_cbe_firstbyte_0__PHI_TEMPORARY = llvm_cbe_firstbyte_4;   /* for PHI node */
  goto llvm_cbe_cond_next5289;

llvm_cbe_cond_next5316:
  *llvm_cbe_reverse_count_0 = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_tmp5298) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_reverse_count_0[((unsigned int )1)]) = (((unsigned char )llvm_cbe_tmp5298));
  llvm_cbe_branchreqbyte_2__PHI_TEMPORARY = llvm_cbe_branchreqbyte_1;   /* for PHI node */
  llvm_cbe_reqbyte_1__PHI_TEMPORARY = llvm_cbe_reqbyte_0;   /* for PHI node */
  llvm_cbe_firstbyte_1__PHI_TEMPORARY = llvm_cbe_firstbyte_0;   /* for PHI node */
  goto llvm_cbe_cond_next5328;

llvm_cbe_cond_true5294:
  *llvm_cbe_code_6 = ((unsigned char )0);
  llvm_cbe_tmp5298 = find_fixedlength(llvm_cbe_last_branch_413917_1);
  if ((((signed int )llvm_cbe_tmp5298) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true5303;
  } else {
    goto llvm_cbe_cond_next5316;
  }

llvm_cbe_cond_false5459:
  *llvm_cbe_code_6 = ((unsigned char )83);
  *(&llvm_cbe_code_6[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)llvm_cbe_code_6)) - (((unsigned int )(unsigned long)llvm_cbe_last_branch_413917_1)))) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code_6[((unsigned int )2)]) = (((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_code_6)) - (((unsigned char )(unsigned long)llvm_cbe_last_branch_413917_1)))));
  *llvm_cbe_tmp11 = llvm_cbe_code_6;
  llvm_cbe_tmp5484 = &llvm_cbe_code_6[((unsigned int )3)];
  llvm_cbe_code_5__PHI_TEMPORARY = llvm_cbe_tmp5484;   /* for PHI node */
  llvm_cbe_last_branch_3__PHI_TEMPORARY = llvm_cbe_code_6;   /* for PHI node */
  goto llvm_cbe_cond_next5485;

  } while (1); /* end of syntactic loop 'cond_next' */
llvm_cbe_cond_true150:
  *llvm_cbe_errorcodeptr = ((unsigned int )52);
  llvm_cbe_tmp51985596 = *(&llvm_cbe_ptr106);
  llvm_cbe_ptr_15625_2__PHI_TEMPORARY = llvm_cbe_tmp51985596;   /* for PHI node */
  goto llvm_cbe_cond_true5206;

llvm_cbe_cond_true5206:
  llvm_cbe_ptr_15625_2 = llvm_cbe_ptr_15625_2__PHI_TEMPORARY;
  *llvm_cbe_ptrptr = llvm_cbe_ptr_15625_2;
  return ((unsigned int )0);
llvm_cbe_cond_true5303:
  *llvm_cbe_errorcodeptr = ((((llvm_cbe_tmp5298 == ((unsigned int )-2))) ? (((unsigned int )36)) : (((unsigned int )25))));
  *llvm_cbe_ptrptr = llvm_cbe_ptr_1;
  return ((unsigned int )0);
llvm_cbe_cond_true5334:
  if ((llvm_cbe_lengthptr == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_cond_true5339;
  } else {
    goto llvm_cbe_cond_next5376;
  }

llvm_cbe_cond_true5339:
  llvm_cbe_tmp5344 = (((unsigned int )(unsigned long)llvm_cbe_code_6)) - (((unsigned int )(unsigned long)llvm_cbe_last_branch_413917_1));
  llvm_cbe_last_branch_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  llvm_cbe_branch_length_2__PHI_TEMPORARY = llvm_cbe_tmp5344;   /* for PHI node */
  goto llvm_cbe_bb5345;

  do {     /* Syntactic loop 'bb5345' to make GCC happy */
llvm_cbe_bb5345:
  llvm_cbe_last_branch_0_rec = llvm_cbe_last_branch_0_rec__PHI_TEMPORARY;
  llvm_cbe_branch_length_2 = llvm_cbe_branch_length_2__PHI_TEMPORARY;
  llvm_cbe_tmp5347 = &llvm_cbe_last_branch_413917_1[(llvm_cbe_last_branch_0_rec + ((unsigned int )1))];
  llvm_cbe_tmp5348 = *llvm_cbe_tmp5347;
  llvm_cbe_tmp5352 = &llvm_cbe_last_branch_413917_1[(llvm_cbe_last_branch_0_rec + ((unsigned int )2))];
  llvm_cbe_tmp5353 = *llvm_cbe_tmp5352;
  llvm_cbe_tmp5355 = ((((unsigned int )(unsigned char )llvm_cbe_tmp5348)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp5353));
  *llvm_cbe_tmp5347 = (((unsigned char )(((unsigned int )(((unsigned int )llvm_cbe_branch_length_2) >> ((unsigned int )((unsigned int )8)))))));
  *llvm_cbe_tmp5352 = (((unsigned char )llvm_cbe_branch_length_2));
  if ((((signed int )llvm_cbe_tmp5355) > ((signed int )((unsigned int )0)))) {
    llvm_cbe_last_branch_0_rec__PHI_TEMPORARY = (llvm_cbe_last_branch_0_rec - llvm_cbe_tmp5355);   /* for PHI node */
    llvm_cbe_branch_length_2__PHI_TEMPORARY = llvm_cbe_tmp5355;   /* for PHI node */
    goto llvm_cbe_bb5345;
  } else {
    goto llvm_cbe_cond_next5376;
  }

  } while (1); /* end of syntactic loop 'bb5345' */
llvm_cbe_cond_next5376:
  *llvm_cbe_code_6 = ((unsigned char )84);
  *(&llvm_cbe_code_6[((unsigned int )1)]) = (((unsigned char )(((unsigned int )(((unsigned int )((((unsigned int )(unsigned long)llvm_cbe_code_6)) - (((unsigned int )(unsigned long)llvm_cbe_tmp5)))) >> ((unsigned int )((unsigned int )8)))))));
  *(&llvm_cbe_code_6[((unsigned int )2)]) = (((unsigned char )((((unsigned char )(unsigned long)llvm_cbe_code_6)) - (((unsigned char )(unsigned long)llvm_cbe_tmp5)))));
  llvm_cbe_tmp5398 = &llvm_cbe_code_6[((unsigned int )3)];
  if (((llvm_cbe_options_addr_25789_6 & ((unsigned int )7)) == llvm_cbe_oldims)) {
    llvm_cbe_code_4__PHI_TEMPORARY = llvm_cbe_tmp5398;   /* for PHI node */
    goto llvm_cbe_cond_next5421;
  } else {
    goto llvm_cbe_cond_true5405;
  }

llvm_cbe_cond_true5405:
  llvm_cbe_tmp5407 = *llvm_cbe_ptr_1;
  if ((llvm_cbe_tmp5407 == ((unsigned char )41))) {
    goto llvm_cbe_cond_true5411;
  } else {
    llvm_cbe_code_4__PHI_TEMPORARY = llvm_cbe_tmp5398;   /* for PHI node */
    goto llvm_cbe_cond_next5421;
  }

llvm_cbe_cond_true5411:
  *llvm_cbe_tmp5398 = ((unsigned char )24);
  *(&llvm_cbe_code_6[((unsigned int )4)]) = (((unsigned char )llvm_cbe_oldims));
  llvm_cbe_tmp5417 = &llvm_cbe_code_6[((unsigned int )5)];
  llvm_cbe_tmp5418 = *(&llvm_cbe_length);
  *(&llvm_cbe_length) = (llvm_cbe_tmp5418 + ((unsigned int )2));
  llvm_cbe_code_4__PHI_TEMPORARY = llvm_cbe_tmp5417;   /* for PHI node */
  goto llvm_cbe_cond_next5421;

llvm_cbe_cond_next5421:
  llvm_cbe_code_4 = llvm_cbe_code_4__PHI_TEMPORARY;
  *llvm_cbe_tmp24 = llvm_cbe_max_bracount_0;
  *llvm_cbe_codeptr = llvm_cbe_code_4;
  *llvm_cbe_ptrptr = llvm_cbe_ptr_1;
  *llvm_cbe_firstbyteptr = llvm_cbe_firstbyte_1;
  *llvm_cbe_reqbyteptr = llvm_cbe_reqbyte_1;
  if ((llvm_cbe_lengthptr == ((unsigned int *)/*NULL*/0))) {
    goto llvm_cbe_UnifiedReturnBlock;
  } else {
    goto llvm_cbe_cond_true5437;
  }

llvm_cbe_cond_true5437:
  llvm_cbe_tmp5439 = *llvm_cbe_lengthptr;
  llvm_cbe_tmp5440 = *(&llvm_cbe_length);
  *llvm_cbe_lengthptr = (llvm_cbe_tmp5440 + llvm_cbe_tmp5439);
  return ((unsigned int )1);
llvm_cbe_UnifiedReturnBlock:
  return ((unsigned int )1);
}


static unsigned int is_anchored(unsigned char *llvm_cbe_code, unsigned int *llvm_cbe_options, unsigned int llvm_cbe_bracket_map, unsigned int llvm_cbe_backref_map) {
  unsigned int llvm_cbe_code_addr_0_rec;
  unsigned int llvm_cbe_code_addr_0_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp2;
  unsigned char llvm_cbe_tmp5;
  unsigned char *llvm_cbe_tmp10;
  unsigned char llvm_cbe_tmp12;
  unsigned int llvm_cbe_tmp1213;
  unsigned int llvm_cbe_tmp21;
  unsigned char llvm_cbe_tmp34;
  unsigned char llvm_cbe_tmp39;
  unsigned int llvm_cbe_tmp41;
  unsigned int llvm_cbe_tmp59;
  unsigned int llvm_cbe_tmp89;
  unsigned int llvm_cbe_tmp114;
  unsigned char llvm_cbe_tmp125;
  unsigned int llvm_cbe_tmp148;
  unsigned char llvm_cbe_tmp171;
  unsigned char llvm_cbe_tmp176;
  unsigned int llvm_cbe_tmp180_rec;
  unsigned char llvm_cbe_tmp182;
  unsigned int llvm_cbe_retval_0;
  unsigned int llvm_cbe_retval_0__PHI_TEMPORARY;

  llvm_cbe_code_addr_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb;

  do {     /* Syntactic loop 'bb' to make GCC happy */
llvm_cbe_bb:
  llvm_cbe_code_addr_0_rec = llvm_cbe_code_addr_0_rec__PHI_TEMPORARY;
  llvm_cbe_tmp2 = *(&llvm_cbe_code[llvm_cbe_code_addr_0_rec]);
  llvm_cbe_tmp5 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp2))]);
  llvm_cbe_tmp10 =  /*tail*/ first_significant_code((&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + (((unsigned int )(unsigned char )llvm_cbe_tmp5)))]), llvm_cbe_options, ((unsigned int )2), ((unsigned int )0));
  llvm_cbe_tmp12 = *llvm_cbe_tmp10;
  llvm_cbe_tmp1213 = ((unsigned int )(unsigned char )llvm_cbe_tmp12);
  switch (llvm_cbe_tmp12) {
  default:
    goto llvm_cbe_bb96;
;
  case ((unsigned char )93):
    goto llvm_cbe_cond_true;
    break;
  case ((unsigned char )94):
    goto llvm_cbe_cond_true31;
  case ((unsigned char )87):
    goto llvm_cbe_bb84;
  case ((unsigned char )92):
    goto llvm_cbe_bb84;
  case ((unsigned char )95):
    goto llvm_cbe_bb84;
  }
llvm_cbe_cond_next168:
  llvm_cbe_tmp171 = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + ((unsigned int )1))]);
  llvm_cbe_tmp176 = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + ((unsigned int )2))]);
  llvm_cbe_tmp180_rec = llvm_cbe_code_addr_0_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp171)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp176)));
  llvm_cbe_tmp182 = *(&llvm_cbe_code[llvm_cbe_tmp180_rec]);
  if ((llvm_cbe_tmp182 == ((unsigned char )83))) {
    llvm_cbe_code_addr_0_rec__PHI_TEMPORARY = llvm_cbe_tmp180_rec;   /* for PHI node */
    goto llvm_cbe_bb;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_true:
  llvm_cbe_tmp21 =  /*tail*/ is_anchored(llvm_cbe_tmp10, llvm_cbe_options, llvm_cbe_bracket_map, llvm_cbe_backref_map);
  if ((llvm_cbe_tmp21 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next168;
  }

llvm_cbe_cond_true31:
  llvm_cbe_tmp34 = *(&llvm_cbe_tmp10[((unsigned int )3)]);
  llvm_cbe_tmp39 = *(&llvm_cbe_tmp10[((unsigned int )4)]);
  llvm_cbe_tmp41 = ((((unsigned int )(unsigned char )llvm_cbe_tmp34)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp39));
  llvm_cbe_tmp59 =  /*tail*/ is_anchored(llvm_cbe_tmp10, llvm_cbe_options, (((((((signed int )llvm_cbe_tmp41) < ((signed int )((unsigned int )32)))) ? ((((unsigned int )1) << llvm_cbe_tmp41)) : (((unsigned int )1)))) | llvm_cbe_bracket_map), llvm_cbe_backref_map);
  if ((llvm_cbe_tmp59 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next168;
  }

llvm_cbe_bb84:
  llvm_cbe_tmp89 =  /*tail*/ is_anchored(llvm_cbe_tmp10, llvm_cbe_options, llvm_cbe_bracket_map, llvm_cbe_backref_map);
  if ((llvm_cbe_tmp89 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next168;
  }

llvm_cbe_cond_next122:
  llvm_cbe_tmp125 = *(&llvm_cbe_tmp10[((unsigned int )1)]);
  if (((llvm_cbe_tmp125 == ((unsigned char )12)) & ((llvm_cbe_backref_map & llvm_cbe_bracket_map) == ((unsigned int )0)))) {
    goto llvm_cbe_cond_next168;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_next112:
  llvm_cbe_tmp114 = *llvm_cbe_options;
  if (((llvm_cbe_tmp114 & ((unsigned int )4)) == ((unsigned int )0))) {
    goto llvm_cbe_bb140;
  } else {
    goto llvm_cbe_cond_next122;
  }

llvm_cbe_bb96:
  if (((((unsigned int )(llvm_cbe_tmp1213 + ((unsigned int )-56))) < ((unsigned int )((unsigned int )2))) | (llvm_cbe_tmp12 == ((unsigned char )65)))) {
    goto llvm_cbe_cond_next112;
  } else {
    goto llvm_cbe_bb140;
  }

llvm_cbe_bb140:
  if ((((unsigned int )(llvm_cbe_tmp1213 + ((unsigned int )-1))) > ((unsigned int )((unsigned int )1)))) {
    goto llvm_cbe_cond_true146;
  } else {
    goto llvm_cbe_cond_next168;
  }

llvm_cbe_cond_true146:
  llvm_cbe_tmp148 = *llvm_cbe_options;
  if ((((llvm_cbe_tmp148 & ((unsigned int )2)) != ((unsigned int )0)) | (llvm_cbe_tmp12 != ((unsigned char )25)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next168;
  }

  } while (1); /* end of syntactic loop 'bb' */
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
}


static unsigned int is_startline(unsigned char *llvm_cbe_code, unsigned int llvm_cbe_bracket_map, unsigned int llvm_cbe_backref_map) {
  unsigned int llvm_cbe_code_addr_0_us_rec;
  unsigned int llvm_cbe_code_addr_0_us_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp2_us;
  unsigned char llvm_cbe_tmp5_us;
  unsigned char *llvm_cbe_tmp9_us;
  unsigned char llvm_cbe_tmp11_us;
  unsigned char llvm_cbe_tmp137_us;
  unsigned char llvm_cbe_tmp142_us;
  unsigned int llvm_cbe_tmp146_us_rec;
  unsigned char llvm_cbe_tmp148_us;
  unsigned int llvm_cbe_tmp19_us;
  unsigned char llvm_cbe_tmp32_us;
  unsigned char llvm_cbe_tmp37_us;
  unsigned int llvm_cbe_tmp39_us;
  unsigned int llvm_cbe_tmp55_us;
  unsigned int llvm_cbe_tmp85_us;
  unsigned char llvm_cbe_tmp108_us;
  unsigned int llvm_cbe_code_addr_0_rec;
  unsigned int llvm_cbe_code_addr_0_rec__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp2;
  unsigned char llvm_cbe_tmp5;
  unsigned char *llvm_cbe_tmp9;
  unsigned char llvm_cbe_tmp11;
  unsigned int llvm_cbe_tmp19;
  unsigned char llvm_cbe_tmp32;
  unsigned char llvm_cbe_tmp37;
  unsigned int llvm_cbe_tmp39;
  unsigned int llvm_cbe_tmp55;
  unsigned int llvm_cbe_tmp85;
  unsigned char llvm_cbe_tmp137;
  unsigned char llvm_cbe_tmp142;
  unsigned int llvm_cbe_tmp146_rec;
  unsigned char llvm_cbe_tmp148;
  unsigned int llvm_cbe_retval_0;
  unsigned int llvm_cbe_retval_0__PHI_TEMPORARY;

  if (((llvm_cbe_backref_map & llvm_cbe_bracket_map) == ((unsigned int )0))) {
    llvm_cbe_code_addr_0_us_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb_us;
  } else {
    llvm_cbe_code_addr_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_bb;
  }

  do {     /* Syntactic loop 'bb.us' to make GCC happy */
llvm_cbe_bb_us:
  llvm_cbe_code_addr_0_us_rec = llvm_cbe_code_addr_0_us_rec__PHI_TEMPORARY;
  llvm_cbe_tmp2_us = *(&llvm_cbe_code[llvm_cbe_code_addr_0_us_rec]);
  llvm_cbe_tmp5_us = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp2_us))]);
  llvm_cbe_tmp9_us =  /*tail*/ first_significant_code((&llvm_cbe_code[(llvm_cbe_code_addr_0_us_rec + (((unsigned int )(unsigned char )llvm_cbe_tmp5_us)))]), ((unsigned int *)/*NULL*/0), ((unsigned int )0), ((unsigned int )0));
  llvm_cbe_tmp11_us = *llvm_cbe_tmp9_us;
  switch (llvm_cbe_tmp11_us) {
  default:
    goto llvm_cbe_bb92_us;
;
  case ((unsigned char )93):
    goto llvm_cbe_cond_true_us;
  case ((unsigned char )94):
    goto llvm_cbe_cond_true29_us;
  case ((unsigned char )87):
    goto llvm_cbe_bb81_us;
  case ((unsigned char )92):
    goto llvm_cbe_bb81_us;
  case ((unsigned char )95):
    goto llvm_cbe_bb81_us;
  }
llvm_cbe_cond_next134_us:
  llvm_cbe_tmp137_us = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_us_rec + ((unsigned int )1))]);
  llvm_cbe_tmp142_us = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_us_rec + ((unsigned int )2))]);
  llvm_cbe_tmp146_us_rec = llvm_cbe_code_addr_0_us_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp137_us)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp142_us)));
  llvm_cbe_tmp148_us = *(&llvm_cbe_code[llvm_cbe_tmp146_us_rec]);
  if ((llvm_cbe_tmp148_us == ((unsigned char )83))) {
    llvm_cbe_code_addr_0_us_rec__PHI_TEMPORARY = llvm_cbe_tmp146_us_rec;   /* for PHI node */
    goto llvm_cbe_bb_us;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_true_us:
  llvm_cbe_tmp19_us =  /*tail*/ is_startline(llvm_cbe_tmp9_us, llvm_cbe_bracket_map, llvm_cbe_backref_map);
  if ((llvm_cbe_tmp19_us == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next134_us;
  }

llvm_cbe_cond_true29_us:
  llvm_cbe_tmp32_us = *(&llvm_cbe_tmp9_us[((unsigned int )3)]);
  llvm_cbe_tmp37_us = *(&llvm_cbe_tmp9_us[((unsigned int )4)]);
  llvm_cbe_tmp39_us = ((((unsigned int )(unsigned char )llvm_cbe_tmp32_us)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp37_us));
  llvm_cbe_tmp55_us =  /*tail*/ is_startline(llvm_cbe_tmp9_us, (((((((signed int )llvm_cbe_tmp39_us) < ((signed int )((unsigned int )32)))) ? ((((unsigned int )1) << llvm_cbe_tmp39_us)) : (((unsigned int )1)))) | llvm_cbe_bracket_map), llvm_cbe_backref_map);
  if ((llvm_cbe_tmp55_us == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next134_us;
  }

llvm_cbe_bb81_us:
  llvm_cbe_tmp85_us =  /*tail*/ is_startline(llvm_cbe_tmp9_us, llvm_cbe_bracket_map, llvm_cbe_backref_map);
  if ((llvm_cbe_tmp85_us == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next134_us;
  }

llvm_cbe_cond_true105_us:
  llvm_cbe_tmp108_us = *(&llvm_cbe_tmp9_us[((unsigned int )1)]);
  if ((llvm_cbe_tmp108_us == ((unsigned char )12))) {
    goto llvm_cbe_cond_next134_us;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_bb92_us:
  if (((((unsigned int )((((unsigned int )(unsigned char )llvm_cbe_tmp11_us)) + ((unsigned int )-56))) < ((unsigned int )((unsigned int )2))) | (llvm_cbe_tmp11_us == ((unsigned char )65)))) {
    goto llvm_cbe_cond_true105_us;
  } else {
    goto llvm_cbe_cond_false123_us;
  }

llvm_cbe_cond_false123_us:
  if ((llvm_cbe_tmp11_us == ((unsigned char )25))) {
    goto llvm_cbe_cond_next134_us;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  }

  } while (1); /* end of syntactic loop 'bb.us' */
  do {     /* Syntactic loop 'bb' to make GCC happy */
llvm_cbe_bb:
  llvm_cbe_code_addr_0_rec = llvm_cbe_code_addr_0_rec__PHI_TEMPORARY;
  llvm_cbe_tmp2 = *(&llvm_cbe_code[llvm_cbe_code_addr_0_rec]);
  llvm_cbe_tmp5 = *(&_pcre_OP_lengths[(((unsigned int )(unsigned char )llvm_cbe_tmp2))]);
  llvm_cbe_tmp9 =  /*tail*/ first_significant_code((&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + (((unsigned int )(unsigned char )llvm_cbe_tmp5)))]), ((unsigned int *)/*NULL*/0), ((unsigned int )0), ((unsigned int )0));
  llvm_cbe_tmp11 = *llvm_cbe_tmp9;
  switch (llvm_cbe_tmp11) {
  default:
    goto llvm_cbe_bb92;
;
  case ((unsigned char )93):
    goto llvm_cbe_cond_true;
    break;
  case ((unsigned char )94):
    goto llvm_cbe_cond_true29;
  case ((unsigned char )87):
    goto llvm_cbe_bb81;
  case ((unsigned char )92):
    goto llvm_cbe_bb81;
  case ((unsigned char )95):
    goto llvm_cbe_bb81;
  }
llvm_cbe_cond_next134:
  llvm_cbe_tmp137 = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + ((unsigned int )1))]);
  llvm_cbe_tmp142 = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + ((unsigned int )2))]);
  llvm_cbe_tmp146_rec = llvm_cbe_code_addr_0_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp137)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp142)));
  llvm_cbe_tmp148 = *(&llvm_cbe_code[llvm_cbe_tmp146_rec]);
  if ((llvm_cbe_tmp148 == ((unsigned char )83))) {
    llvm_cbe_code_addr_0_rec__PHI_TEMPORARY = llvm_cbe_tmp146_rec;   /* for PHI node */
    goto llvm_cbe_bb;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )1);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_true:
  llvm_cbe_tmp19 =  /*tail*/ is_startline(llvm_cbe_tmp9, llvm_cbe_bracket_map, llvm_cbe_backref_map);
  if ((llvm_cbe_tmp19 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next134;
  }

llvm_cbe_cond_true29:
  llvm_cbe_tmp32 = *(&llvm_cbe_tmp9[((unsigned int )3)]);
  llvm_cbe_tmp37 = *(&llvm_cbe_tmp9[((unsigned int )4)]);
  llvm_cbe_tmp39 = ((((unsigned int )(unsigned char )llvm_cbe_tmp32)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp37));
  llvm_cbe_tmp55 =  /*tail*/ is_startline(llvm_cbe_tmp9, (((((((signed int )llvm_cbe_tmp39) < ((signed int )((unsigned int )32)))) ? ((((unsigned int )1) << llvm_cbe_tmp39)) : (((unsigned int )1)))) | llvm_cbe_bracket_map), llvm_cbe_backref_map);
  if ((llvm_cbe_tmp55 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next134;
  }

llvm_cbe_bb81:
  llvm_cbe_tmp85 =  /*tail*/ is_startline(llvm_cbe_tmp9, llvm_cbe_bracket_map, llvm_cbe_backref_map);
  if ((llvm_cbe_tmp85 == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next134;
  }

llvm_cbe_cond_false123:
  if ((llvm_cbe_tmp11 == ((unsigned char )25))) {
    goto llvm_cbe_cond_next134;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_bb92:
  if (((((unsigned int )((((unsigned int )(unsigned char )llvm_cbe_tmp11)) + ((unsigned int )-56))) < ((unsigned int )((unsigned int )2))) | (llvm_cbe_tmp11 == ((unsigned char )65)))) {
    goto llvm_cbe_cond_true105;
  } else {
    goto llvm_cbe_cond_false123;
  }

  } while (1); /* end of syntactic loop 'bb' */
llvm_cbe_cond_true105:
  return ((unsigned int )0);
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
}


static unsigned int find_firstassertedchar(unsigned char *llvm_cbe_code, unsigned int *llvm_cbe_options, unsigned int llvm_cbe_inassert) {
  unsigned int llvm_cbe_c_1;
  unsigned int llvm_cbe_c_1__PHI_TEMPORARY;
  unsigned int llvm_cbe_code_addr_0_rec;
  unsigned int llvm_cbe_code_addr_0_rec__PHI_TEMPORARY;
  unsigned char *llvm_cbe_tmp5;
  unsigned char llvm_cbe_tmp7;
  unsigned int llvm_cbe_tmp22;
  unsigned char *llvm_cbe_tmp44;
  unsigned char *llvm_cbe_scode_1;
  unsigned char *llvm_cbe_scode_1__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp64;
  unsigned int llvm_cbe_tmp6465;
  unsigned int llvm_cbe_tmp67;
  unsigned int llvm_cbe_tmp73;
  unsigned int llvm_cbe_c_0;
  unsigned int llvm_cbe_c_0__PHI_TEMPORARY;
  unsigned char llvm_cbe_tmp92;
  unsigned char llvm_cbe_tmp97;
  unsigned int llvm_cbe_tmp101_rec;
  unsigned char llvm_cbe_tmp103;
  unsigned int llvm_cbe_retval_0;
  unsigned int llvm_cbe_retval_0__PHI_TEMPORARY;

  llvm_cbe_c_1__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
  llvm_cbe_code_addr_0_rec__PHI_TEMPORARY = ((unsigned int )0);   /* for PHI node */
  goto llvm_cbe_bb;

  do {     /* Syntactic loop 'bb' to make GCC happy */
llvm_cbe_bb:
  llvm_cbe_c_1 = llvm_cbe_c_1__PHI_TEMPORARY;
  llvm_cbe_code_addr_0_rec = llvm_cbe_code_addr_0_rec__PHI_TEMPORARY;
  llvm_cbe_tmp5 =  /*tail*/ first_significant_code((&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + ((unsigned int )3))]), llvm_cbe_options, ((unsigned int )1), ((unsigned int )1));
  llvm_cbe_tmp7 = *llvm_cbe_tmp5;
  switch ((((unsigned int )(unsigned char )llvm_cbe_tmp7))) {
  default:
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
;
  case ((unsigned int )27):
    llvm_cbe_scode_1__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_bb49;
  case ((unsigned int )28):
    llvm_cbe_scode_1__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_bb49;
  case ((unsigned int )32):
    llvm_cbe_scode_1__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_bb49;
  case ((unsigned int )33):
    llvm_cbe_scode_1__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_bb49;
  case ((unsigned int )38):
    goto llvm_cbe_bb42;
  case ((unsigned int )40):
    llvm_cbe_scode_1__PHI_TEMPORARY = llvm_cbe_tmp5;   /* for PHI node */
    goto llvm_cbe_bb49;
  case ((unsigned int )87):
    goto llvm_cbe_bb16;
    break;
  case ((unsigned int )92):
    goto llvm_cbe_bb16;
    break;
  case ((unsigned int )93):
    goto llvm_cbe_bb16;
    break;
  case ((unsigned int )94):
    goto llvm_cbe_bb16;
    break;
  case ((unsigned int )95):
    goto llvm_cbe_bb16;
    break;
  }
llvm_cbe_bb89:
  llvm_cbe_c_0 = llvm_cbe_c_0__PHI_TEMPORARY;
  llvm_cbe_tmp92 = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + ((unsigned int )1))]);
  llvm_cbe_tmp97 = *(&llvm_cbe_code[(llvm_cbe_code_addr_0_rec + ((unsigned int )2))]);
  llvm_cbe_tmp101_rec = llvm_cbe_code_addr_0_rec + (((((unsigned int )(unsigned char )llvm_cbe_tmp92)) << ((unsigned int )8)) | (((unsigned int )(unsigned char )llvm_cbe_tmp97)));
  llvm_cbe_tmp103 = *(&llvm_cbe_code[llvm_cbe_tmp101_rec]);
  if ((llvm_cbe_tmp103 == ((unsigned char )83))) {
    llvm_cbe_c_1__PHI_TEMPORARY = llvm_cbe_c_0;   /* for PHI node */
    llvm_cbe_code_addr_0_rec__PHI_TEMPORARY = llvm_cbe_tmp101_rec;   /* for PHI node */
    goto llvm_cbe_bb;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = llvm_cbe_c_0;   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_next:
  if ((((signed int )llvm_cbe_c_1) < ((signed int )((unsigned int )0)))) {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_tmp22;   /* for PHI node */
    goto llvm_cbe_bb89;
  } else {
    goto llvm_cbe_cond_false;
  }

llvm_cbe_bb16:
  llvm_cbe_tmp22 =  /*tail*/ find_firstassertedchar(llvm_cbe_tmp5, llvm_cbe_options, (((unsigned int )(bool )(llvm_cbe_tmp7 == ((unsigned char )87)))));
  if ((((signed int )llvm_cbe_tmp22) < ((signed int )((unsigned int )0)))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next;
  }

llvm_cbe_cond_false:
  if ((llvm_cbe_c_1 == llvm_cbe_tmp22)) {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_c_1;   /* for PHI node */
    goto llvm_cbe_bb89;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  }

llvm_cbe_cond_true61:
  llvm_cbe_tmp67 = *llvm_cbe_options;
  if (((llvm_cbe_tmp67 & ((unsigned int )1)) == ((unsigned int )0))) {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_tmp6465;   /* for PHI node */
    goto llvm_cbe_bb89;
  } else {
    goto llvm_cbe_cond_true71;
  }

llvm_cbe_cond_next56:
  llvm_cbe_tmp64 = *(&llvm_cbe_scode_1[((unsigned int )1)]);
  llvm_cbe_tmp6465 = ((unsigned int )(unsigned char )llvm_cbe_tmp64);
  if ((((signed int )llvm_cbe_c_1) < ((signed int )((unsigned int )0)))) {
    goto llvm_cbe_cond_true61;
  } else {
    goto llvm_cbe_cond_false75;
  }

llvm_cbe_bb49:
  llvm_cbe_scode_1 = llvm_cbe_scode_1__PHI_TEMPORARY;
  if ((llvm_cbe_inassert == ((unsigned int )0))) {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  } else {
    goto llvm_cbe_cond_next56;
  }

llvm_cbe_bb42:
  llvm_cbe_tmp44 = &llvm_cbe_tmp5[((unsigned int )2)];
  llvm_cbe_scode_1__PHI_TEMPORARY = llvm_cbe_tmp44;   /* for PHI node */
  goto llvm_cbe_bb49;

llvm_cbe_cond_true71:
  llvm_cbe_tmp73 = llvm_cbe_tmp6465 | ((unsigned int )256);
  llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_tmp73;   /* for PHI node */
  goto llvm_cbe_bb89;

llvm_cbe_cond_false75:
  if ((llvm_cbe_tmp6465 == llvm_cbe_c_1)) {
    llvm_cbe_c_0__PHI_TEMPORARY = llvm_cbe_c_1;   /* for PHI node */
    goto llvm_cbe_bb89;
  } else {
    llvm_cbe_retval_0__PHI_TEMPORARY = ((unsigned int )-1);   /* for PHI node */
    goto llvm_cbe_return;
  }

  } while (1); /* end of syntactic loop 'bb' */
llvm_cbe_return:
  llvm_cbe_retval_0 = llvm_cbe_retval_0__PHI_TEMPORARY;
  return llvm_cbe_retval_0;
}

