/*******************************************************************************
* Copyright @ Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.  
* File name: securec.h
* Decription: 
*             the user of this secure c library should include this header file
*             in you source code. This header file declare all supported API
*             prototype of the library, such as memcpy_s, strcpy_s, wcscpy_s,
*             strcat_s, strncat_s, sprintf_s, scanf_s, and so on.
* History:   
*     1. Date: 
*         Author:  
*         Modification: 
********************************************************************************
*/

#ifndef __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27
#define __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27

/* If you need high performance, enable the WITH_PERFORMANCE_ADDONS macro! */
#define WITH_PERFORMANCE_ADDONS

#include "securectype.h"    /*lint !e537*/
#include <stdarg.h>

/* If stack size on some embedded platform is limited, you can define the following macro
*  which will put some variables on heap instead of stack.
#define STACK_SIZE_LESS_THAN_1K
*/

/* for performance consideration, the following macro will call the corresponding API 
* of libC for memcpy, memmove and memset
*/
#define CALL_LIBC_COR_API

/* codes should run under the macro COMPATIBLE_LINUX_FORMAT in unknow system on default, 
   and strtold. The function 
   strtold is referenced first at ISO9899:1999(C99), and some old compilers can
   not support these functions. Here provides a macro to open these functions:
       
       SECUREC_SUPPORT_STRTOLD  -- if defined, strtold will   be used
*/

/*define error code*/
#ifndef errno_t
typedef int errno_t;
#endif

/* success */
#define EOK (0)

/* invalid parameter */
#ifdef EINVAL
#undef EINVAL
#endif
#define EINVAL (22)
#define EINVAL_AND_RESET (22 | 0X80)
/* invalid parameter range */
#ifdef ERANGE
#undef ERANGE  /* to avoid redefinition */
#endif
#define ERANGE (34)
#define ERANGE_AND_RESET  (34 | 0X80)

#ifdef EOVERLAP_AND_RESET
#undef EOVERLAP_AND_RESET
#endif
/*Once the buffer overlap is detected, the dest buffer must be reseted! */
#define EOVERLAP_AND_RESET (54 | 0X80)

/*if you need export the function of this library in Win32 dll, use __declspec(dllexport) */

#ifdef __cplusplus
extern "C"
{
#endif




    /* return SecureC Version */
    void getHwSecureCVersion(char* verStr, int bufSize, unsigned short* verNumber);

    /* wmemcpy */
    errno_t wmemcpy_s(wchar_t* dest, size_t destMax, const wchar_t* src, size_t count);
    
    /* memmove */
    errno_t memmove_s(void* dest, size_t destMax, const void* src, size_t count);

    errno_t wmemmove_s(wchar_t* dest, size_t destMax, const wchar_t* src, size_t count);

    errno_t wcscpy_s(wchar_t* strDest, size_t destMax, const wchar_t* strSrc);

    errno_t wcsncpy_s(wchar_t* strDest, size_t destMax, const wchar_t* strSrc, size_t count);

    errno_t wcscat_s(wchar_t* strDest, size_t destMax, const wchar_t* strSrc);

    errno_t wcsncat_s(wchar_t* strDest, size_t destMax, const wchar_t* strSrc, size_t count);

    /* strtok */
    char* strtok_s(char* strToken, const char* strDelimit, char** context);

    wchar_t* wcstok_s(wchar_t* strToken, const wchar_t* strDelimit, wchar_t** context);

    /* sprintf */
    int sprintf_s(char* strDest, size_t destMax, const char* format, ...) SECUREC_ATTRIBUTE(3,4);

    int swprintf_s(wchar_t* strDest, size_t destMax, const wchar_t* format, ...);

    /* vsprintf */
    int vsprintf_s(char* strDest, size_t destMax, const char* format, va_list argptr) SECUREC_ATTRIBUTE(3,0);

    int vswprintf_s(wchar_t* strDest, size_t destMax, const wchar_t* format, va_list argptr);

    int vsnprintf_s(char* strDest, size_t destMax, size_t count, const char* format, va_list arglist) SECUREC_ATTRIBUTE(4,0);

    /* snprintf */
    int snprintf_s(char* strDest, size_t destMax, size_t count, const char* format, ...) SECUREC_ATTRIBUTE(4,5);

    /* scanf */
    int scanf_s(const char* format, ...);

    int wscanf_s(const wchar_t* format, ...);

    /* vscanf */
    int vscanf_s(const char* format, va_list arglist);

    int vwscanf_s(const wchar_t* format, va_list arglist);

    /* fscanf */
    int fscanf_s(FILE* stream, const char* format, ...);

    int fwscanf_s(FILE* stream, const wchar_t* format, ...);

    /* vfscanf */
    int vfscanf_s(FILE* stream, const char* format, va_list arglist);

    int vfwscanf_s(FILE* stream, const wchar_t* format, va_list arglist);

    /* sscanf */
    int sscanf_s(const char* buffer, const char* format, ...);

    int swscanf_s(const wchar_t* buffer, const wchar_t* format, ...);

    /* vsscanf */
    int vsscanf_s(const char* buffer, const char* format, va_list argptr);

    int vswscanf_s(const wchar_t* buffer, const wchar_t* format, va_list arglist);

    /* gets */
    char* gets_s(char* buffer, size_t destMax);
    
    /* memset function*/
    errno_t memset_s(void* dest, size_t destMax, int c, size_t count);
    /* memcpy function*/
    errno_t memcpy_s(void* dest, size_t destMax, const void* src, size_t count);

    /* strcpy */
    errno_t strcpy_s(char* strDest, size_t destMax, const char* strSrc);
    /* strncpy */
    errno_t strncpy_s(char* strDest, size_t destMax, const char* strSrc, size_t count);

    /* strcat */
    errno_t strcat_s(char* strDest, size_t destMax, const char* strSrc);
    /* strncat */
    errno_t strncat_s(char* strDest, size_t destMax, const char* strSrc, size_t count);

    errno_t strncpy_error(char* strDest, size_t destMax, const char* strSrc, size_t count);
    errno_t strcpy_error(char* strDest, size_t destMax, const char* strSrc);

#if defined(WITH_PERFORMANCE_ADDONS) 
    /* those functions are used by macro */
    errno_t memset_sOptAsm(void* dest, size_t destMax, int c, size_t count);
    errno_t memset_sOptTc(void* dest, size_t destMax, int c, size_t count);
    errno_t memcpy_sOptAsm(void* dest, size_t destMax, const void* src, size_t count);
    errno_t memcpy_sOptTc(void* dest, size_t destMax, const void* src, size_t count);
    
    /* strcpy_sp is a macro, NOT a function in performance optimization mode. */
#define strcpy_sp(dest, destMax, src)  /*lint -save -e506 -e1055  -e650 -e778 -e802 */ (( __builtin_constant_p((destMax)) && __builtin_constant_p((src))) ?  \
    STRCPY_SM((dest), (destMax), (src)) : strcpy_s((dest), (destMax), (src)) ) /*lint -restore */

    /* strncpy_sp is a macro, NOT a function in performance optimization mode. */
#define strncpy_sp(dest, destMax, src, count)  /*lint -save -e506 -e1055 -e666  -e650 -e778 -e802 */ ((__builtin_constant_p((count)) &&  __builtin_constant_p((destMax)) && __builtin_constant_p((src))) ?  \
    STRNCPY_SM((dest), (destMax), (src), (count)) : strncpy_s((dest), (destMax), (src), (count)) ) /*lint -restore */

    /* strcat_sp is a macro, NOT a function in performance optimization mode. */
#define strcat_sp(dest, destMax, src)  /*lint -save -e506 -e1055  -e650 -e778 -e802 */ (( __builtin_constant_p((destMax)) && __builtin_constant_p((src))) ?  \
    STRCAT_SM((dest), (destMax), (src)) : strcat_s((dest), (destMax), (src)) ) /*lint -restore */

    /* strncat_sp is a macro, NOT a function in performance optimization mode. */
#define strncat_sp(dest, destMax, src, count)  /*lint -save -e506 -e1055 -e666  -e650 -e778 -e802 */ ((__builtin_constant_p((count)) &&  __builtin_constant_p((destMax)) && __builtin_constant_p((src))) ?  \
    STRNCAT_SM((dest), (destMax), (src), (count)) : strncat_s((dest), (destMax), (src), (count)) ) /*lint -restore */

    /* memcpy_sp is a macro, NOT a function in performance optimization mode. */
#define memcpy_sp(dest, destMax, src, count)  /*lint -save -e506 -e1055 -e650 -e778 -e802 */ (__builtin_constant_p((count)) ? (MEMCPY_SM((dest), (destMax),  (src), (count))) :  \
       (__builtin_constant_p((destMax)) ? (((size_t)(destMax) > 0 && (((UINT64T)(destMax) & (UINT64T)(-2)) < SECUREC_MEM_MAX_LEN)) ? memcpy_sOptTc((dest), (destMax), (src), (count)) : ERANGE ) :  memcpy_sOptAsm((dest), (destMax), (src), (count)))) /*lint -restore */

    /* memset_sp is a macro, NOT a function in performance optimization mode. */
#define memset_sp(dest, destMax, c, count)  /*lint -save -e506 -e1055 -e650 -e778 -e802 */ (__builtin_constant_p((count)) ? (MEMSET_SM((dest), (destMax),  (c), (count))) :  \
       (__builtin_constant_p((destMax)) ? (((size_t)(destMax) > 0 && (((UINT64T)(destMax) & (UINT64T)(-2)) < SECUREC_MEM_MAX_LEN)) ? memset_sOptTc((dest), (destMax), (c), (count)) : ERANGE ) :  memset_sOptAsm((dest), (destMax), (c), (count)))) /*lint -restore */

#endif


    /************************/
    /*��ȫ�ļ��������������*/
    /************************/
    /**************************************************************************
    * �������ƣ� fscanf_s_op
    * ���������� �Ӱ�ȫ�ļ����ж�ȡ��ʽ��������Ϣ
    * ����˵���� (IN)
    *                pSecStr ��ȫ�ļ���
    *                format ��ʽ���ַ���
    *                ... 
    *            (OUT)
    *             
    * �� �� ֵ�� �ɹ���ȡ��ʽ�����ݵĸ��������Ϊ�����������쳣
    * ����˵���� 
    **************************************************************************/
    int fscanf_s_op(SEC_FILE_STREAM *pfStr, const char* format, ...);

    /**************************************************************************
    * �������ƣ� fwscanf_s_sp
    * ���������� �Ӱ�ȫ�ļ����ж�ȡ��ʽ��������Ϣ��Unicode�ַ���
    * ����˵���� (IN)
    *            pSecStr ��ȫ�ļ���
    *            format ��ʽ���ַ���
    *             ... 
    *            (OUT)
    *             
    * �� �� ֵ�� �ɹ���ȡ��ʽ�����ݵĸ��������Ϊ�����������쳣
    * ����˵���� 
    **************************************************************************/
    int fwscanf_s_op(SEC_FILE_STREAM *pfStr, const char* format, ...);

    /**************************************************************************
    * �������ƣ� sec_fopen_op
    * ���������� �򿪰�ȫ�ļ���
    * ����˵���� (IN)
    *                pfilePath �ļ�����
    *            (OUT)
    *             
    * �� �� ֵ�� ��ȫ�ļ������������ʧ�ܣ�����NULL
    * ����˵���� ��ȫ�ļ���ֻ��Ϊֻ�����޷��������д������
    * ����ʾ����
    ****************************************
    *  int a;
    *  SEC_FILE_STREAM *file = sec_fopen_op("t.txt");
    *  fscanf_s_op(file, "%d", &a);
    *  sec_fclose_op(file);
    *  file = NULL;
    **************************************************************************/
    SEC_FILE_STREAM *sec_fopen_op(char *pfilePath);


    /**************************************************************************
    * �������ƣ� sec_fopen_op_ex
    * ���������� �����ļ���������ȫ�ļ���
    * ����˵���� (IN)
    fp �ļ�����
    *            (OUT)
    *             
    * �� �� ֵ�� ��ȫ�ļ��������������ʧ�ܣ�����NULL
    * ����˵���� 
    * �޸���ʷ��
    1������;
    **************************************************************************/
    SEC_FILE_STREAM *sec_fopen_op_ex(FILE *fp);
	
   
    /**************************************************************************
    * �������ƣ� sec_fclose_op
    * ���������� �ͷŰ�ȫ�ļ�������
    * ����˵���� (IN)
    *                pSecStr ��ȫ�ļ���
    *            (OUT)
    *             
    * �� �� ֵ�� ��
    * ����˵���� 
    **************************************************************************/
    void sec_fclose_op(SEC_FILE_STREAM *pfStr);
	void sec_fclose_op_ex(SEC_FILE_STREAM *pfStr);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif/* __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27 */


