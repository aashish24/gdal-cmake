/******************************************************************************
 * $Id$
 *
 * Project:  CPL - Common Portability Library
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 * Purpose:  
 * Include file providing low level portability services for CPL.  This
 * should be the first include file for any CPL based code.  It provides the
 * following:
 *
 * o Includes some standard system include files, such as stdio, and stdlib.
 *
 * o Defines CPL_C_START, CPL_C_END macros.
 *
 * o Ensures that some other standard macros like NULL are defined.
 *
 * o Defines some portability stuff like CPL_MSB, or CPL_LSB.
 *
 * o Ensures that core types such as GBool, GInt32, GInt16, GUInt32, 
 *   GUInt16, and GByte are defined.
 *
 ******************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.28  2001/08/30 21:20:49  warmerda
 * expand tabs
 *
 * Revision 1.27  2001/07/18 04:00:49  warmerda
 * added CPL_CVSID
 *
 * Revision 1.26  2001/06/21 21:17:26  warmerda
 * added irix 64bit file api support
 *
 * Revision 1.25  2001/04/30 18:18:38  warmerda
 * added macos support, standard header
 *
 * Revision 1.24  2001/01/19 21:16:41  warmerda
 * expanded tabs
 *
 * Revision 1.23  2001/01/13 04:06:39  warmerda
 * added strings.h on AIX as per patch from Dale.
 *
 * Revision 1.22  2001/01/03 16:18:07  warmerda
 * added GUIntBig
 *
 * Revision 1.21  2000/10/20 04:20:33  warmerda
 * added SWAP16PTR macros
 *
 * Revision 1.20  2000/10/13 17:32:42  warmerda
 * check for unix instead of IGNORE_WIN32
 *
 * Revision 1.19  2000/09/25 19:58:43  warmerda
 * ensure win32 doesn't get defined in Cygnus builds
 *
 * Revision 1.18  2000/07/20 13:15:03  warmerda
 * don't redeclare CPL_DLL
 */

#ifndef CPL_BASE_H_INCLUDED
#define CPL_BASE_H_INCLUDED

/**
 * \file cpl_port.h
 *
 * Core portability definitions for CPL.
 *
 */

/* ==================================================================== */
/*      We will use macos_pre10 to indicate compilation with MacOS      */
/*      versions before MacOS X.                                        */
/* ==================================================================== */
#ifdef macintosh
#  define macos_pre10
#endif

/* ==================================================================== */
/*      We will use WIN32 as a standard windows define.                 */
/* ==================================================================== */
#if defined(_WIN32) && !defined(WIN32)
#  define WIN32
#endif

#if defined(_WINDOWS) && !defined(WIN32)
#  define WIN32
#endif

#include "cpl_config.h"

/* ==================================================================== */
/*      This will disable most WIN32 stuff in a Cygnus build which      */
/*      defines unix to 1.                                              */
/* ==================================================================== */

#ifdef unix
#  undef WIN32
#endif

/* ==================================================================== */
/*      Standard include files.                                         */
/* ==================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#ifdef _AIX
#  include <strings.h>
#endif

#if defined(HAVE_LIBDBMALLOC) && defined(HAVE_DBMALLOC_H) && defined(DEBUG)
#  define DBMALLOC
#  include <dbmalloc.h>
#endif

#if !defined(DBMALLOC) && defined(HAVE_DMALLOC_H)
#  define USE_DMALLOC
#  include <dmalloc.h>
#endif

/* ==================================================================== */
/*      Base portability stuff ... this stuff may need to be            */
/*      modified for new platforms.                                     */
/* ==================================================================== */

/*---------------------------------------------------------------------
 *        types for 16 and 32 bits integers, etc...
 *--------------------------------------------------------------------*/
#if UINT_MAX == 65535
typedef long            GInt32;
typedef unsigned long   GUInt32;
#else
typedef int             GInt32;
typedef unsigned int    GUInt32;
#endif

typedef short           GInt16;
typedef unsigned short  GUInt16;
typedef unsigned char   GByte;
typedef int             GBool;

/* -------------------------------------------------------------------- */
/*      64bit support                                                   */
/* -------------------------------------------------------------------- */

#ifdef WIN32

#define VSI_LARGE_API_SUPPORTED
typedef __int64          GIntBig;
typedef unsigned __int64 GUIntBig;

#elif HAVE_LONG_LONG

typedef long long        GIntBig;
typedef unsigned long long GUIntBig;

#else

typedef long             GIntBig;
typedef unsigned long    GUIntBig;

#endif

/* ==================================================================== */
/*      Other standard services.                                        */
/* ==================================================================== */
#ifdef __cplusplus
#  define CPL_C_START           extern "C" {
#  define CPL_C_END             }
#else
#  define CPL_C_START
#  define CPL_C_END
#endif

#ifndef CPL_DLL
#if defined(WIN32) && !defined(CPL_DISABLE_DLL)
#  define CPL_DLL     __declspec(dllexport)
#else
#  define CPL_DLL
#endif
#endif


#ifndef NULL
#  define NULL  0
#endif

#ifndef FALSE
#  define FALSE 0
#endif

#ifndef TRUE
#  define TRUE  1
#endif

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

#ifndef ABS
#  define ABS(x)        ((x<0) ? (-1*(x)) : x)
#endif

#ifndef EQUAL
#ifdef WIN32
#  define EQUALN(a,b,n)           (strnicmp(a,b,n)==0)
#  define EQUAL(a,b)              (stricmp(a,b)==0)
#else
#  define EQUALN(a,b,n)           (strncasecmp(a,b,n)==0)
#  define EQUAL(a,b)              (strcasecmp(a,b)==0)
#endif
#endif

#ifdef macos_pre10
int strcasecmp(char * str1, char * str2);
int strncasecmp(char * str1, char * str2, int len);
char * strdup (char *instr);
#endif

/*---------------------------------------------------------------------
 *                         CPL_LSB and CPL_MSB
 * Only one of these 2 macros should be defined and specifies the byte 
 * ordering for the current platform.  
 * This should be defined in the Makefile, but if it is not then
 * the default is CPL_LSB (Intel ordering, LSB first).
 *--------------------------------------------------------------------*/
#if defined(WORDS_BIGENDIAN) && !defined(CPL_MSB) && !defined(CPL_LSB)
#  define CPL_MSB
#endif

#if ! ( defined(CPL_LSB) || defined(CPL_MSB) )
#define CPL_LSB
#endif

/*---------------------------------------------------------------------
 *        Little endian <==> big endian byte swap macros.
 *--------------------------------------------------------------------*/

#define CPL_SWAP16(x) \
        ((GUInt16)( \
            (((GUInt16)(x) & 0x00ffU) << 8) | \
            (((GUInt16)(x) & 0xff00U) >> 8) ))

#define CPL_SWAP16PTR(x) \
{                                                               \
    GByte       byTemp, *pabyData = (GByte *) (x);              \
                                                                \
    byTemp = pabyData[0];                                       \
    pabyData[0] = pabyData[1];                                  \
    pabyData[1] = byTemp;                                       \
}                                                                    
                                                            
#define CPL_SWAP32(x) \
        ((GUInt32)( \
            (((GUInt32)(x) & (GUInt32)0x000000ffUL) << 24) | \
            (((GUInt32)(x) & (GUInt32)0x0000ff00UL) <<  8) | \
            (((GUInt32)(x) & (GUInt32)0x00ff0000UL) >>  8) | \
            (((GUInt32)(x) & (GUInt32)0xff000000UL) >> 24) ))

#define CPL_SWAP32PTR(x) \
{                                                               \
    GByte       byTemp, *pabyData = (GByte *) (x);              \
                                                                \
    byTemp = pabyData[0];                                       \
    pabyData[0] = pabyData[3];                                  \
    pabyData[3] = byTemp;                                       \
    byTemp = pabyData[1];                                       \
    pabyData[1] = pabyData[2];                                  \
    pabyData[2] = byTemp;                                       \
}                                                                    
                                                            
#define CPL_SWAP64PTR(x) \
{                                                               \
    GByte       byTemp, *pabyData = (GByte *) (x);              \
                                                                \
    byTemp = pabyData[0];                                       \
    pabyData[0] = pabyData[7];                                  \
    pabyData[7] = byTemp;                                       \
    byTemp = pabyData[1];                                       \
    pabyData[1] = pabyData[6];                                  \
    pabyData[6] = byTemp;                                       \
    byTemp = pabyData[2];                                       \
    pabyData[2] = pabyData[5];                                  \
    pabyData[5] = byTemp;                                       \
    byTemp = pabyData[3];                                       \
    pabyData[3] = pabyData[4];                                  \
    pabyData[4] = byTemp;                                       \
}                                                                    
                                                            

/* Until we have a safe 64 bits integer data type defined, we'll replace
m * this version of the CPL_SWAP64() macro with a less efficient one.
 */
/*
#define CPL_SWAP64(x) \
        ((uint64)( \
            (uint64)(((uint64)(x) & (uint64)0x00000000000000ffULL) << 56) | \
            (uint64)(((uint64)(x) & (uint64)0x000000000000ff00ULL) << 40) | \
            (uint64)(((uint64)(x) & (uint64)0x0000000000ff0000ULL) << 24) | \
            (uint64)(((uint64)(x) & (uint64)0x00000000ff000000ULL) << 8) | \
            (uint64)(((uint64)(x) & (uint64)0x000000ff00000000ULL) >> 8) | \
            (uint64)(((uint64)(x) & (uint64)0x0000ff0000000000ULL) >> 24) | \
            (uint64)(((uint64)(x) & (uint64)0x00ff000000000000ULL) >> 40) | \
            (uint64)(((uint64)(x) & (uint64)0xff00000000000000ULL) >> 56) ))
*/

#define CPL_SWAPDOUBLE(p) {                             \
        double _tmp = *(double *)(p);                     \
        ((GByte *)(p))[0] = ((GByte *)&_tmp)[7];          \
        ((GByte *)(p))[1] = ((GByte *)&_tmp)[6];          \
        ((GByte *)(p))[2] = ((GByte *)&_tmp)[5];          \
        ((GByte *)(p))[3] = ((GByte *)&_tmp)[4];          \
        ((GByte *)(p))[4] = ((GByte *)&_tmp)[3];          \
        ((GByte *)(p))[5] = ((GByte *)&_tmp)[2];          \
        ((GByte *)(p))[6] = ((GByte *)&_tmp)[1];          \
        ((GByte *)(p))[7] = ((GByte *)&_tmp)[0];          \
}

#ifdef CPL_MSB
#  define CPL_MSBWORD16(x)      (x)
#  define CPL_LSBWORD16(x)      CPL_SWAP16(x)
#  define CPL_MSBWORD32(x)      (x)
#  define CPL_LSBWORD32(x)      CPL_SWAP32(x)
#  define CPL_MSBPTR16(x)       
#  define CPL_LSBPTR16(x)       CPL_SWAP16PTR(x)
#  define CPL_MSBPTR32(x)       
#  define CPL_LSBPTR32(x)       CPL_SWAP32PTR(x)
#  define CPL_MSBPTR64(x)       
#  define CPL_LSBPTR64(x)       CPL_SWAP64PTR(x)
#else
#  define CPL_LSBWORD16(x)      (x)
#  define CPL_MSBWORD16(x)      CPL_SWAP16(x)
#  define CPL_LSBWORD32(x)      (x)
#  define CPL_MSBWORD32(x)      CPL_SWAP32(x)
#  define CPL_LSBPTR16(x)       
#  define CPL_MSBPTR16(x)       CPL_SWAP16PTR(x)
#  define CPL_LSBPTR32(x)       
#  define CPL_MSBPTR32(x)       CPL_SWAP32PTR(x)
#  define CPL_LSBPTR64(x)       
#  define CPL_MSBPTR64(x)       CPL_SWAP64PTR(x)
#endif

/***********************************************************************
 * Define CPL_CVSID() macro.  It can be disabled during a build by
 * defining DISABLE_CPLID in the compiler options.
 *
 * The cvsid_aw() function is just there to prevent reports of cpl_cvsid()
 * being unused.
 */

#ifndef DISABLE_CVSID
#  define CPL_CVSID(string)     static char cpl_cvsid[] = string; \
static char *cvsid_aw() { return( cvsid_aw() ? ((char *) NULL) : cpl_cvsid ); }
#else
#  define CPL_CVSID(string)
#endif

#endif /* ndef CPL_BASE_H_INCLUDED */
