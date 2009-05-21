/*****************************************************************************/
/*  AUTHOR:                                                                  */
/*****************************************************************************/
/*                                                                           */
/*    Steffen Beyer                                                          */
/*    mailto:sb@engelschall.com                                              */
/*    http://www.engelschall.com/u/sb/download/                              */
/*                                                                           */
/*****************************************************************************/
/*  COPYRIGHT:                                                               */
/*****************************************************************************/
/*                                                                           */
/*    Copyright (c) 1995 - 2004 by Steffen Beyer.                            */
/*    All rights reserved.                                                   */
/*                                                                           */
/*****************************************************************************/
/*  LICENSE:                                                                 */
/*****************************************************************************/
/*                                                                           */
/*    This library is free software; you can redistribute it and/or          */
/*    modify it under the terms of the GNU Library General Public            */
/*    License as published by the Free Software Foundation; either           */
/*    version 2 of the License, || (at your option) any later version.       */
/*                                                                           */
/*    This library is distributed in the hope that it will be useful,        */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*    MERCHANTABILITY || FITNESS FOR A PARTICULAR PURPOSE. See the GNU       */
/*    Library General Public License for more details.                       */
/*                                                                           */
/*    You should have received a copy of the GNU Library General Public      */
/*    License along with this library; if not, write to the                  */
/*    Free Software Foundation, Inc.,                                        */
/*    59 Temple Place, Suite 330, Boston, MA 02111-1307 USA                  */
/*                                                                           */
/*    || download a copy from ftp://ftp.gnu.org/pub/gnu/COPYING.LIB-2.0      */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  MODULE NAME:  constantbv.cpp                       MODULE TYPE:constantbv*/
/*****************************************************************************/
/*  MODULE IMPORTS:                                                          */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>                                 /*  MODULE TYPE:  (sys)  */
#include <limits.h>                                 /*  MODULE TYPE:  (sys)  */
#include <string.h>                                 /*  MODULE TYPE:  (sys)  */
#include <ctype.h>                                  /*  MODULE TYPE:  (sys)  */
#include "constantbv.h"

namespace CONSTANTBV {
/*****************************************************************************/
/*  MODULE IMPLEMENTATION:                                                   */
/*****************************************************************************/
    /**********************************************/
    /* global implementation-intrinsic constants: */
    /**********************************************/

#define BIT_VECTOR_HIDDEN_WORDS 3

    /*****************************************************************/
    /* global machine-dependent constants (set by "BitVector_Boot"): */
    /*****************************************************************/

static unsigned int BITS;     /* = # of bits in machine word (must be power of 2)  */
static unsigned int MODMASK;  /* = BITS - 1 (mask for calculating modulo BITS)     */
static unsigned int LOGBITS;  /* = ld(BITS) (logarithmus dualis)                   */
static unsigned int FACTOR;   /* = ld(BITS / 8) (ld of # of bytes)                 */

static unsigned int LSB = 1;  /* = mask for least significant bit                  */
static unsigned int MSB;      /* = mask for most significant bit                   */

static unsigned int LONGBITS; /* = # of bits in unsigned long                      */

static unsigned int LOG10;    /* = logarithm to base 10 of BITS - 1                */
static unsigned int EXP10;    /* = largest possible power of 10 in signed int      */

    /********************************************************************/
    /* global bit mask table for fast access (set by "BitVector_Boot"): */
    /********************************************************************/

static unsigned int *  BITMASKTAB;

    /*****************************/
    /* global macro definitions: */
    /*****************************/

#define BIT_VECTOR_ZERO_WORDS(target,count) \
    while (count-- > 0) *target++ = 0;

#define BIT_VECTOR_FILL_WORDS(target,fill,count) \
    while (count-- > 0) *target++ = fill;

#define BIT_VECTOR_FLIP_WORDS(target,flip,count) \
    while (count-- > 0) *target++ ^= flip;

#define BIT_VECTOR_COPY_WORDS(target,source,count) \
    while (count-- > 0) *target++ = *source++;

#define BIT_VECTOR_BACK_WORDS(target,source,count) \
    { target += count; source += count; while (count-- > 0) *--target = *--source; }

#define BIT_VECTOR_CLR_BIT(address,index) \
    *(address+(index>>LOGBITS)) &= ~ BITMASKTAB[index & MODMASK];

#define BIT_VECTOR_SET_BIT(address,index) \
    *(address+(index>>LOGBITS)) |= BITMASKTAB[index & MODMASK];

#define BIT_VECTOR_TST_BIT(address,index) \
    ((*(address+(index>>LOGBITS)) & BITMASKTAB[index & MODMASK]) != 0)

#define BIT_VECTOR_FLP_BIT(address,index,mask) \
    (mask = BITMASKTAB[index & MODMASK]), \
    (((*(addr+(index>>LOGBITS)) ^= mask) & mask) != 0)

#define BIT_VECTOR_DIGITIZE(type,value,digit) \
    value = (type) ((digit = value) / 10); \
    digit -= value * 10; \
    digit += (type) '0';

    /*********************************************************/
    /* private low-level functions (potentially dangerous!): */
    /*********************************************************/

static unsigned int power10(unsigned int x) {
    unsigned int y = 1;

    while (x-- > 0) y *= 10;
    return(y);
}

static void BIT_VECTOR_zro_words(unsigned int *  addr, unsigned int count) {
    BIT_VECTOR_ZERO_WORDS(addr,count)
}

static void BIT_VECTOR_cpy_words(unsigned int *  target, 
				 unsigned int *  source, unsigned int count) {
    BIT_VECTOR_COPY_WORDS(target,source,count)
}

static void BIT_VECTOR_mov_words(unsigned int *  target, 
				 unsigned int *  source, unsigned int count) {
    if (target != source) {
        if (target < source) BIT_VECTOR_COPY_WORDS(target,source,count)
        else                 BIT_VECTOR_BACK_WORDS(target,source,count)
    }
}

static void BIT_VECTOR_ins_words(unsigned int *  addr, 
				 unsigned int total, unsigned int count, boolean clear) {
    unsigned int length;

    if ((total > 0) && (count > 0)) {
        if (count > total) count = total;
        length = total - count;
        if (length > 0) BIT_VECTOR_mov_words(addr+count,addr,length);
        if (clear)      BIT_VECTOR_zro_words(addr,count);
    }
}

static void BIT_VECTOR_del_words(unsigned int *  addr, 
				 unsigned int total, unsigned int count, boolean clear) {
    unsigned int length;

    if ((total > 0) && (count > 0)) {
        if (count > total) count = total;
        length = total - count;
        if (length > 0) BIT_VECTOR_mov_words(addr,addr+count,length);
        if (clear)      BIT_VECTOR_zro_words(addr+length,count);
    }
}

static void BIT_VECTOR_reverse(unsigned char * string, unsigned int length) {
    unsigned char * last;
    unsigned char  temp;

    if (length > 1) {
        last = string + length - 1;
        while (string < last) {
            temp = *string;
            *string = *last;
            *last = temp;
            string++;
            last--;
        }
    }
}

static unsigned int BIT_VECTOR_int2str(unsigned char * string, unsigned int value) {
    unsigned int  length;
    unsigned int  digit;
    unsigned char * work;

    work = string;
    if (value > 0) {
        length = 0;
        while (value > 0) {
            BIT_VECTOR_DIGITIZE(unsigned int,value,digit)
            *work++ = (unsigned char) digit;
            length++;
        }
        BIT_VECTOR_reverse(string,length);
    }
    else {
        length = 1;
        *work++ = (unsigned char) '0';
    }
    return(length);
}

static unsigned int BIT_VECTOR_str2int(unsigned char * string, unsigned int *value) {
    unsigned int  length;
    unsigned int  digit;

    *value = 0;
    length = 0;
    digit = (unsigned int) *string++;
    /* separate because isdigit() is likely a macro! */
    while (isdigit((int)digit) != 0) {
        length++;
        digit -= (unsigned int) '0';
        if (*value) *value *= 10;
        *value += digit;
        digit = (unsigned int) *string++;
    }
    return(length);
}

    /********************************************/
    /* routine to convert error code to string: */
    /********************************************/

unsigned char * BitVector_Error(ErrCode error) {
    switch (error) {
        case ErrCode_Ok:   return( (unsigned char *)     NULL     ); break;
        case ErrCode_Type: return( (unsigned char *) ERRCODE_TYPE ); break;
        case ErrCode_Bits: return( (unsigned char *) ERRCODE_BITS ); break;
        case ErrCode_Word: return( (unsigned char *) ERRCODE_WORD ); break;
        case ErrCode_Long: return( (unsigned char *) ERRCODE_LONG ); break;
        case ErrCode_Powr: return( (unsigned char *) ERRCODE_POWR ); break;
        case ErrCode_Loga: return( (unsigned char *) ERRCODE_LOGA ); break;
        case ErrCode_Null: return( (unsigned char *) ERRCODE_NULL ); break;
        case ErrCode_Indx: return( (unsigned char *) ERRCODE_INDX ); break;
        case ErrCode_Ordr: return( (unsigned char *) ERRCODE_ORDR ); break;
        case ErrCode_Size: return( (unsigned char *) ERRCODE_SIZE ); break;
        case ErrCode_Pars: return( (unsigned char *) ERRCODE_PARS ); break;
        case ErrCode_Ovfl: return( (unsigned char *) ERRCODE_OVFL ); break;
        case ErrCode_Same: return( (unsigned char *) ERRCODE_SAME ); break;
        case ErrCode_Expo: return( (unsigned char *) ERRCODE_EXPO ); break;
        case ErrCode_Zero: return( (unsigned char *) ERRCODE_ZERO ); break;
        default:           return( (unsigned char *) ERRCODE_OOPS ); break;
    }
}

    /*****************************************/
    /* automatic self-configuration routine: */
    /*****************************************/

    /*******************************************************/
    /*                                                     */
    /*   MUST be called once prior to any other function   */
    /*   to initialize the machine dependent constants     */
    /*   of this package! (But call only ONCE, or you      */
    /*   will suffer memory leaks!)                        */
    /*                                                     */
    /*******************************************************/

ErrCode BitVector_Boot(void) {
    unsigned long longsample = 1L;
    unsigned int sample = LSB;
    unsigned int lsb;

    if (sizeof(unsigned int) > sizeof(size_t)) return(ErrCode_Type);

    BITS = 1;
    while (sample <<= 1) BITS++;    /* determine # of bits in a machine word */

    if (BITS != (sizeof(unsigned int) << 3)) return(ErrCode_Bits);

    if (BITS < 16) return(ErrCode_Word);

    LONGBITS = 1;
    while (longsample <<= 1) LONGBITS++;  /* = # of bits in an unsigned long */

    if (BITS > LONGBITS) return(ErrCode_Long);

    LOGBITS = 0;
    sample = BITS;
    lsb = (sample & LSB);
    while ((sample >>= 1) && (! lsb)) {
        LOGBITS++;
        lsb = (sample & LSB);
    }

    if (sample) return(ErrCode_Powr);      /* # of bits is not a power of 2! */

    if (BITS != (LSB << LOGBITS)) return(ErrCode_Loga);

    MODMASK = BITS - 1;
    FACTOR = LOGBITS - 3;  /* ld(BITS / 8) = ld(BITS) - ld(8) = ld(BITS) - 3 */
    MSB = (LSB << MODMASK);

    BITMASKTAB = (unsigned int * ) malloc((size_t) (BITS << FACTOR));

    if (BITMASKTAB == NULL) return(ErrCode_Null);

    for ( sample = 0; sample < BITS; sample++ ) {
        BITMASKTAB[sample] = (LSB << sample);
    }

    LOG10 = (unsigned int) (MODMASK * 0.30103); /* = (BITS - 1) * ( ln 2 / ln 10 ) */
    EXP10 = power10(LOG10);

    return(ErrCode_Ok);
}

unsigned int BitVector_Size(unsigned int bits) {          /* bit vector size (# of words)  */
    unsigned int size;

    size = bits >> LOGBITS;
    if (bits & MODMASK) size++;
    return(size);
}

unsigned int BitVector_Mask(unsigned int bits)           /* bit vector mask (unused bits) */
{
    unsigned int mask;

    mask = bits & MODMASK;
    if (mask) mask = (unsigned int) ~(~0L << mask); else mask = (unsigned int) ~0L;
    return(mask);
}

unsigned char * BitVector_Version(void)
{
    return((unsigned char *)"6.4");
}

unsigned int BitVector_Word_Bits(void)
{
    return(BITS);
}

unsigned int BitVector_Long_Bits(void)
{
    return(LONGBITS);
}

/********************************************************************/
/*                                                                  */
/*  WARNING: Do not "free()" constant character strings, i.e.,      */
/*           don't call "BitVector_Dispose()" for strings returned  */
/*           by "BitVector_Error()" or "BitVector_Version()"!       */
/*                                                                  */
/*  ONLY call this function for strings allocated with "malloc()",  */
/*  i.e., the strings returned by the functions "BitVector_to_*()"  */
/*  and "BitVector_Block_Read()"!                                   */
/*                                                                  */
/********************************************************************/

void BitVector_Dispose(unsigned char * string)                      /* free string   */
{
    if (string != NULL) free((void *) string);
}

void BitVector_Destroy(unsigned int *  addr)                        /* free bitvec   */
{
    if (addr != NULL)
    {
        addr -= BIT_VECTOR_HIDDEN_WORDS;
        free((void *) addr);
    }
}

void BitVector_Destroy_List(unsigned int *  *  list, unsigned int count)      /* free list     */
{
    unsigned int *  *  slot;

    if (list != NULL)
    {
        slot = list;
        while (count-- > 0)
        {
            BitVector_Destroy(*slot++);
        }
        free((void *) list);
    }
}

unsigned int *  BitVector_Create(unsigned int bits, boolean clear)         /* malloc        */
{
    unsigned int  size;
    unsigned int  mask;
    unsigned int  bytes;
    unsigned int *  addr;
    unsigned int *  zero;

    size = BitVector_Size(bits);
    mask = BitVector_Mask(bits);
    bytes = (size + BIT_VECTOR_HIDDEN_WORDS) << FACTOR;
    addr = (unsigned int * ) malloc((size_t) bytes);
    if (addr != NULL)
    {
        *addr++ = bits;
        *addr++ = size;
        *addr++ = mask;
        if (clear)
        {
            zero = addr;
            BIT_VECTOR_ZERO_WORDS(zero,size)
        }
    }
    return(addr);
}

unsigned int *  *  BitVector_Create_List(unsigned int bits, boolean clear, unsigned int count)
{
    unsigned int *  *  list = NULL;
    unsigned int *  *  slot;
    unsigned int *  addr;
    unsigned int   i;

    if (count > 0)
    {
        list = (unsigned int *  * ) malloc(sizeof(unsigned int * ) * count);
        if (list != NULL)
        {
            slot = list;
            for ( i = 0; i < count; i++ )
            {
                addr = BitVector_Create(bits,clear);
                if (addr == NULL)
                {
                    BitVector_Destroy_List(list,i);
                    return(NULL);
                }
                *slot++ = addr;
            }
        }
    }
    return(list);
}

unsigned int *  BitVector_Resize(unsigned int *  oldaddr, unsigned int bits)       /* realloc       */
{
    unsigned int  bytes;
    unsigned int  oldsize;
    unsigned int  oldmask;
    unsigned int  newsize;
    unsigned int  newmask;
    unsigned int *  newaddr;
    unsigned int *  source;
    unsigned int *  target;

    oldsize = size_(oldaddr);
    oldmask = mask_(oldaddr);
    newsize = BitVector_Size(bits);
    newmask = BitVector_Mask(bits);
    if (oldsize > 0) *(oldaddr+oldsize-1) &= oldmask;
    if (newsize <= oldsize)
    {
        newaddr = oldaddr;
        bits_(newaddr) = bits;
        size_(newaddr) = newsize;
        mask_(newaddr) = newmask;
        if (newsize > 0) *(newaddr+newsize-1) &= newmask;
    }
    else
    {
        bytes = (newsize + BIT_VECTOR_HIDDEN_WORDS) << FACTOR;
        newaddr = (unsigned int * ) malloc((size_t) bytes);
        if (newaddr != NULL)
        {
            *newaddr++ = bits;
            *newaddr++ = newsize;
            *newaddr++ = newmask;
            target = newaddr;
            source = oldaddr;
            newsize -= oldsize;
            BIT_VECTOR_COPY_WORDS(target,source,oldsize)
            BIT_VECTOR_ZERO_WORDS(target,newsize)
        }
        BitVector_Destroy(oldaddr);
    }
    return(newaddr);
}

unsigned int *  BitVector_Shadow(unsigned int *  addr)     /* makes new, same size but empty */
{
    return( BitVector_Create(bits_(addr),true) );
}

unsigned int *  BitVector_Clone(unsigned int *  addr)               /* makes exact duplicate */
{
    unsigned int  bits;
    unsigned int *  twin;

    bits = bits_(addr);
    twin = BitVector_Create(bits,false);
    if ((twin != NULL) && (bits > 0))
        BIT_VECTOR_cpy_words(twin,addr,size_(addr));
    return(twin);
}

unsigned int *  BitVector_Concat(unsigned int *  X, unsigned int *  Y)      /* returns concatenation */
{
    /* BEWARE that X = most significant part, Y = least significant part! */

    unsigned int  bitsX;
    unsigned int  bitsY;
    unsigned int  bitsZ;
    unsigned int *  Z;

    bitsX = bits_(X);
    bitsY = bits_(Y);
    bitsZ = bitsX + bitsY;
    Z = BitVector_Create(bitsZ,false);
    if ((Z != NULL) && (bitsZ > 0))
    {
        BIT_VECTOR_cpy_words(Z,Y,size_(Y));
        BitVector_Interval_Copy(Z,X,bitsY,0,bitsX);
        *(Z+size_(Z)-1) &= mask_(Z);
    }
    return(Z);
}

void BitVector_Copy(unsigned int *  X, unsigned int *  Y)                           /* X = Y */
{
    unsigned int  sizeX = size_(X);
    unsigned int  sizeY = size_(Y);
    unsigned int  maskX = mask_(X);
    unsigned int  maskY = mask_(Y);
    unsigned int  fill  = 0;
    unsigned int *  lastX;
    unsigned int *  lastY;

    if ((X != Y) && (sizeX > 0))
    {
        lastX = X + sizeX - 1;
        if (sizeY > 0)
        {
            lastY = Y + sizeY - 1;
            if ( (*lastY & (maskY & ~ (maskY >> 1))) == 0 ) *lastY &= maskY;
            else
            {
                fill = (unsigned int) ~0L;
                *lastY |= ~ maskY;
            }
            while ((sizeX > 0) && (sizeY > 0))
            {
                *X++ = *Y++;
                sizeX--;
                sizeY--;
            }
            *lastY &= maskY;
        }
        while (sizeX-- > 0) *X++ = fill;
        *lastX &= maskX;
    }
}

void BitVector_Empty(unsigned int *  addr)                        /* X = {}  clr all */
{
    unsigned int size = size_(addr);

    BIT_VECTOR_ZERO_WORDS(addr,size)
}

void BitVector_Fill(unsigned int *  addr)                         /* X = ~{} set all */
{
    unsigned int size = size_(addr);
    unsigned int mask = mask_(addr);
    unsigned int fill = (unsigned int) ~0L;

    if (size > 0)
    {
        BIT_VECTOR_FILL_WORDS(addr,fill,size)
        *(--addr) &= mask;
    }
}

void BitVector_Flip(unsigned int *  addr)                         /* X = ~X flip all */
{
    unsigned int size = size_(addr);
    unsigned int mask = mask_(addr);
    unsigned int flip = (unsigned int) ~0L;

    if (size > 0)
    {
        BIT_VECTOR_FLIP_WORDS(addr,flip,size)
        *(--addr) &= mask;
    }
}

void BitVector_Primes(unsigned int *  addr)
{
    unsigned int  bits = bits_(addr);
    unsigned int  size = size_(addr);
    unsigned int *  work;
    unsigned int  temp;
    unsigned int  i,j;

    if (size > 0)
    {
        temp = 0xAAAA;
        i = BITS >> 4;
        while (--i > 0)
        {
            temp <<= 16;
            temp |= 0xAAAA;
        }
        i = size;
        work = addr;
        *work++ = temp ^ 0x0006;
        while (--i > 0) *work++ = temp;
        for ( i = 3; (j = i * i) < bits; i += 2 )
        {
            for ( ; j < bits; j += i ) BIT_VECTOR_CLR_BIT(addr,j)
        }
        *(addr+size-1) &= mask_(addr);
    }
}

void BitVector_Reverse(unsigned int *  X, unsigned int *  Y)
{
    unsigned int bits = bits_(X);
    unsigned int mask;
    unsigned int bit;
    unsigned int value;

    if (bits > 0)
    {
        if (X == Y) BitVector_Interval_Reverse(X,0,bits-1);
        else if (bits == bits_(Y))
        {
/*          mask = mask_(Y);  */
/*          mask &= ~ (mask >> 1);  */
            mask = BITMASKTAB[(bits-1) & MODMASK];
            Y += size_(Y) - 1;
            value = 0;
            bit = LSB;
            while (bits-- > 0)
            {
                if ((*Y & mask) != 0)
                {
                    value |= bit;
                }
                if (! (mask >>= 1))
                {
                    Y--;
                    mask = MSB;
                }
                if (! (bit <<= 1))
                {
                    *X++ = value;
                    value = 0;
                    bit = LSB;
                }
            }
            if (bit > LSB) *X = value;
        }
    }
}

void BitVector_Interval_Empty(unsigned int *  addr, unsigned int lower, unsigned int upper)
{                                                  /* X = X \ [lower..upper] */
    unsigned int  bits = bits_(addr);
    unsigned int  size = size_(addr);
    unsigned int *  loaddr;
    unsigned int *  hiaddr;
    unsigned int  lobase;
    unsigned int  hibase;
    unsigned int  lomask;
    unsigned int  himask;
    unsigned int  diff;

    if ((size > 0) && (lower < bits) && (upper < bits) && (lower <= upper))
    {
        lobase = lower >> LOGBITS;
        hibase = upper >> LOGBITS;
        diff = hibase - lobase;
        loaddr = addr + lobase;
        hiaddr = addr + hibase;

        lomask = (unsigned int)   (~0L << (lower & MODMASK));
        himask = (unsigned int) ~((~0L << (upper & MODMASK)) << 1);

        if (diff == 0)
        {
            *loaddr &= ~ (lomask & himask);
        }
        else
        {
            *loaddr++ &= ~ lomask;
            while (--diff > 0)
            {
                *loaddr++ = 0;
            }
            *hiaddr &= ~ himask;
        }
    }
}

void BitVector_Interval_Fill(unsigned int *  addr, unsigned int lower, unsigned int upper)
{                                                  /* X = X + [lower..upper] */
    unsigned int  bits = bits_(addr);
    unsigned int  size = size_(addr);
    unsigned int  fill = (unsigned int) ~0L;
    unsigned int *  loaddr;
    unsigned int *  hiaddr;
    unsigned int  lobase;
    unsigned int  hibase;
    unsigned int  lomask;
    unsigned int  himask;
    unsigned int  diff;

    if ((size > 0) && (lower < bits) && (upper < bits) && (lower <= upper))
    {
        lobase = lower >> LOGBITS;
        hibase = upper >> LOGBITS;
        diff = hibase - lobase;
        loaddr = addr + lobase;
        hiaddr = addr + hibase;

        lomask = (unsigned int)   (~0L << (lower & MODMASK));
        himask = (unsigned int) ~((~0L << (upper & MODMASK)) << 1);

        if (diff == 0)
        {
            *loaddr |= (lomask & himask);
        }
        else
        {
            *loaddr++ |= lomask;
            while (--diff > 0)
            {
                *loaddr++ = fill;
            }
            *hiaddr |= himask;
        }
        *(addr+size-1) &= mask_(addr);
    }
}

void BitVector_Interval_Flip(unsigned int *  addr, unsigned int lower, unsigned int upper)
{                                                  /* X = X ^ [lower..upper] */
    unsigned int  bits = bits_(addr);
    unsigned int  size = size_(addr);
    unsigned int  flip = (unsigned int) ~0L;
    unsigned int *  loaddr;
    unsigned int *  hiaddr;
    unsigned int  lobase;
    unsigned int  hibase;
    unsigned int  lomask;
    unsigned int  himask;
    unsigned int  diff;

    if ((size > 0) && (lower < bits) && (upper < bits) && (lower <= upper))
    {
        lobase = lower >> LOGBITS;
        hibase = upper >> LOGBITS;
        diff = hibase - lobase;
        loaddr = addr + lobase;
        hiaddr = addr + hibase;

        lomask = (unsigned int)   (~0L << (lower & MODMASK));
        himask = (unsigned int) ~((~0L << (upper & MODMASK)) << 1);

        if (diff == 0)
        {
            *loaddr ^= (lomask & himask);
        }
        else
        {
            *loaddr++ ^= lomask;
            while (--diff > 0)
            {
                *loaddr++ ^= flip;
            }
            *hiaddr ^= himask;
        }
        *(addr+size-1) &= mask_(addr);
    }
}

void BitVector_Interval_Reverse(unsigned int *  addr, unsigned int lower, unsigned int upper)
{
    unsigned int  bits = bits_(addr);
    unsigned int *  loaddr;
    unsigned int *  hiaddr;
    unsigned int  lomask;
    unsigned int  himask;

    if ((bits > 0) && (lower < bits) && (upper < bits) && (lower < upper))
    {
        loaddr = addr + (lower >> LOGBITS);
        hiaddr = addr + (upper >> LOGBITS);
        lomask = BITMASKTAB[lower & MODMASK];
        himask = BITMASKTAB[upper & MODMASK];
        for ( bits = upper - lower + 1; bits > 1; bits -= 2 )
        {
            if (((*loaddr & lomask) != 0) ^ ((*hiaddr & himask) != 0))
            {
                *loaddr ^= lomask;  /* swap bits only if they differ! */
                *hiaddr ^= himask;
            }
            if (! (lomask <<= 1))
            {
                lomask = LSB;
                loaddr++;
            }
            if (! (himask >>= 1))
            {
                himask = MSB;
                hiaddr--;
            }
        }
    }
}

boolean BitVector_interval_scan_inc(unsigned int *  addr, unsigned int start,
                                    unsigned int * min, unsigned int * max)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int  offset;
    unsigned int  bitmask;
    unsigned int  value;
    boolean empty;

    if ((size == 0) || (start >= bits_(addr))) return(false);

    *min = start;
    *max = start;

    offset = start >> LOGBITS;

    *(addr+size-1) &= mask;

    addr += offset;
    size -= offset;

    bitmask = BITMASKTAB[start & MODMASK];
    mask = ~ (bitmask | (bitmask - 1));

    value = *addr++;
    if ((value & bitmask) == 0)
    {
        value &= mask;
        if (value == 0)
        {
            offset++;
            empty = true;
            while (empty && (--size > 0))
            {
                if ((value = *addr++)) empty = false; else offset++;
            }
            if (empty) return(false);
        }
        start = offset << LOGBITS;
        bitmask = LSB;
        mask = value;
        while (! (mask & LSB))
        {
            bitmask <<= 1;
            mask >>= 1;
            start++;
        }
        mask = ~ (bitmask | (bitmask - 1));
        *min = start;
        *max = start;
    }
    value = ~ value;
    value &= mask;
    if (value == 0)
    {
        offset++;
        empty = true;
        while (empty && (--size > 0))
        {
            if ((value = ~ *addr++)) empty = false; else offset++;
        }
        if (empty) value = LSB;
    }
    start = offset << LOGBITS;
    while (! (value & LSB))
    {
        value >>= 1;
        start++;
    }
    *max = --start;
    return(true);
}

boolean BitVector_interval_scan_dec(unsigned int *  addr, unsigned int start,
                                    unsigned int * min, unsigned int * max)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int offset;
    unsigned int bitmask;
    unsigned int value;
    boolean empty;

    if ((size == 0) || (start >= bits_(addr))) return(false);

    *min = start;
    *max = start;

    offset = start >> LOGBITS;

    if (offset >= size) return(false);

    *(addr+size-1) &= mask;

    addr += offset;
    size = ++offset;

    bitmask = BITMASKTAB[start & MODMASK];
    mask = (bitmask - 1);

    value = *addr--;
    if ((value & bitmask) == 0)
    {
        value &= mask;
        if (value == 0)
        {
            offset--;
            empty = true;
            while (empty && (--size > 0))
            {
                if ((value = *addr--)) empty = false; else offset--;
            }
            if (empty) return(false);
        }
        start = offset << LOGBITS;
        bitmask = MSB;
        mask = value;
        while (! (mask & MSB))
        {
            bitmask >>= 1;
            mask <<= 1;
            start--;
        }
        mask = (bitmask - 1);
        *max = --start;
        *min = start;
    }
    value = ~ value;
    value &= mask;
    if (value == 0)
    {
        offset--;
        empty = true;
        while (empty && (--size > 0))
        {
            if ((value = ~ *addr--)) empty = false; else offset--;
        }
        if (empty) value = MSB;
    }
    start = offset << LOGBITS;
    while (! (value & MSB))
    {
        value <<= 1;
        start--;
    }
    *min = start;
    return(true);
}

void BitVector_Interval_Copy(unsigned int *  X, unsigned int *  Y, unsigned int Xoffset,
                             unsigned int Yoffset, unsigned int length)
{
    unsigned int  bitsX = bits_(X);
    unsigned int  bitsY = bits_(Y);
    unsigned int  source = 0;        /* silence compiler warning */
    unsigned int  target = 0;        /* silence compiler warning */
    unsigned int  s_lo_base;
    unsigned int  s_hi_base;
    unsigned int  s_lo_bit;
    unsigned int  s_hi_bit;
    unsigned int  s_base;
    unsigned int  s_lower = 0;       /* silence compiler warning */
    unsigned int  s_upper = 0;       /* silence compiler warning */
    unsigned int  s_bits;
    unsigned int  s_min;
    unsigned int  s_max;
    unsigned int  t_lo_base;
    unsigned int  t_hi_base;
    unsigned int  t_lo_bit;
    unsigned int  t_hi_bit;
    unsigned int  t_base;
    unsigned int  t_lower = 0;       /* silence compiler warning */
    unsigned int  t_upper = 0;       /* silence compiler warning */
    unsigned int  t_bits;
    unsigned int  t_min;
    unsigned int  mask;
    unsigned int  bits;
    unsigned int  select;
    boolean ascending;
    boolean notfirst;
    unsigned int *  Z = X;

    if ((length > 0) && (Xoffset < bitsX) && (Yoffset < bitsY))
    {
        if ((Xoffset + length) > bitsX) length = bitsX - Xoffset;
        if ((Yoffset + length) > bitsY) length = bitsY - Yoffset;

        ascending = (Xoffset <= Yoffset);

        s_lo_base = Yoffset >> LOGBITS;
        s_lo_bit = Yoffset & MODMASK;
        Yoffset += --length;
        s_hi_base = Yoffset >> LOGBITS;
        s_hi_bit = Yoffset & MODMASK;

        t_lo_base = Xoffset >> LOGBITS;
        t_lo_bit = Xoffset & MODMASK;
        Xoffset += length;
        t_hi_base = Xoffset >> LOGBITS;
        t_hi_bit = Xoffset & MODMASK;

        if (ascending)
        {
            s_base = s_lo_base;
            t_base = t_lo_base;
        }
        else
        {
            s_base = s_hi_base;
            t_base = t_hi_base;
        }
        s_bits = 0;
        t_bits = 0;
        Y += s_base;
        X += t_base;
        notfirst = false;
        while (true)
        {
            if (t_bits == 0)
            {
                if (notfirst)
                {
                    *X = target;
                    if (ascending)
                    {
                        if (t_base == t_hi_base) break;
                        t_base++;
                        X++;
                    }
                    else
                    {
                        if (t_base == t_lo_base) break;
                        t_base--;
                        X--;
                    }
                }
                select = ((t_base == t_hi_base) << 1) | (t_base == t_lo_base);
                switch (select)
                {
                    case 0:
                        t_lower = 0;
                        t_upper = BITS - 1;
                        t_bits = BITS;
                        target = 0;
                        break;
                    case 1:
                        t_lower = t_lo_bit;
                        t_upper = BITS - 1;
                        t_bits = BITS - t_lo_bit;
                        mask = (unsigned int) (~0L << t_lower);
                        target = *X & ~ mask;
                        break;
                    case 2:
                        t_lower = 0;
                        t_upper = t_hi_bit;
                        t_bits = t_hi_bit + 1;
                        mask = (unsigned int) ((~0L << t_upper) << 1);
                        target = *X & mask;
                        break;
                    case 3:
                        t_lower = t_lo_bit;
                        t_upper = t_hi_bit;
                        t_bits = t_hi_bit - t_lo_bit + 1;
                        mask = (unsigned int) (~0L << t_lower);
                        mask &= (unsigned int) ~((~0L << t_upper) << 1);
                        target = *X & ~ mask;
                        break;
                }
            }
            if (s_bits == 0)
            {
                if (notfirst)
                {
                    if (ascending)
                    {
                        if (s_base == s_hi_base) break;
                        s_base++;
                        Y++;
                    }
                    else
                    {
                        if (s_base == s_lo_base) break;
                        s_base--;
                        Y--;
                    }
                }
                source = *Y;
                select = ((s_base == s_hi_base) << 1) | (s_base == s_lo_base);
                switch (select)
                {
                    case 0:
                        s_lower = 0;
                        s_upper = BITS - 1;
                        s_bits = BITS;
                        break;
                    case 1:
                        s_lower = s_lo_bit;
                        s_upper = BITS - 1;
                        s_bits = BITS - s_lo_bit;
                        break;
                    case 2:
                        s_lower = 0;
                        s_upper = s_hi_bit;
                        s_bits = s_hi_bit + 1;
                        break;
                    case 3:
                        s_lower = s_lo_bit;
                        s_upper = s_hi_bit;
                        s_bits = s_hi_bit - s_lo_bit + 1;
                        break;
                }
            }
            notfirst = true;
            if (s_bits > t_bits)
            {
                bits = t_bits - 1;
                if (ascending)
                {
                    s_min = s_lower;
                    s_max = s_lower + bits;
                }
                else
                {
                    s_max = s_upper;
                    s_min = s_upper - bits;
                }
                t_min = t_lower;
            }
            else
            {
                bits = s_bits - 1;
                if (ascending) t_min = t_lower;
                else           t_min = t_upper - bits;
                s_min = s_lower;
                s_max = s_upper;
            }
            bits++;
            mask = (unsigned int) (~0L << s_min);
            mask &= (unsigned int) ~((~0L << s_max) << 1);
            if (s_min == t_min) target |= (source & mask);
            else
            {
                if (s_min < t_min) target |= (source & mask) << (t_min-s_min);
                else               target |= (source & mask) >> (s_min-t_min);
            }
            if (ascending)
            {
                s_lower += bits;
                t_lower += bits;
            }
            else
            {
                s_upper -= bits;
                t_upper -= bits;
            }
            s_bits -= bits;
            t_bits -= bits;
        }
        *(Z+size_(Z)-1) &= mask_(Z);
    }
}


unsigned int *  BitVector_Interval_Substitute(unsigned int *  X, unsigned int *  Y,
                                      unsigned int Xoffset, unsigned int Xlength,
                                      unsigned int Yoffset, unsigned int Ylength)
{
    unsigned int Xbits = bits_(X);
    unsigned int Ybits = bits_(Y);
    unsigned int limit;
    unsigned int diff;

    if ((Xoffset <= Xbits) && (Yoffset <= Ybits))
    {
        limit = Xoffset + Xlength;
        if (limit > Xbits)
        {
            limit = Xbits;
            Xlength = Xbits - Xoffset;
        }
        if ((Yoffset + Ylength) > Ybits)
        {
            Ylength = Ybits - Yoffset;
        }
        if (Xlength == Ylength)
        {
            if ((Ylength > 0) && ((X != Y) || (Xoffset != Yoffset)))
            {
                BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
            }
        }
        else /* Xlength != Ylength */
        {
            if (Xlength > Ylength)
            {
                diff = Xlength - Ylength;
                if (Ylength > 0) BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                if (limit < Xbits) BitVector_Delete(X,Xoffset+Ylength,diff,false);
                if ((X = BitVector_Resize(X,Xbits-diff)) == NULL) return(NULL);
            }
            else /* Ylength > Xlength  ==>  Ylength > 0 */
            {
                diff = Ylength - Xlength;
                if (X != Y)
                {
                    if ((X = BitVector_Resize(X,Xbits+diff)) == NULL) return(NULL);
                    if (limit < Xbits) BitVector_Insert(X,limit,diff,false);
                    BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                }
                else /* in-place */
                {
                    if ((Y = X = BitVector_Resize(X,Xbits+diff)) == NULL) return(NULL);
                    if (limit >= Xbits)
                    {
                        BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                    }
                    else /* limit < Xbits */
                    {
                        BitVector_Insert(X,limit,diff,false);
                        if ((Yoffset+Ylength) <= limit)
                        {
                            BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                        }
                        else /* overlaps or lies above critical area */
                        {
                            if (limit <= Yoffset)
                            {
                                Yoffset += diff;
                                BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                            }
                            else /* Yoffset < limit */
                            {
                                Xlength = limit - Yoffset;
                                BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Xlength);
                                Yoffset = Xoffset + Ylength; /* = limit + diff */
                                Xoffset += Xlength;
                                Ylength -= Xlength;
                                BitVector_Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                            }
                        }
                    }
                }
            }
        }
    }
    return(X);
}

boolean BitVector_is_empty(unsigned int *  addr)                    /* X == {} ?     */
{
    unsigned int  size = size_(addr);
    boolean r = true;

    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while (r && (size-- > 0)) r = ( *addr++ == 0 );
    }
    return(r);
}

boolean BitVector_is_full(unsigned int *  addr)                     /* X == ~{} ?    */
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    boolean r = false;
    unsigned int *  last;

    if (size > 0)
    {
        r = true;
        last = addr + size - 1;
        *last |= ~ mask;
        while (r && (size-- > 0)) r = ( ~ *addr++ == 0 );
        *last &= mask;
    }
    return(r);
}

boolean BitVector_equal(unsigned int *  X, unsigned int *  Y)               /* X == Y ?      */
{
    unsigned int  size = size_(X);
    unsigned int  mask = mask_(X);
    boolean r = false;

    if (bits_(X) == bits_(Y))
    {
        r = true;
        if (size > 0)
        {
            *(X+size-1) &= mask;
            *(Y+size-1) &= mask;
            while (r && (size-- > 0)) r = (*X++ == *Y++);
        }
    }
    return(r);
}

/* X <,=,> Y ?     : unsigned     */
signed int BitVector_Lexicompare(unsigned int *  X, unsigned int *  Y) {
    unsigned int  bitsX = bits_(X);
    unsigned int  bitsY = bits_(Y);
    unsigned int  size  = size_(X);
    boolean r = true;

    if (bitsX == bitsY) {
        if (size > 0) {
            X += size;
            Y += size;
            while (r && (size-- > 0)) r = (*(--X) == *(--Y));
        }
        if (r) return((signed int) 0);
        else {
            if (*X < *Y) return((signed int) -1); else return((signed int) 1);
        }
    }
    else {
        if (bitsX < bitsY) return((signed int) -1); else return((signed int) 1);
    }
}

signed int BitVector_Compare(unsigned int *  X, unsigned int *  Y)               /* X <,=,> Y ?   */
{                                                           /*   signed      */
    unsigned int  bitsX = bits_(X);
    unsigned int  bitsY = bits_(Y);
    unsigned int  size  = size_(X);
    unsigned int  mask  = mask_(X);
    unsigned int  sign;
    boolean r = true;

    if (bitsX == bitsY)
    {
        if (size > 0)
        {
            X += size;
            Y += size;
            mask &= ~ (mask >> 1);
            if ((sign = (*(X-1) & mask)) != (*(Y-1) & mask))
            {
                if (sign) return((signed int) -1); else return((signed int) 1);
            }
            while (r && (size-- > 0)) r = (*(--X) == *(--Y));
        }
        if (r) return((signed int) 0);
        else
        {
            if (*X < *Y) return((signed int) -1); else return((signed int) 1);
        }
    }
    else
    {
        if (bitsX < bitsY) return((signed int) -1); else return((signed int) 1);
    }
}

size_t BitVector_Hash(unsigned int * addr)
{
  unsigned int  bits = bits_(addr);
  unsigned int  size = size_(addr);
  unsigned int  value;
  unsigned int  count;
  unsigned int  digit;
  unsigned int  length;
    
  size_t result = 0;

  length = bits >> 2;
  if (bits & 0x0003) length++;
  if (size > 0)
    {
      *(addr+size-1) &= mask_(addr);
      while ((size-- > 0) && (length > 0))
        {
          value = *addr++;
          count = BITS >> 2;
          while ((count-- > 0) && (length > 0))
            {
              digit = value & 0x000F;
              if (digit > 9) digit += (unsigned int) 'A' - 10;
              else           digit += (unsigned int) '0';
              result = 5*result + digit; length--;
              if ((count > 0) && (length > 0)) value >>= 4;
            }
        }
    }
  return result;
}


unsigned char * BitVector_to_Hex(unsigned int *  addr)
{
    unsigned int  bits = bits_(addr);
    unsigned int  size = size_(addr);
    unsigned int  value;
    unsigned int  count;
    unsigned int  digit;
    unsigned int  length;
    unsigned char * string;

    length = bits >> 2;
    if (bits & 0x0003) length++;
    string = (unsigned char *) malloc((size_t) (length+1));
    if (string == NULL) return(NULL);
    string += length;
    *string = (unsigned char) '\0';
    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while ((size-- > 0) && (length > 0))
        {
            value = *addr++;
            count = BITS >> 2;
            while ((count-- > 0) && (length > 0))
            {
                digit = value & 0x000F;
                if (digit > 9) digit += (unsigned int) 'A' - 10;
                else           digit += (unsigned int) '0';
                *(--string) = (unsigned char) digit; length--;
                if ((count > 0) && (length > 0)) value >>= 4;
            }
        }
    }
    return(string);
}

ErrCode BitVector_from_Hex(unsigned int *  addr, unsigned char * string)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    boolean ok = true;
    unsigned int  length;
    unsigned int  value;
    unsigned int  count;
    int     digit;

    if (size > 0)
    {
        length = strlen((char *) string);
        string += length;
        while (size-- > 0)
        {
            value = 0;
            for ( count = 0; (ok && (length > 0) && (count < BITS)); count += 4 )
            {
                digit = (int) *(--string); length--;
                /* separate because toupper() is likely a macro! */
                digit = toupper(digit);
                if ((ok = (isxdigit(digit) != 0)))
                {
		  if (digit >= (int) 'A') digit -= (int) 'A' - 10;
		  else                    digit -= (int) '0';
		  value |= (((unsigned int) digit) << count);
                }
            }
            *addr++ = value;
        }
        *(--addr) &= mask;
    }
    if (ok) return(ErrCode_Ok);
    else    return(ErrCode_Pars);
}

unsigned char * BitVector_to_Bin(unsigned int *  addr)
{
    unsigned int  size = size_(addr);
    unsigned int  value;
    unsigned int  count;
    unsigned int  digit;
    unsigned int  length;
    unsigned char * string;

    length = bits_(addr);
    string = (unsigned char *) malloc((size_t) (length+1));
    if (string == NULL) return(NULL);
    string += length;
    *string = (unsigned char) '\0';
    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while (size-- > 0)
        {
            value = *addr++;
            count = BITS;
            if (count > length) count = length;
            while (count-- > 0)
            {
                digit = value & 0x0001;
                digit += (unsigned int) '0';
                *(--string) = (unsigned char) digit; length--;
                if (count > 0) value >>= 1;
            }
        }
    }
    return(string);
}

ErrCode BitVector_from_Bin(unsigned int *  addr, unsigned char * string)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    boolean ok = true;
    unsigned int  length;
    unsigned int  value;
    unsigned int  count;
    int     digit;

    if (size > 0)
    {
        length = strlen((char *) string);
        string += length;
        while (size-- > 0)
        {
            value = 0;
            for ( count = 0; (ok && (length > 0) && (count < BITS)); count++ )
            {
                digit = (int) *(--string); length--;
                switch (digit)
                {
                    case (int) '0':
                        break;
                    case (int) '1':
                        value |= BITMASKTAB[count];
                        break;
                    default:
                        ok = false;
                        break;
                }
            }
            *addr++ = value;
        }
        *(--addr) &= mask;
    }
    if (ok) return(ErrCode_Ok);
    else    return(ErrCode_Pars);
}

unsigned char * BitVector_to_Dec(unsigned int *  addr)
{
    unsigned int  bits = bits_(addr);
    unsigned int  length;
    unsigned int  digits;
    unsigned int  count;
    unsigned int  q;
    unsigned int  r;
    boolean loop;
    unsigned char * result;
    unsigned char * string;
    unsigned int *  quot;
    unsigned int *  rest;
    unsigned int *  temp;
    unsigned int *  base;
    signed int   sign;

    length = (unsigned int) (bits / 3.3);        /* digits = bits * ln(2) / ln(10) */
    length += 2; /* compensate for truncating & provide space for minus sign */
    result = (unsigned char *) malloc((size_t) (length+1));    /* remember the '\0'! */
    if (result == NULL) return(NULL);
    string = result;
    sign = BitVector_Sign(addr);
    if ((bits < 4) || (sign == 0))
    {
        if (bits > 0) digits = *addr; else digits = (unsigned int) 0;
        if (sign < 0) digits = ((unsigned int)(-((signed int)digits))) & mask_(addr);
        *string++ = (unsigned char) digits + (unsigned char) '0';
        digits = 1;
    }
    else
    {
        quot = BitVector_Create(bits,false);
        if (quot == NULL)
        {
            BitVector_Dispose(result);
            return(NULL);
        }
        rest = BitVector_Create(bits,false);
        if (rest == NULL)
        {
            BitVector_Dispose(result);
            BitVector_Destroy(quot);
            return(NULL);
        }
        temp = BitVector_Create(bits,false);
        if (temp == NULL)
        {
            BitVector_Dispose(result);
            BitVector_Destroy(quot);
            BitVector_Destroy(rest);
            return(NULL);
        }
        base = BitVector_Create(bits,true);
        if (base == NULL)
        {
            BitVector_Dispose(result);
            BitVector_Destroy(quot);
            BitVector_Destroy(rest);
            BitVector_Destroy(temp);
            return(NULL);
        }
        if (sign < 0) BitVector_Negate(quot,addr);
        else           BitVector_Copy(quot,addr);
        digits = 0;
        *base = EXP10;
        loop = (bits >= BITS);
        do
        {
            if (loop)
            {
                BitVector_Copy(temp,quot);
                if (BitVector_Div_Pos(quot,temp,base,rest))
                {
                    BitVector_Dispose(result); /* emergency exit */
                    BitVector_Destroy(quot);
                    BitVector_Destroy(rest);   /* should never occur */
                    BitVector_Destroy(temp);   /* under normal operation */
                    BitVector_Destroy(base);
                    return(NULL);
                }
                loop = ! BitVector_is_empty(quot);
                q = *rest;
            }
            else q = *quot;
            count = LOG10;
            while (((loop && (count-- > 0)) || ((! loop) && (q != 0))) &&
                (digits < length))
            {
                if (q != 0)
                {
                    BIT_VECTOR_DIGITIZE(unsigned int,q,r)
                }
                else r = (unsigned int) '0';
                *string++ = (unsigned char) r;
                digits++;
            }
        }
        while (loop && (digits < length));
        BitVector_Destroy(quot);
        BitVector_Destroy(rest);
        BitVector_Destroy(temp);
        BitVector_Destroy(base);
    }
    if ((sign < 0) && (digits < length))
    {
        *string++ = (unsigned char) '-';
        digits++;
    }
    *string = (unsigned char) '\0';
    BIT_VECTOR_reverse(result,digits);
    return(result);
}

ErrCode BitVector_from_Dec(unsigned int *  addr, unsigned char * string)
{
    ErrCode error = ErrCode_Ok;
    unsigned int  bits = bits_(addr);
    unsigned int  mask = mask_(addr);
    boolean init = (bits > BITS);
    boolean minus;
    boolean shift;
    boolean carry;
    unsigned int *  term;
    unsigned int *  base;
    unsigned int *  prod;
    unsigned int *  rank;
    unsigned int *  temp;
    unsigned int  accu;
    unsigned int  powr;
    unsigned int  count;
    unsigned int  length;
    int     digit;

    if (bits > 0)
    {
        length = strlen((char *) string);
        if (length == 0) return(ErrCode_Pars);
        digit = (int) *string;
        if ((minus = (digit == (int) '-')) ||
                     (digit == (int) '+'))
        {
            string++;
            if (--length == 0) return(ErrCode_Pars);
        }
        string += length;
        term = BitVector_Create(BITS,false);
        if (term == NULL)
        {
            return(ErrCode_Null);
        }
        base = BitVector_Create(BITS,false);
        if (base == NULL)
        {
            BitVector_Destroy(term);
            return(ErrCode_Null);
        }
        prod = BitVector_Create(bits,init);
        if (prod == NULL)
        {
            BitVector_Destroy(term);
            BitVector_Destroy(base);
            return(ErrCode_Null);
        }
        rank = BitVector_Create(bits,init);
        if (rank == NULL)
        {
            BitVector_Destroy(term);
            BitVector_Destroy(base);
            BitVector_Destroy(prod);
            return(ErrCode_Null);
        }
        temp = BitVector_Create(bits,false);
        if (temp == NULL)
        {
            BitVector_Destroy(term);
            BitVector_Destroy(base);
            BitVector_Destroy(prod);
            BitVector_Destroy(rank);
            return(ErrCode_Null);
        }
        BitVector_Empty(addr);
        *base = EXP10;
        shift = false;
        while ((! error) && (length > 0))
        {
            accu = 0;
            powr = 1;
            count = LOG10;
            while ((! error) && (length > 0) && (count-- > 0))
            {
                digit = (int) *(--string); length--;
                /* separate because isdigit() is likely a macro! */
                if (isdigit(digit) != 0)
                {
                    accu += ((unsigned int) digit - (unsigned int) '0') * powr;
                    powr *= 10;
                }
                else error = ErrCode_Pars;
            }
            if (! error)
            {
                if (shift)
                {
                    *term = accu;
                    BitVector_Copy(temp,rank);
                    error = BitVector_Mul_Pos(prod,temp,term,false);
                }
                else
                {
                    *prod = accu;
                    if ((! init) && ((accu & ~ mask) != 0)) error = ErrCode_Ovfl;
                }
                if (! error)
                {
                    carry = false;
                    BitVector_compute(addr,addr,prod,false,&carry);
                    /* ignores sign change (= overflow) but ! */
                    /* numbers too large (= carry) for resulting bit vector */
                    if (carry) error = ErrCode_Ovfl;
                    else
                    {
                        if (length > 0)
                        {
                            if (shift)
                            {
                                BitVector_Copy(temp,rank);
                                error = BitVector_Mul_Pos(rank,temp,base,false);
                            }
                            else
                            {
                                *rank = *base;
                                shift = true;
                            }
                        }
                    }
                }
            }
        }
        BitVector_Destroy(term);
        BitVector_Destroy(base);
        BitVector_Destroy(prod);
        BitVector_Destroy(rank);
        BitVector_Destroy(temp);
        if (! error && minus)
        {
            BitVector_Negate(addr,addr);
            if ((*(addr + size_(addr) - 1) & mask & ~ (mask >> 1)) == 0)
                error = ErrCode_Ovfl;
        }
    }
    return(error);
}

unsigned char * BitVector_to_Enum(unsigned int *  addr)
{
    unsigned int  bits = bits_(addr);
    unsigned int  sample;
    unsigned int  length;
    unsigned int  digits;
    unsigned int  factor;
    unsigned int  power;
    unsigned int  start;
    unsigned int  min;
    unsigned int  max;
    unsigned char * string;
    unsigned char * target;
    boolean comma;

    if (bits > 0)
    {
        sample = bits - 1;  /* greatest possible index */
        length = 2;         /* account for index 0 && terminating '\0' */
        digits = 1;         /* account for intervening dashes && commas */
        factor = 1;
        power = 10;
        while (sample >= (power-1))
        {
            length += ++digits * factor * 6;  /* 9,90,900,9000,... (9*2/3 = 6) */
            factor = power;
            power *= 10;
        }
        if (sample > --factor)
        {
            sample -= factor;
            factor = (unsigned int) ( sample / 3 );
            factor = (factor << 1) + (sample - (factor * 3));
            length += ++digits * factor;
        }
    }
    else length = 1;
    string = (unsigned char *) malloc((size_t) length);
    if (string == NULL) return(NULL);
    start = 0;
    comma = false;
    target = string;
    while ((start < bits) && BitVector_interval_scan_inc(addr,start,&min,&max))
    {
        start = max + 2;
        if (comma) *target++ = (unsigned char) ',';
        if (min == max)
        {
            target += BIT_VECTOR_int2str(target,min);
        }
        else
        {
            if (min+1 == max)
            {
                target += BIT_VECTOR_int2str(target,min);
                *target++ = (unsigned char) ',';
                target += BIT_VECTOR_int2str(target,max);
            }
            else
            {
                target += BIT_VECTOR_int2str(target,min);
                *target++ = (unsigned char) '-';
                target += BIT_VECTOR_int2str(target,max);
            }
        }
        comma = true;
    }
    *target = (unsigned char) '\0';
    return(string);
}

ErrCode BitVector_from_Enum(unsigned int *  addr, unsigned char * string)
{
    ErrCode error = ErrCode_Ok;
    unsigned int  bits = bits_(addr);
    unsigned int  state = 1;
    unsigned int  token;
    unsigned int  index = 0;
    unsigned int  start = 0;         /* silence compiler warning */

    if (bits > 0)
    {
        BitVector_Empty(addr);
        while ((! error) && (state != 0))
        {
            token = (unsigned int) *string;
            /* separate because isdigit() is likely a macro! */
            if (isdigit((int)token) != 0)
            {
                string += BIT_VECTOR_str2int(string,&index);
                if (index < bits) token = (unsigned int) '0';
                else error = ErrCode_Indx;
            }
            else string++;
            if (! error)
            switch (state)
            {
                case 1:
                    switch (token)
                    {
                        case (unsigned int) '0':
                            state = 2;
                            break;
                        case (unsigned int) '\0':
                            state = 0;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 2:
                    switch (token)
                    {
                        case (unsigned int) '-':
                            start = index;
                            state = 3;
                            break;
                        case (unsigned int) ',':
                            BIT_VECTOR_SET_BIT(addr,index)
                            state = 5;
                            break;
                        case (unsigned int) '\0':
                            BIT_VECTOR_SET_BIT(addr,index)
                            state = 0;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 3:
                    switch (token)
                    {
                        case (unsigned int) '0':
                            if (start < index)
                                BitVector_Interval_Fill(addr,start,index);
                            else if (start == index)
                                BIT_VECTOR_SET_BIT(addr,index)
                            else error = ErrCode_Ordr;
                            state = 4;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 4:
                    switch (token)
                    {
                        case (unsigned int) ',':
                            state = 5;
                            break;
                        case (unsigned int) '\0':
                            state = 0;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 5:
                    switch (token)
                    {
                        case (unsigned int) '0':
                            state = 2;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
            }
        }
    }
    return(error);
}

void BitVector_Bit_Off(unsigned int *  addr, unsigned int index)           /* X = X \ {x}   */
{
    if (index < bits_(addr)) BIT_VECTOR_CLR_BIT(addr,index)
}

void BitVector_Bit_On(unsigned int *  addr, unsigned int index)            /* X = X + {x}   */
{
    if (index < bits_(addr)) BIT_VECTOR_SET_BIT(addr,index)
}

boolean BitVector_bit_flip(unsigned int *  addr, unsigned int index)   /* X=(X+{x})\(X*{x}) */
{
    unsigned int mask;

    if (index < bits_(addr)) return( BIT_VECTOR_FLP_BIT(addr,index,mask) );
    else                     return( false );
}

boolean BitVector_bit_test(unsigned int *  addr, unsigned int index)       /* {x} in X ?    */
{
    if (index < bits_(addr)) return( BIT_VECTOR_TST_BIT(addr,index) );
    else                     return( false );
}

void BitVector_Bit_Copy(unsigned int *  addr, unsigned int index, boolean bit)
{
    if (index < bits_(addr))
    {
        if (bit) BIT_VECTOR_SET_BIT(addr,index)
        else     BIT_VECTOR_CLR_BIT(addr,index)
    }
}

void BitVector_LSB(unsigned int *  addr, boolean bit)
{
    if (bits_(addr) > 0)
    {
        if (bit) *addr |= LSB;
        else     *addr &= ~ LSB;
    }
}

void BitVector_MSB(unsigned int *  addr, boolean bit)
{
    unsigned int size = size_(addr);
    unsigned int mask = mask_(addr);

    if (size-- > 0)
    {
        if (bit) *(addr+size) |= mask & ~ (mask >> 1);
        else     *(addr+size) &= ~ mask | (mask >> 1);
    }
}

boolean BitVector_lsb_(unsigned int *  addr)
{
    if (size_(addr) > 0) return( (*addr & LSB) != 0 );
    else                 return( false );
}

boolean BitVector_msb_(unsigned int *  addr)
{
    unsigned int size = size_(addr);
    unsigned int mask = mask_(addr);

    if (size-- > 0)
        return( (*(addr+size) & (mask & ~ (mask >> 1))) != 0 );
    else
        return( false );
}

boolean BitVector_rotate_left(unsigned int *  addr)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int  msb;
    boolean carry_in;
    boolean carry_out = false;

    if (size > 0)
    {
        msb = mask & ~ (mask >> 1);
        carry_in = ((*(addr+size-1) & msb) != 0);
        while (size-- > 1)
        {
            carry_out = ((*addr & MSB) != 0);
            *addr <<= 1;
            if (carry_in) *addr |= LSB;
            carry_in = carry_out;
            addr++;
        }
        carry_out = ((*addr & msb) != 0);
        *addr <<= 1;
        if (carry_in) *addr |= LSB;
        *addr &= mask;
    }
    return(carry_out);
}

boolean BitVector_rotate_right(unsigned int *  addr)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int  msb;
    boolean carry_in;
    boolean carry_out = false;

    if (size > 0)
    {
        msb = mask & ~ (mask >> 1);
        carry_in = ((*addr & LSB) != 0);
        addr += size-1;
        *addr &= mask;
        carry_out = ((*addr & LSB) != 0);
        *addr >>= 1;
        if (carry_in) *addr |= msb;
        carry_in = carry_out;
        addr--;
        size--;
        while (size-- > 0)
        {
            carry_out = ((*addr & LSB) != 0);
            *addr >>= 1;
            if (carry_in) *addr |= MSB;
            carry_in = carry_out;
            addr--;
        }
    }
    return(carry_out);
}

boolean BitVector_shift_left(unsigned int *  addr, boolean carry_in)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int  msb;
    boolean carry_out = carry_in;

    if (size > 0)
    {
        msb = mask & ~ (mask >> 1);
        while (size-- > 1)
        {
            carry_out = ((*addr & MSB) != 0);
            *addr <<= 1;
            if (carry_in) *addr |= LSB;
            carry_in = carry_out;
            addr++;
        }
        carry_out = ((*addr & msb) != 0);
        *addr <<= 1;
        if (carry_in) *addr |= LSB;
        *addr &= mask;
    }
    return(carry_out);
}

boolean BitVector_shift_right(unsigned int *  addr, boolean carry_in)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int  msb;
    boolean carry_out = carry_in;

    if (size > 0)
    {
        msb = mask & ~ (mask >> 1);
        addr += size-1;
        *addr &= mask;
        carry_out = ((*addr & LSB) != 0);
        *addr >>= 1;
        if (carry_in) *addr |= msb;
        carry_in = carry_out;
        addr--;
        size--;
        while (size-- > 0)
        {
            carry_out = ((*addr & LSB) != 0);
            *addr >>= 1;
            if (carry_in) *addr |= MSB;
            carry_in = carry_out;
            addr--;
        }
    }
    return(carry_out);
}

void BitVector_Move_Left(unsigned int *  addr, unsigned int bits)
{
    unsigned int count;
    unsigned int words;

    if (bits > 0)
    {
        count = bits & MODMASK;
        words = bits >> LOGBITS;
        if (bits >= bits_(addr)) BitVector_Empty(addr);
        else
        {
            while (count-- > 0) BitVector_shift_left(addr,0);
            BitVector_Word_Insert(addr,0,words,true);
        }
    }
}

void BitVector_Move_Right(unsigned int *  addr, unsigned int bits)
{
    unsigned int count;
    unsigned int words;

    if (bits > 0)
    {
        count = bits & MODMASK;
        words = bits >> LOGBITS;
        if (bits >= bits_(addr)) BitVector_Empty(addr);
        else
        {
            while (count-- > 0) BitVector_shift_right(addr,0);
            BitVector_Word_Delete(addr,0,words,true);
        }
    }
}

void BitVector_Insert(unsigned int *  addr, unsigned int offset, unsigned int count, boolean clear)
{
    unsigned int bits = bits_(addr);
    unsigned int last;

    if ((count > 0) && (offset < bits))
    {
        last = offset + count;
        if (last < bits)
        {
            BitVector_Interval_Copy(addr,addr,last,offset,(bits-last));
        }
        else last = bits;
        if (clear) BitVector_Interval_Empty(addr,offset,(last-1));
    }
}

void BitVector_Delete(unsigned int *  addr, unsigned int offset, unsigned int count, boolean clear)
{
    unsigned int bits = bits_(addr);
    unsigned int last;

    if ((count > 0) && (offset < bits))
    {
        last = offset + count;
        if (last < bits)
        {
            BitVector_Interval_Copy(addr,addr,offset,last,(bits-last));
        }
        else count = bits - offset;
        if (clear) BitVector_Interval_Empty(addr,(bits-count),(bits-1));
    }
}

boolean BitVector_increment(unsigned int *  addr)                   /* X++           */
{
    unsigned int  size  = size_(addr);
    unsigned int  mask  = mask_(addr);
    unsigned int *  last  = addr + size - 1;
    boolean carry = true;

    if (size > 0)
    {
        *last |= ~ mask;
        while (carry && (size-- > 0))
        {
            carry = (++(*addr++) == 0);
        }
        *last &= mask;
    }
    return(carry);
}

boolean BitVector_decrement(unsigned int *  addr)                   /* X--           */
{
    unsigned int  size  = size_(addr);
    unsigned int  mask  = mask_(addr);
    unsigned int *  last  = addr + size - 1;
    boolean carry = true;

    if (size > 0)
    {
        *last &= mask;
        while (carry && (size-- > 0))
        {
            carry = (*addr == 0);
            --(*addr++);
        }
        *last &= mask;
    }
    return(carry);
}

boolean BitVector_compute(unsigned int *  X, unsigned int *  Y, unsigned int *  Z, boolean minus, boolean *carry)
{
    unsigned int size = size_(X);
    unsigned int mask = mask_(X);
    unsigned int vv = 0;
    unsigned int cc;
    unsigned int mm;
    unsigned int yy;
    unsigned int zz;
    unsigned int lo;
    unsigned int hi;

    if (size > 0)
    {
        if (minus) cc = (*carry == 0);
        else       cc = (*carry != 0);
        /* deal with (size-1) least significant full words first: */
        while (--size > 0)
        {
            yy = *Y++;
            if (minus) zz = (unsigned int) ~ ( Z ? *Z++ : 0 );
            else       zz = (unsigned int)     ( Z ? *Z++ : 0 );
            lo = (yy & LSB) + (zz & LSB) + cc;
            hi = (yy >> 1) + (zz >> 1) + (lo >> 1);
            cc = ((hi & MSB) != 0);
            *X++ = (hi << 1) | (lo & LSB);
        }
        /* deal with most significant word (may be used only partially): */
        yy = *Y & mask;
        if (minus) zz = (unsigned int) ~ ( Z ? *Z : 0 );
        else       zz = (unsigned int)     ( Z ? *Z : 0 );
        zz &= mask;
        if (mask == LSB) /* special case, only one bit used */
        {
            vv = cc;
            lo = yy + zz + cc;
            cc = (lo >> 1);
            vv ^= cc;
            *X = lo & LSB;
        }
        else
        {
            if (~ mask) /* not all bits are used, but more than one */
            {
                mm = (mask >> 1);
                vv = (yy & mm) + (zz & mm) + cc;
                mm = mask & ~ mm;
                lo = yy + zz + cc;
                cc = (lo >> 1);
                vv ^= cc;
                vv &= mm;
                cc &= mm;
                *X = lo & mask;
            }
            else /* other special case, all bits are used */
            {
                mm = ~ MSB;
                lo = (yy & mm) + (zz & mm) + cc;
                vv = lo & MSB;
                hi = ((yy & MSB) >> 1) + ((zz & MSB) >> 1) + (vv >> 1);
                cc = hi & MSB;
                vv ^= cc;
                *X = (hi << 1) | (lo & mm);
            }
        }
        if (minus) *carry = (cc == 0);
        else       *carry = (cc != 0);
    }
    return(vv != 0);
}

boolean BitVector_add(unsigned int *  X, unsigned int *  Y, unsigned int *  Z, boolean *carry)
{
    return(BitVector_compute(X,Y,Z,false,carry));
}

boolean BitVector_sub(unsigned int *  X, unsigned int *  Y, unsigned int *  Z, boolean *carry)
{
    return(BitVector_compute(X,Y,Z,true,carry));
}

boolean BitVector_inc(unsigned int *  X, unsigned int *  Y)
{
    boolean carry = true;

    return(BitVector_compute(X,Y,NULL,false,&carry));
}

boolean BitVector_dec(unsigned int *  X, unsigned int *  Y)
{
    boolean carry = true;

    return(BitVector_compute(X,Y,NULL,true,&carry));
}

void BitVector_Negate(unsigned int *  X, unsigned int *  Y)
{
    unsigned int  size  = size_(X);
    unsigned int  mask  = mask_(X);
    boolean carry = true;

    if (size > 0)
    {
        while (size-- > 0)
        {
            *X = ~ *Y++;
            if (carry)
            {
                carry = (++(*X) == 0);
            }
            X++;
        }
        *(--X) &= mask;
    }
}

void BitVector_Absolute(unsigned int *  X, unsigned int *  Y)
{
    unsigned int size = size_(Y);
    unsigned int mask = mask_(Y);

    if (size > 0)
    {
        if (*(Y+size-1) & (mask & ~ (mask >> 1))) BitVector_Negate(X,Y);
        else                                             BitVector_Copy(X,Y);
    }
}

// FIXME:  What the hell does the return value of this mean?
// It returns 0, 1, or -1 under mysterious circumstances.
signed int BitVector_Sign(unsigned int *  addr)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int *  last = addr + size - 1;
    boolean r    = true;

    if (size > 0)
    {
        *last &= mask;
        while (r && (size-- > 0)) r = ( *addr++ == 0 );
    }
    if (r) return((signed int) 0);
    else
    {
        if (*last & (mask & ~ (mask >> 1))) return((signed int) -1);
        else                                      return((signed int)  1);
    }
}

ErrCode BitVector_Mul_Pos(unsigned int *  X, unsigned int *  Y, unsigned int *  Z, boolean strict)
{
    unsigned int  mask;
    unsigned int  limit;
    unsigned int  count;
    signed long  last;
    unsigned int *  sign;
    boolean carry;
    boolean overflow;
    boolean ok = true;

    /*
       Requirements:
         -  X, Y && Z must be distinct
         -  X && Y must have equal sizes (whereas Z may be any size!)
         -  Z should always contain the SMALLER of the two factors Y && Z
       Constraints:
         -  The contents of Y (&& of X, of course) are destroyed
            (only Z is preserved!)
    */

    if ((X == Y) || (X == Z) || (Y == Z)) return(ErrCode_Same);
    if (bits_(X) != bits_(Y)) return(ErrCode_Size);
    BitVector_Empty(X);
    if (BitVector_is_empty(Y)) return(ErrCode_Ok); /* exit also taken if bits_(Y)==0 */
    if ((last = Set_Max(Z)) < 0L) return(ErrCode_Ok);
    limit = (unsigned int) last;
    sign = Y + size_(Y) - 1;
    mask = mask_(Y);
    *sign &= mask;
    mask &= ~ (mask >> 1);
    for ( count = 0; (ok && (count <= limit)); count++ )
    {
        if ( BIT_VECTOR_TST_BIT(Z,count) )
        {
            carry = false;
            overflow = BitVector_compute(X,X,Y,false,&carry);
            if (strict) ok = ! (carry || overflow);
            else        ok = !  carry;
        }
        if (ok && (count < limit))
        {
            carry = BitVector_shift_left(Y,0);
            if (strict)
            {
                overflow = ((*sign & mask) != 0);
                ok = ! (carry || overflow);
            }
            else ok = ! carry;
        }
    }
    if (ok) return(ErrCode_Ok); else return(ErrCode_Ovfl);
}

ErrCode BitVector_Multiply(unsigned int *  X, unsigned int *  Y, unsigned int *  Z)
{
    ErrCode error = ErrCode_Ok;
    unsigned int  bit_x = bits_(X);
    unsigned int  bit_y = bits_(Y);
    unsigned int  bit_z = bits_(Z);
    unsigned int  size;
    unsigned int  mask;
    unsigned int  msb;
    unsigned int *  ptr_y;
    unsigned int *  ptr_z;
    boolean sgn_x;
    boolean sgn_y;
    boolean sgn_z;
    boolean zero;
    unsigned int *  A;
    unsigned int *  B;

    /*
       Requirements:
         -  Y && Z must have equal sizes
         -  X must have at least the same size as Y && Z but may be larger (!)
       Features:
         -  The contents of Y && Z are preserved
         -  X may be identical with Y or Z (or both!)
            (in-place multiplication is possible!)
    */

    if ((bit_y != bit_z) || (bit_x < bit_y)) return(ErrCode_Size);
    if (BitVector_is_empty(Y) || BitVector_is_empty(Z))
    {
        BitVector_Empty(X);
    }
    else
    {
        A = BitVector_Create(bit_y,false);
        if (A == NULL) return(ErrCode_Null);
        B = BitVector_Create(bit_z,false);
        if (B == NULL) { BitVector_Destroy(A); return(ErrCode_Null); }
        size  = size_(Y);
        mask  = mask_(Y);
        msb   = (mask & ~ (mask >> 1));
        sgn_y = (((*(Y+size-1) &= mask) & msb) != 0);
        sgn_z = (((*(Z+size-1) &= mask) & msb) != 0);
        sgn_x = sgn_y ^ sgn_z;
        if (sgn_y) BitVector_Negate(A,Y); else BitVector_Copy(A,Y);
        if (sgn_z) BitVector_Negate(B,Z); else BitVector_Copy(B,Z);
        ptr_y = A + size;
        ptr_z = B + size;
        zero = true;
        while (zero && (size-- > 0))
        {
            zero &= (*(--ptr_y) == 0);
            zero &= (*(--ptr_z) == 0);
        }
        if (*ptr_y > *ptr_z)
        {
            if (bit_x > bit_y)
            {
                A = BitVector_Resize(A,bit_x);
                if (A == NULL) { BitVector_Destroy(B); return(ErrCode_Null); }
            }
            error = BitVector_Mul_Pos(X,A,B,true);
        }
        else
        {
            if (bit_x > bit_z)
            {
                B = BitVector_Resize(B,bit_x);
                if (B == NULL) { BitVector_Destroy(A); return(ErrCode_Null); }
            }
            error = BitVector_Mul_Pos(X,B,A,true);
        }
        if ((! error) && sgn_x) BitVector_Negate(X,X);
        BitVector_Destroy(A);
        BitVector_Destroy(B);
    }
    return(error);
}

ErrCode BitVector_Div_Pos(unsigned int *  Q, unsigned int *  X, unsigned int *  Y, unsigned int *  R)
{
    unsigned int  bits = bits_(Q);
    unsigned int  mask;
    unsigned int *  addr;
    signed long  last;
    boolean flag;
    boolean copy = false; /* flags whether valid rest is in R (0) || X (1) */

    /*
       Requirements:
         -  All bit vectors must have equal sizes
         -  Q, X, Y && R must all be distinct bit vectors
         -  Y must be non-zero (of course!)
       Constraints:
         -  The contents of X (&& Q && R, of course) are destroyed
            (only Y is preserved!)
    */

    if ((bits != bits_(X)) || (bits != bits_(Y)) || (bits != bits_(R)))
        return(ErrCode_Size);
    if ((Q == X) || (Q == Y) || (Q == R) || (X == Y) || (X == R) || (Y == R))
        return(ErrCode_Same);
    if (BitVector_is_empty(Y))
        return(ErrCode_Zero);

    BitVector_Empty(R);
    BitVector_Copy(Q,X);
    if ((last = Set_Max(Q)) < 0L) return(ErrCode_Ok);
    bits = (unsigned int) ++last;
    while (bits-- > 0)
    {
        addr = Q + (bits >> LOGBITS);
        mask = BITMASKTAB[bits & MODMASK];
        flag = ((*addr & mask) != 0);
        if (copy)
        {
            BitVector_shift_left(X,flag);
            flag = false;
            BitVector_compute(R,X,Y,true,&flag);
        }
        else
        {
            BitVector_shift_left(R,flag);
            flag = false;
            BitVector_compute(X,R,Y,true,&flag);
        }
        if (flag) *addr &= ~ mask;
        else
        {
            *addr |= mask;
            copy = ! copy;
        }
    }
    if (copy) BitVector_Copy(R,X);
    return(ErrCode_Ok);
}

ErrCode BitVector_Divide(unsigned int *  Q, unsigned int *  X, unsigned int *  Y, unsigned int *  R)
{
    ErrCode error = ErrCode_Ok;
    unsigned int  bits = bits_(Q);
    unsigned int  size = size_(Q);
    unsigned int  mask = mask_(Q);
    unsigned int  msb = (mask & ~ (mask >> 1));
    boolean sgn_q;
    boolean sgn_x;
    boolean sgn_y;
    unsigned int *  A;
    unsigned int *  B;

    /*
       Requirements:
         -  All bit vectors must have equal sizes
         -  Q && R must be two distinct bit vectors
         -  Y must be non-zero (of course!)
       Features:
         -  The contents of X && Y are preserved
         -  Q may be identical with X || Y (or both)
            (in-place division is possible!)
         -  R may be identical with X || Y (or both)
            (but not identical with Q!)
    */

    if ((bits != bits_(X)) || (bits != bits_(Y)) || (bits != bits_(R)))
        return(ErrCode_Size);
    if (Q == R)
        return(ErrCode_Same);
    if (BitVector_is_empty(Y))
        return(ErrCode_Zero);

    if (BitVector_is_empty(X))
    {
        BitVector_Empty(Q);
        BitVector_Empty(R);
    }
    else
    {
        A = BitVector_Create(bits,false);
        if (A == NULL) return(ErrCode_Null);
        B = BitVector_Create(bits,false);
        if (B == NULL) { BitVector_Destroy(A); return(ErrCode_Null); }
        size--;
        sgn_x = (((*(X+size) &= mask) & msb) != 0);
        sgn_y = (((*(Y+size) &= mask) & msb) != 0);
        sgn_q = sgn_x ^ sgn_y;
        if (sgn_x) BitVector_Negate(A,X); else BitVector_Copy(A,X);
        if (sgn_y) BitVector_Negate(B,Y); else BitVector_Copy(B,Y);
        if (! (error = BitVector_Div_Pos(Q,A,B,R)))
        {
            if (sgn_q) BitVector_Negate(Q,Q);
            if (sgn_x) BitVector_Negate(R,R);
        }
        BitVector_Destroy(A);
        BitVector_Destroy(B);
    }
    return(error);
}

ErrCode BitVector_GCD(unsigned int *  X, unsigned int *  Y, unsigned int *  Z)
{
    ErrCode error = ErrCode_Ok;
    unsigned int  bits = bits_(X);
    unsigned int  size = size_(X);
    unsigned int  mask = mask_(X);
    unsigned int  msb = (mask & ~ (mask >> 1));
    boolean sgn_a;
    boolean sgn_b;
    boolean sgn_r;
    unsigned int *  Q;
    unsigned int *  R;
    unsigned int *  A;
    unsigned int *  B;
    unsigned int *  T;

    /*
       Requirements:
         -  All bit vectors must have equal sizes
       Features:
         -  The contents of Y && Z are preserved
         -  X may be identical with Y || Z (or both)
            (in-place is possible!)
         -  GCD(0,z) == GCD(z,0) == z
         -  negative values are h&&led correctly
    */

    if ((bits != bits_(Y)) || (bits != bits_(Z))) return(ErrCode_Size);
    if (BitVector_is_empty(Y))
    {
        if (X != Z) BitVector_Copy(X,Z);
        return(ErrCode_Ok);
    }
    if (BitVector_is_empty(Z))
    {
        if (X != Y) BitVector_Copy(X,Y);
        return(ErrCode_Ok);
    }
    Q = BitVector_Create(bits,false);
    if (Q == NULL)
    {
        return(ErrCode_Null);
    }
    R = BitVector_Create(bits,false);
    if (R == NULL)
    {
        BitVector_Destroy(Q);
        return(ErrCode_Null);
    }
    A = BitVector_Create(bits,false);
    if (A == NULL)
    {
        BitVector_Destroy(Q);
        BitVector_Destroy(R);
        return(ErrCode_Null);
    }
    B = BitVector_Create(bits,false);
    if (B == NULL)
    {
        BitVector_Destroy(Q);
        BitVector_Destroy(R);
        BitVector_Destroy(A);
        return(ErrCode_Null);
    }
    size--;
    sgn_a = (((*(Y+size) &= mask) & msb) != 0);
    sgn_b = (((*(Z+size) &= mask) & msb) != 0);
    if (sgn_a) BitVector_Negate(A,Y); else BitVector_Copy(A,Y);
    if (sgn_b) BitVector_Negate(B,Z); else BitVector_Copy(B,Z);
    while (! error)
    {
        if (! (error = BitVector_Div_Pos(Q,A,B,R)))
        {
            if (BitVector_is_empty(R)) break;
            T = A; sgn_r = sgn_a;
            A = B; sgn_a = sgn_b;
            B = R; sgn_b = sgn_r;
            R = T;
        }
    }
    if (! error)
    {
        if (sgn_b) BitVector_Negate(X,B); else BitVector_Copy(X,B);
    }
    BitVector_Destroy(Q);
    BitVector_Destroy(R);
    BitVector_Destroy(A);
    BitVector_Destroy(B);
    return(error);
}

ErrCode BitVector_GCD2(unsigned int *  U, unsigned int *  V, unsigned int *  W, unsigned int *  X, unsigned int *  Y)
{
    ErrCode error = ErrCode_Ok;
    unsigned int  bits = bits_(U);
    unsigned int  size = size_(U);
    unsigned int  mask = mask_(U);
    unsigned int  msb = (mask & ~ (mask >> 1));
    boolean minus;
    boolean carry;
    boolean sgn_q;
    boolean sgn_r;
    boolean sgn_a;
    boolean sgn_b;
    boolean sgn_x;
    boolean sgn_y;
    unsigned int *  *  L;
    unsigned int *  Q;
    unsigned int *  R;
    unsigned int *  A;
    unsigned int *  B;
    unsigned int *  T;
    unsigned int *  X1;
    unsigned int *  X2;
    unsigned int *  X3;
    unsigned int *  Y1;
    unsigned int *  Y2;
    unsigned int *  Y3;
    unsigned int *  Z;

    /*
       Requirements:
         -  All bit vectors must have equal sizes
         -  U, V, && W must all be distinct bit vectors
       Features:
         -  The contents of X && Y are preserved
         -  U, V && W may be identical with X || Y (or both,
            provided that U, V && W are mutually distinct)
            (i.e., in-place is possible!)
         -  GCD(0,z) == GCD(z,0) == z
         -  negative values are h&&led correctly
    */

    if ((bits != bits_(V)) ||
        (bits != bits_(W)) ||
        (bits != bits_(X)) ||
        (bits != bits_(Y)))
    {
        return(ErrCode_Size);
    }
    if ((U == V) || (U == W) || (V == W))
    {
        return(ErrCode_Same);
    }
    if (BitVector_is_empty(X))
    {
        if (U != Y) BitVector_Copy(U,Y);
        BitVector_Empty(V);
        BitVector_Empty(W);
        *W = 1;
        return(ErrCode_Ok);
    }
    if (BitVector_is_empty(Y))
    {
        if (U != X) BitVector_Copy(U,X);
        BitVector_Empty(V);
        BitVector_Empty(W);
        *V = 1;
        return(ErrCode_Ok);
    }
    if ((L = BitVector_Create_List(bits,false,11)) == NULL)
    {
        return(ErrCode_Null);
    }
    Q  = L[0];
    R  = L[1];
    A  = L[2];
    B  = L[3];
    X1 = L[4];
    X2 = L[5];
    X3 = L[6];
    Y1 = L[7];
    Y2 = L[8];
    Y3 = L[9];
    Z  = L[10];
    size--;
    sgn_a = (((*(X+size) &= mask) & msb) != 0);
    sgn_b = (((*(Y+size) &= mask) & msb) != 0);
    if (sgn_a) BitVector_Negate(A,X); else BitVector_Copy(A,X);
    if (sgn_b) BitVector_Negate(B,Y); else BitVector_Copy(B,Y);
    BitVector_Empty(X1);
    BitVector_Empty(X2);
    *X1 = 1;
    BitVector_Empty(Y1);
    BitVector_Empty(Y2);
    *Y2 = 1;
    sgn_x = false;
    sgn_y = false;
    while (! error)
    {
        if ((error = BitVector_Div_Pos(Q,A,B,R)))
        {
            break;
        }
        if (BitVector_is_empty(R))
        {
            break;
        }
        sgn_q = sgn_a ^ sgn_b;

        if (sgn_x) BitVector_Negate(Z,X2); else BitVector_Copy(Z,X2);
        if ((error = BitVector_Mul_Pos(X3,Z,Q,true)))
        {
            break;
        }
        minus = ! (sgn_x ^ sgn_q);
        carry = 0;
        if (BitVector_compute(X3,X1,X3,minus,&carry))
        {
            error = ErrCode_Ovfl;
            break;
        }
        sgn_x = (((*(X3+size) &= mask) & msb) != 0);

        if (sgn_y) BitVector_Negate(Z,Y2); else BitVector_Copy(Z,Y2);
        if ((error = BitVector_Mul_Pos(Y3,Z,Q,true)))
        {
            break;
        }
        minus = ! (sgn_y ^ sgn_q);
        carry = 0;
        if (BitVector_compute(Y3,Y1,Y3,minus,&carry))
        {
            error = ErrCode_Ovfl;
            break;
        }
        sgn_y = (((*(Y3+size) &= mask) & msb) != 0);

        T = A; sgn_r = sgn_a;
        A = B; sgn_a = sgn_b;
        B = R; sgn_b = sgn_r;
        R = T;

        T = X1;
        X1 = X2;
        X2 = X3;
        X3 = T;

        T = Y1;
        Y1 = Y2;
        Y2 = Y3;
        Y3 = T;
    }
    if (! error)
    {
        if (sgn_b) BitVector_Negate(U,B); else BitVector_Copy(U,B);
        BitVector_Copy(V,X2);
        BitVector_Copy(W,Y2);
    }
    BitVector_Destroy_List(L,11);
    return(error);
}

ErrCode BitVector_Power(unsigned int *  X, unsigned int *  Y, unsigned int *  Z)
{
    ErrCode error = ErrCode_Ok;
    unsigned int  bits  = bits_(X);
    boolean first = true;
    signed long  last;
    unsigned int  limit;
    unsigned int  count;
    unsigned int *  T;

    /*
       Requirements:
         -  X must have at least the same size as Y but may be larger (!)
         -  X may not be identical with Z
         -  Z must be positive
       Features:
         -  The contents of Y && Z are preserved
    */

    if (X == Z) return(ErrCode_Same);
    if (bits < bits_(Y)) return(ErrCode_Size);
    if (BitVector_msb_(Z)) return(ErrCode_Expo);
    if ((last = Set_Max(Z)) < 0L)
    {
        if (bits < 2) return(ErrCode_Ovfl);
        BitVector_Empty(X);
        *X |= LSB;
        return(ErrCode_Ok);                             /* anything ^ 0 == 1 */
    }
    if (BitVector_is_empty(Y))
    {
        if (X != Y) BitVector_Empty(X);
        return(ErrCode_Ok);                    /* 0 ^ anything ! zero == 0 */
    }
    T = BitVector_Create(bits,false);
    if (T == NULL) return(ErrCode_Null);
    limit = (unsigned int) last;
    for ( count = 0; ((!error) && (count <= limit)); count++ )
    {
        if ( BIT_VECTOR_TST_BIT(Z,count) )
        {
            if (first)
            {
                first = false;
                if (count) {             BitVector_Copy(X,T); }
                else       { if (X != Y) BitVector_Copy(X,Y); }
            }
            else error = BitVector_Multiply(X,T,X); /* order important because T > X */
        }
        if ((!error) && (count < limit))
        {
            if (count) error = BitVector_Multiply(T,T,T);
            else       error = BitVector_Multiply(T,Y,Y);
        }
    }
    BitVector_Destroy(T);
    return(error);
}

void BitVector_Block_Store(unsigned int *  addr, unsigned char * buffer, unsigned int length)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int  value;
    unsigned int  count;

    /* provide translation for independence of endian-ness: */
    if (size > 0)
    {
        while (size-- > 0)
        {
            value = 0;
            for ( count = 0; (length > 0) && (count < BITS); count += 8 )
            {
                value |= (((unsigned int) *buffer++) << count); length--;
            }
            *addr++ = value;
        }
        *(--addr) &= mask;
    }
}

unsigned char * BitVector_Block_Read(unsigned int *  addr, unsigned int * length)
{
    unsigned int  size = size_(addr);
    unsigned int  value;
    unsigned int  count;
    unsigned char * buffer;
    unsigned char * target;

    /* provide translation for independence of endian-ness: */
    *length = size << FACTOR;
    buffer = (unsigned char *) malloc((size_t) ((*length)+1));
    if (buffer == NULL) return(NULL);
    target = buffer;
    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while (size-- > 0)
        {
            value = *addr++;
            count = BITS >> 3;
            while (count-- > 0)
            {
                *target++ = (unsigned char) (value & 0x00FF);
                if (count > 0) value >>= 8;
            }
        }
    }
    *target = (unsigned char) '\0';
    return(buffer);
}

void BitVector_Word_Store(unsigned int *  addr, unsigned int offset, unsigned int value)
{
    unsigned int size = size_(addr);

    if (size > 0)
    {
        if (offset < size) *(addr+offset) = value;
        *(addr+size-1) &= mask_(addr);
    }
}

unsigned int BitVector_Word_Read(unsigned int *  addr, unsigned int offset)
{
    unsigned int size = size_(addr);

    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        if (offset < size) return( *(addr+offset) );
    }
    return( (unsigned int) 0 );
}

void BitVector_Word_Insert(unsigned int *  addr, unsigned int offset, unsigned int count,
                           boolean clear)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int *  last = addr+size-1;

    if (size > 0)
    {
        *last &= mask;
        if (offset > size) offset = size;
        BIT_VECTOR_ins_words(addr+offset,size-offset,count,clear);
        *last &= mask;
    }
}

void BitVector_Word_Delete(unsigned int *  addr, unsigned int offset, unsigned int count,
                           boolean clear)
{
    unsigned int  size = size_(addr);
    unsigned int  mask = mask_(addr);
    unsigned int *  last = addr+size-1;

    if (size > 0)
    {
        *last &= mask;
        if (offset > size) offset = size;
        BIT_VECTOR_del_words(addr+offset,size-offset,count,clear);
        *last &= mask;
    }
}

void BitVector_Chunk_Store(unsigned int *  addr, unsigned int chunksize, unsigned int offset,
                           unsigned long value)
{
    unsigned int bits = bits_(addr);
    unsigned int mask;
    unsigned int temp;

    if ((chunksize > 0) && (offset < bits))
    {
        if (chunksize > LONGBITS) chunksize = LONGBITS;
        if ((offset + chunksize) > bits) chunksize = bits - offset;
        addr += offset >> LOGBITS;
        offset &= MODMASK;
        while (chunksize > 0)
        {
            mask = (unsigned int) (~0L << offset);
            bits = offset + chunksize;
            if (bits < BITS)
            {
                mask &= (unsigned int) ~(~0L << bits);
                bits = chunksize;
            }
            else bits = BITS - offset;
            temp = (unsigned int) (value << offset);
            temp &= mask;
            *addr &= ~ mask;
            *addr++ |= temp;
            value >>= bits;
            chunksize -= bits;
            offset = 0;
        }
    }
}

unsigned long BitVector_Chunk_Read(unsigned int *  addr, unsigned int chunksize, unsigned int offset)
{
    unsigned int bits = bits_(addr);
    unsigned int chunkbits = 0;
    unsigned long value = 0L;
    unsigned long temp;
    unsigned int mask;

    if ((chunksize > 0) && (offset < bits))
    {
        if (chunksize > LONGBITS) chunksize = LONGBITS;
        if ((offset + chunksize) > bits) chunksize = bits - offset;
        addr += offset >> LOGBITS;
        offset &= MODMASK;
        while (chunksize > 0)
        {
            bits = offset + chunksize;
            if (bits < BITS)
            {
                mask = (unsigned int) ~(~0L << bits);
                bits = chunksize;
            }
            else
            {
                mask = (unsigned int) ~0L;
                bits = BITS - offset;
            }
            temp = (unsigned long) ((*addr++ & mask) >> offset);
            value |= temp << chunkbits;
            chunkbits += bits;
            chunksize -= bits;
            offset = 0;
        }
    }
    return(value);
}

    /*******************/
    /* set operations: */
    /*******************/

void Set_Union(unsigned int *  X, unsigned int *  Y, unsigned int *  Z)             /* X = Y + Z     */
{
    unsigned int bits = bits_(X);
    unsigned int size = size_(X);
    unsigned int mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ | *Z++;
        *(--X) &= mask;
    }
}

void Set_Intersection(unsigned int *  X, unsigned int *  Y, unsigned int *  Z)      /* X = Y * Z     */
{
    unsigned int bits = bits_(X);
    unsigned int size = size_(X);
    unsigned int mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ & *Z++;
        *(--X) &= mask;
    }
}

void Set_Difference(unsigned int *  X, unsigned int *  Y, unsigned int *  Z)        /* X = Y \ Z     */
{
    unsigned int bits = bits_(X);
    unsigned int size = size_(X);
    unsigned int mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ & ~ *Z++;
        *(--X) &= mask;
    }
}

void Set_ExclusiveOr(unsigned int *  X, unsigned int *  Y, unsigned int *  Z)       /* X=(Y+Z)\(Y*Z) */
{
    unsigned int bits = bits_(X);
    unsigned int size = size_(X);
    unsigned int mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ ^ *Z++;
        *(--X) &= mask;
    }
}

void Set_Complement(unsigned int *  X, unsigned int *  Y)                   /* X = ~Y        */
{
    unsigned int size = size_(X);
    unsigned int mask = mask_(X);

    if ((size > 0) && (bits_(X) == bits_(Y)))
    {
        while (size-- > 0) *X++ = ~ *Y++;
        *(--X) &= mask;
    }
}

    /******************/
    /* set functions: */
    /******************/

boolean Set_subset(unsigned int *  X, unsigned int *  Y)                    /* X subset Y ?  */
{
    unsigned int size = size_(X);
    boolean r = false;

    if ((size > 0) && (bits_(X) == bits_(Y)))
    {
        r = true;
        while (r && (size-- > 0)) r = ((*X++ & ~ *Y++) == 0);
    }
    return(r);
}

unsigned int Set_Norm(unsigned int *  addr)                                /* = | X |       */
{
    unsigned char * byte;
    unsigned int  bytes;
    unsigned int   n;

    byte = (unsigned char *) addr;
    bytes = size_(addr) << FACTOR;
    n = 0;
    while (bytes-- > 0)
    {
        n += BitVector_BYTENORM[*byte++];
    }
    return(n);
}

unsigned int Set_Norm2(unsigned int *  addr)                               /* = | X |       */
{
    unsigned int  size = size_(addr);
    unsigned int  w0,w1;
    unsigned int   n,k;

    n = 0;
    while (size-- > 0)
    {
        k = 0;
        w1 = ~ (w0 = *addr++);
        while (w0 && w1)
        {
            w0 &= w0 - 1;
            w1 &= w1 - 1;
            k++;
        }
        if (w0 == 0) n += k;
        else         n += BITS - k;
    }
    return(n);
}

unsigned int Set_Norm3(unsigned int *  addr)                               /* = | X |       */
{
    unsigned int  size  = size_(addr);
    unsigned int   count = 0;
    unsigned int  c;

    while (size-- > 0)
    {
        c = *addr++;
        while (c)
        {
            c &= c - 1;
            count++;
        }
    }
    return(count);
}

signed long Set_Min(unsigned int *  addr)                                /* = min(X)      */
{
    boolean empty = true;
    unsigned int  size  = size_(addr);
    unsigned int  i     = 0;
    unsigned int  c     = 0;         /* silence compiler warning */

    while (empty && (size-- > 0))
    {
        if ((c = *addr++)) empty = false; else i++;
    }
    if (empty) return((signed long) LONG_MAX);                  /* plus infinity  */
    i <<= LOGBITS;
    while (! (c & LSB))
    {
        c >>= 1;
        i++;
    }
    return((signed long) i);
}

signed long Set_Max(unsigned int *  addr)                                /* = max(X)      */
{
    boolean empty = true;
    unsigned int  size  = size_(addr);
    unsigned int  i     = size;
    unsigned int  c     = 0;         /* silence compiler warning */

    addr += size-1;
    while (empty && (size-- > 0))
    {
        if ((c = *addr--)) empty = false; else i--;
    }
    if (empty) return((signed long) LONG_MIN);                  /* minus infinity */
    i <<= LOGBITS;
    while (! (c & MSB))
    {
        c <<= 1;
        i--;
    }
    return((signed long) --i);
}

    /**********************************/
    /* matrix-of-booleans operations: */
    /**********************************/

void Matrix_Multiplication(unsigned int *  X, unsigned int rowsX, unsigned int colsX,
                           unsigned int *  Y, unsigned int rowsY, unsigned int colsY,
                           unsigned int *  Z, unsigned int rowsZ, unsigned int colsZ)
{
    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int indxX;
    unsigned int indxY;
    unsigned int indxZ;
    unsigned int termX;
    unsigned int termY;
    unsigned int sum;

  if ((colsY == rowsZ) && (rowsX == rowsY) && (colsX == colsZ) &&
      (bits_(X) == rowsX*colsX) &&
      (bits_(Y) == rowsY*colsY) &&
      (bits_(Z) == rowsZ*colsZ))
  {
    for ( i = 0; i < rowsY; i++ )
    {
        termX = i * colsX;
        termY = i * colsY;
        for ( j = 0; j < colsZ; j++ )
        {
            indxX = termX + j;
            sum = 0;
            for ( k = 0; k < colsY; k++ )
            {
                indxY = termY + k;
                indxZ = k * colsZ + j;
                if ( BIT_VECTOR_TST_BIT(Y,indxY) &
                     BIT_VECTOR_TST_BIT(Z,indxZ) ) sum ^= 1;
            }
            if (sum) BIT_VECTOR_SET_BIT(X,indxX)
            else     BIT_VECTOR_CLR_BIT(X,indxX)
        }
    }
  }
}

void Matrix_Product(unsigned int *  X, unsigned int rowsX, unsigned int colsX,
                    unsigned int *  Y, unsigned int rowsY, unsigned int colsY,
                    unsigned int *  Z, unsigned int rowsZ, unsigned int colsZ)
{
    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int indxX;
    unsigned int indxY;
    unsigned int indxZ;
    unsigned int termX;
    unsigned int termY;
    unsigned int sum;

  if ((colsY == rowsZ) && (rowsX == rowsY) && (colsX == colsZ) &&
      (bits_(X) == rowsX*colsX) &&
      (bits_(Y) == rowsY*colsY) &&
      (bits_(Z) == rowsZ*colsZ))
  {
    for ( i = 0; i < rowsY; i++ )
    {
        termX = i * colsX;
        termY = i * colsY;
        for ( j = 0; j < colsZ; j++ )
        {
            indxX = termX + j;
            sum = 0;
            for ( k = 0; k < colsY; k++ )
            {
                indxY = termY + k;
                indxZ = k * colsZ + j;
                if ( BIT_VECTOR_TST_BIT(Y,indxY) &
                     BIT_VECTOR_TST_BIT(Z,indxZ) ) sum |= 1;
            }
            if (sum) BIT_VECTOR_SET_BIT(X,indxX)
            else     BIT_VECTOR_CLR_BIT(X,indxX)
        }
    }
  }
}

void Matrix_Closure(unsigned int *  addr, unsigned int rows, unsigned int cols)
{
    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int ii;
    unsigned int ij;
    unsigned int ik;
    unsigned int kj;
    unsigned int termi;
    unsigned int termk;

  if ((rows == cols) && (bits_(addr) == rows*cols))
  {
    for ( i = 0; i < rows; i++ )
    {
        ii = i * cols + i;
        BIT_VECTOR_SET_BIT(addr,ii)
    }
    for ( k = 0; k < rows; k++ )
    {
        termk = k * cols;
        for ( i = 0; i < rows; i++ )
        {
            termi = i * cols;
            ik = termi + k;
            for ( j = 0; j < rows; j++ )
            {
                ij = termi + j;
                kj = termk + j;
                if ( BIT_VECTOR_TST_BIT(addr,ik) &
                     BIT_VECTOR_TST_BIT(addr,kj) )
                     BIT_VECTOR_SET_BIT(addr,ij)
            }
        }
    }
  }
}

void Matrix_Transpose(unsigned int *  X, unsigned int rowsX, unsigned int colsX,
                      unsigned int *  Y, unsigned int rowsY, unsigned int colsY)
{
    unsigned int  i;
    unsigned int  j;
    unsigned int  ii;
    unsigned int  ij;
    unsigned int  ji;
    unsigned int  addii;
    unsigned int  addij;
    unsigned int  addji;
    unsigned int  bitii;
    unsigned int  bitij;
    unsigned int  bitji;
    unsigned int  termi;
    unsigned int  termj;
    boolean swap;

  /* BEWARE that "in-place" is ONLY possible if the matrix is quadratic!! */

  if ((rowsX == colsY) && (colsX == rowsY) &&
      (bits_(X) == rowsX*colsX) &&
      (bits_(Y) == rowsY*colsY))
  {
    if (rowsY == colsY) /* in-place is possible! */
    {
        for ( i = 0; i < rowsY; i++ )
        {
            termi = i * colsY;
            for ( j = 0; j < i; j++ )
            {
                termj = j * colsX;
                ij = termi + j;
                ji = termj + i;
                addij = ij >> LOGBITS;
                addji = ji >> LOGBITS;
                bitij = BITMASKTAB[ij & MODMASK];
                bitji = BITMASKTAB[ji & MODMASK];
                swap = ((*(Y+addij) & bitij) != 0);
                if ((*(Y+addji) & bitji) != 0)
                     *(X+addij) |=     bitij;
                else
                     *(X+addij) &= ~ bitij;
                if (swap)
                     *(X+addji) |=     bitji;
                else
                     *(X+addji) &= ~ bitji;
            }
            ii = termi + i;
            addii = ii >> LOGBITS;
            bitii = BITMASKTAB[ii & MODMASK];
            if ((*(Y+addii) & bitii) != 0)
                 *(X+addii) |=     bitii;
            else
                 *(X+addii) &= ~ bitii;
        }
    }
    else /* rowsX != colsX, in-place is ~ possible! */
    {
        for ( i = 0; i < rowsY; i++ )
        {
            termi = i * colsY;
            for ( j = 0; j < colsY; j++ )
            {
                termj = j * colsX;
                ij = termi + j;
                ji = termj + i;
                addij = ij >> LOGBITS;
                addji = ji >> LOGBITS;
                bitij = BITMASKTAB[ij & MODMASK];
                bitji = BITMASKTAB[ji & MODMASK];
                if ((*(Y+addij) & bitij) != 0)
                     *(X+addji) |=     bitji;
                else
                     *(X+addji) &= ~ bitji;
            }
        }
    }
  }
}
}; //end of namespace CONSTANTBV
