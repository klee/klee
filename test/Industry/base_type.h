#ifndef __BASE_TYPE_H_
#define __BASE_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef signed    char          INT8;
typedef unsigned  char          UINT8;
typedef signed    short         INT16;
typedef unsigned  short         UINT16;
typedef signed    int           INT32;
typedef unsigned  int           UINT32;                  
typedef signed    long   long   INT64;
typedef unsigned  long   long   UINT64;
typedef signed    long          LONG;
typedef unsigned  long          ULONG;
typedef           void          VOID;
typedef           float         FLOAT;
typedef           double        DOUBLE;
typedef           long   double LDOUBLE;
#if defined(_WIN64) || defined(WIN64) || defined(__LP64__) || defined(_LP64)
typedef           INT64        SSIZE_T;
typedef           UINT64       SIZE_T;
#else
typedef           INT32        SSIZE_T;
typedef           UINT32       SIZE_T;
#endif

typedef struct TLV{
    UINT32 type;
    UINT32 length;
    UINT8  data[0];
}TLV;

typedef struct LV{
    UINT32 length;
    UINT8  data[0];
}LV;




#ifdef __cplusplus
}
#endif
#endif //__BASE_TYPE_H_