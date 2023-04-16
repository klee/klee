/*******************************************************************************
* Copyright @ Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.  
* File name: securectype.h
* Decription: 
*             define internal used macro and data type. The marco of SECUREC_ON_64BITS
*             will be determined in this header file, which is a switch for part
*             of code. Some macro are used to supress warning by MS compiler.
*Note:        
*             user can change the value of SECUREC_STRING_MAX_LEN and SECUREC_MEM_MAX_LEN
*             macro to meet their special need.
* History:   
********************************************************************************
*/

#ifndef __SECURECTYPE_H__A7BBB686_AADA_451B_B9F9_44DACDAE18A7
#define __SECURECTYPE_H__A7BBB686_AADA_451B_B9F9_44DACDAE18A7

/*Shielding VC symbol redefinition warning*/
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#ifdef __STDC_WANT_SECURE_LIB__
    #undef __STDC_WANT_SECURE_LIB__
#endif
    #define __STDC_WANT_SECURE_LIB__ 0
#ifdef _CRTIMP_ALTERNATIVE
    #undef _CRTIMP_ALTERNATIVE
#endif
    #define _CRTIMP_ALTERNATIVE //comment microsoft *_s function
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* #include <limits.h> this file is used to define some macros, such as  INT_MAX and SIZE_MAX */

/*if enable COMPATIBLE_WIN_FORMAT, the output format will be compatible to Windows.*/
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define COMPATIBLE_WIN_FORMAT
#endif
#if defined(COMPATIBLE_WIN_FORMAT)
/* in windows platform, can't use optimized function for there is no __builtin_constant_p like function */
/* If need optimized macro, can define this: #define __builtin_constant_p(x) 1 */
#ifdef WITH_PERFORMANCE_ADDONS
#undef WITH_PERFORMANCE_ADDONS
#endif
#endif

#if (defined(__VXWORKS__) || defined(__vxworks) || defined(__VXWORKS) || defined(_VXWORKS_PLATFORM_)  || defined(SECUREC_VXWORKS_VERSION_5_4))
#if  !defined(SECUREC_VXWORKS_PLATFORM)
#define SECUREC_VXWORKS_PLATFORM 
#endif
#endif

#ifdef SECUREC_VXWORKS_PLATFORM
#include <version.h>
#endif

/*if enable COMPATIBLE_LINUX_FORMAT, the output format will be compatible to Linux.*/
#if !(defined(COMPATIBLE_WIN_FORMAT) || defined(SECUREC_VXWORKS_PLATFORM))
#define COMPATIBLE_LINUX_FORMAT
#endif
#ifdef COMPATIBLE_LINUX_FORMAT
#include <stddef.h>
#endif

#if defined(__GNUC__)  && !defined(WIN32) && !defined(SECUREC_PCLINT)
#define SECUREC_ATTRIBUTE(x,y)  __attribute__((format(printf, (x), (y) ))) 
#else
#define SECUREC_ATTRIBUTE(x,y)
#endif

#if defined(__GNUC__) && ((__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3 /*above 3.4*/ ))  ) 
    long __builtin_expect(long exp, long c);
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x) 
#endif

#ifndef TWO_MIN
#define TWO_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define WCHAR_SIZE sizeof(wchar_t)

/*ref //sourceforge.net/p/predef/wiki/OperatingSystems/ 
#if !(defined(__hpux) || defined(_AIX) || defined(__VXWORKS__) || defined(__vxworks) ||defined(__ANDROID__) || defined(__WRLINUX__)|| defined(_TYPE_uint8_t))
typedef unsigned char unit8_t;
#endif 
*/ 
typedef signed char INT8T;
typedef unsigned char UINT8T;

#if defined(COMPATIBLE_WIN_FORMAT) || defined(__ARMCC_VERSION)
typedef  __int64 INT64T;
typedef unsigned __int64 UINT64T;
#if defined(__ARMCC_VERSION)
typedef int INT32T;
typedef unsigned int UINT32T;
#else
typedef  __int32 INT32T;
typedef unsigned __int32 UINT32T;
#endif
#else
typedef int INT32T;
typedef unsigned int UINT32T;
typedef long long INT64T;
typedef unsigned long long UINT64T;
#endif

/*安全文件流对象*/
typedef struct
{
    int _cnt;                                   /* the size of buffered string in bytes*/
    char* _ptr;                                 /* the pointer to next read position */
    char* _base;                                /*the pointer to the header of buffered string*/
    int _flag;                                  /* mark the properties of input stream*/
    FILE* pf;                                   /*the file pointer*/
    int fileRealRead;
    long oriFilePos;                            /*the original position of file offset when fscanf is called*/
    unsigned int lastChar;                      /*the char code of last input*/
    int fUnget;                                 /*the boolean flag of pushing a char back to read stream*/
} SEC_FILE_STREAM;

/*安全文件流对象初始化宏*/
#define INIT_SEC_FILE_STREAM {0, NULL, NULL, 0, NULL, 0, 0, 0, 0 }

/* define the max length of the string */
#define SECUREC_STRING_MAX_LEN (0x7fffffffUL)
#define SECUREC_WCHAR_STRING_MAX_LEN (SECUREC_STRING_MAX_LEN / WCHAR_SIZE)

/* add SECUREC_MEM_MAX_LEN for memcpy and memmove*/
#define SECUREC_MEM_MAX_LEN (0x7fffffffUL)
#define SECUREC_WCHAR_MEM_MAX_LEN (SECUREC_MEM_MAX_LEN / WCHAR_SIZE)

#if SECUREC_STRING_MAX_LEN > 0x7fffffff
#error "max string is 2G, or you may remove this macro"
#endif

#if (defined(__GNUC__ ) && defined(__SIZEOF_POINTER__ ))
#if (__SIZEOF_POINTER__ != 4) && (__SIZEOF_POINTER__ != 8)
#error "unsupported system, contact Security Design Technology Department of 2012 Labs"
#endif
#endif

#define IN_REGISTER register

#define SECC_MALLOC(x) malloc((size_t)(x))
#define SECC_FREE(x)   free((void *)(x))

#if defined(_WIN64) || defined(WIN64) || defined(__LP64__) || defined(_LP64)
#define SECUREC_ON_64BITS
#endif

#if (!defined(SECUREC_ON_64BITS) && defined(__GNUC__ ) && defined(__SIZEOF_POINTER__ ))
#if __SIZEOF_POINTER__ == 8
#define SECUREC_ON_64BITS
#endif
#endif

#if defined(__SVR4) || defined(__svr4__)
#define __SOLARIS
#endif

#if (defined(__hpux) || defined(_AIX) || defined(__SOLARIS))
#define __UNIX
#endif

#if ((!defined(SECUREC_SUPPORT_STRTOLD)) && defined(COMPATIBLE_LINUX_FORMAT))
#if  defined(__USE_ISOC99)  \
      || (defined(_AIX) && defined(_ISOC99_SOURCE)) \
      || (defined(__hpux) && defined(__ia64)) \
      || (defined(__SOLARIS) &&  (!defined(_STRICT_STDC) && !defined(__XOPEN_OR_POSIX)) || defined(_STDC_C99) || defined(__EXTENSIONS__))
#define SECUREC_SUPPORT_STRTOLD
#endif
#endif
#if ((defined(SECUREC_WRLINUX_BELOW4) || defined(_WRLINUX_BELOW4_)) && defined(SECUREC_SUPPORT_STRTOLD))
#undef SECUREC_SUPPORT_STRTOLD
#endif

#if defined(WITH_PERFORMANCE_ADDONS) 
    /* for strncpy_s performance optimization */
#define STRNCPY_SM(dest, destMax, src, count) \
    ((NULL != (void*)dest && NULL != (void*)src && (size_t)destMax >0 && (((UINT64T)(destMax) & (UINT64T)(-2)) < SECUREC_STRING_MAX_LEN) && (TWO_MIN(count , strlen(src)) + 1) <= (size_t)destMax ) ?  ( (count < strlen(src))? (memcpy(dest, src, count), *((char*)dest + count) = '\0', EOK) :( memcpy(dest, src, strlen(src) + 1), EOK ) ) :(strncpy_error(dest, destMax, src, count))  )

#define STRCPY_SM(dest, destMax, src) \
    (( NULL != (void*)dest && NULL != (void*)src  && (size_t)destMax >0 && (((UINT64T)(destMax) & (UINT64T)(-2)) < SECUREC_STRING_MAX_LEN) && ( strlen(src) + 1) <= (size_t)destMax )? (memcpy(dest, src, strlen(src) + 1), EOK) :( strcpy_error(dest, destMax, src)))

   /* for strcat_s performance optimization */
#if defined(__GNUC__)
#define STRCAT_SM(dest, destMax, src) \
    ({ int catRet =EOK;\
    if ( NULL != (void*)dest && NULL != (void*)src && (size_t)(destMax) >0 && (((UINT64T)(destMax) & (UINT64T)(-2)) < SECUREC_STRING_MAX_LEN) ) {\
        char* pCatTmpDst = (dest);\
        size_t catRestSz = (destMax);\
        do{\
            while(catRestSz > 0 && *pCatTmpDst) {\
                ++pCatTmpDst;\
                --catRestSz;\
            }\
            if (catRestSz == 0) {\
                catRet = EINVAL;\
                break;\
            }\
            if ( ( strlen(src) + 1) <= catRestSz ) {\
                memcpy(pCatTmpDst, (src), strlen(src) + 1);\
                catRet = EOK;\
            }else{\
                catRet = ERANGE;\
            }\
        }while(0);\
        if ( EOK != catRet) catRet = strcat_s((dest), (destMax), (src));\
    }else{\
        catRet = strcat_s((dest), (destMax), (src));\
    }\
    catRet;})
#else
#define STRCAT_SM(dest, destMax, src) strcat_s(dest, destMax, src)
#endif

    /*for strncat_s performance optimization*/
#if defined(__GNUC__)
#define STRNCAT_SM(dest, destMax, src, count) \
    ({ int ncatRet = EOK;\
    if (NULL != (void*)dest && NULL != (void*)src && (size_t)destMax > 0 && (((UINT64T)(destMax) & (UINT64T)(-2)) < SECUREC_STRING_MAX_LEN)  && (((UINT64T)(count) & (UINT64T)(-2)) < SECUREC_STRING_MAX_LEN)) {\
        char* pCatTmpDest = (dest);\
        size_t ncatRestSz = (destMax);\
        do{\
            while(ncatRestSz > 0 && *pCatTmpDest) {\
                ++pCatTmpDest;\
                --ncatRestSz;\
            }\
            if (ncatRestSz == 0) {\
                ncatRet = EINVAL;\
                break;\
            }\
            if ( (TWO_MIN((count) , strlen(src)) + 1) <= ncatRestSz ) {\
                if ((count) < strlen(src)) {\
                    memcpy(pCatTmpDest, (src), (count));\
                    *(pCatTmpDest + (count)) = '\0';\
                }else {\
                    memcpy(pCatTmpDest, (src), strlen(src) + 1);\
                }\
            }else{\
                ncatRet = ERANGE;\
            }\
        }while(0);\
        if ( EOK != ncatRet) ncatRet = strncat_s((dest), (destMax), (src), (count));\
    }else{\
        ncatRet = strncat_s((dest), (destMax), (src), (count));\
    }\
    ncatRet;})
#else
#define STRNCAT_SM(dest, destMax, src, count) strncat_s(dest, destMax, src, count)
#endif

    /*
    MEMCPY_SM do NOT check buffer overlap by default, or you can add this check to improve security
    condCheck = condCheck || (dest == src) || (dest > src && dest < (void*)((UINT8T*)src + count));\
    condCheck = condCheck || (src > dest && src < (void*)((UINT8T*)dest + count)); \
    */

#define  MEMCPY_SM(dest, destMax, src, count)\
    (!(((size_t)destMax== 0 )||(((UINT64T)(destMax) & (UINT64T)(-2)) > SECUREC_MEM_MAX_LEN)||((size_t)count > (size_t)destMax) || (NULL == (void*)dest) || (NULL == (void*)src))? (memcpy(dest, src, count), EOK) : (memcpy_s(dest, destMax, src, count)))

#define  MEMSET_SM(dest, destMax, c, count)\
    (!(((size_t)destMax == 0 ) || (((UINT64T)(destMax) & (UINT64T)(-2)) > SECUREC_MEM_MAX_LEN) || (NULL == (void*)dest) || ((size_t)count > (size_t)destMax)) ? (memset(dest, c, count), EOK) : ( memset_s(dest, destMax, c, count)))

#endif /* WITH_PERFORMANCE_ADDONS */

/* 20150105 For software and hardware decoupling,such as UMG*/
#ifdef SECUREC_SYSAPI4VXWORKS
#ifdef feof
#undef feof
#endif
extern int feof(FILE *stream);

#ifndef isspace
#define isspace(c)      (((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n'))
#endif
#ifndef isascii
#define isascii(c)       (((unsigned char)(c))<=0x7f)
#endif
#ifndef isupper
#define isupper(c)       ((c) >= 'A' && (c) <= 'Z')
#endif
#ifndef islower
#define islower(c)       ((c) >= 'a' && (c) <= 'z')
#endif
#ifndef isalpha
#define isalpha(c)       (isupper(c) || (islower(c)))
#endif
#ifndef isdigit
#define isdigit(c)       ((c) >= '0' && (c) <= '9')
#endif
#ifndef isxdigit
#define isxupper(c)      ((c) >= 'A' && (c) <= 'F')
#define isxlower(c)      ((c) >= 'a' && (c) <= 'f')
#define isxdigit(c)      (isdigit(c) || isxupper(c) ||isxlower(c))
#endif
#endif

#endif    /*__SECURECTYPE_H__A7BBB686_AADA_451B_B9F9_44DACDAE18A7*/


