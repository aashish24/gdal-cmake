/******************************************************************************
 * $Id$
 *
 * Project:  CPL - Common Portability Library
 * Purpose:  Convenience functions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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
 * Revision 1.28  2003/09/08 12:54:42  dron
 * Fixed warnings.
 *
 * Revision 1.27  2003/09/08 11:09:53  dron
 * Added CPLPrintDouble() and CPLPrintTime().
 *
 * Revision 1.26  2003/09/07 14:38:43  dron
 * Added CPLPrintString(), CPLPrintStringFill(), CPLPrintInt32(), CPLPrintUIntBig().
 *
 * Revision 1.25  2003/09/03 13:07:26  warmerda
 * Cleaned up CPLScanLong() a bit to avoid warnings, and
 * unnecessary conversion to/from double.
 *
 * Revision 1.24  2003/08/31 14:48:05  dron
 * Added CPLScanLong() and CPLScanDouble().
 *
 * Revision 1.23  2003/08/25 20:01:58  dron
 * Added CPLFGets() helper function.
 *
 * Revision 1.22  2003/05/08 21:51:14  warmerda
 * added CPL{G,S}etConfigOption() usage
 *
 * Revision 1.21  2003/03/05 16:46:54  warmerda
 * Cast strchr() result for Sun (patch from Graeme).
 *
 * Revision 1.20  2003/03/02 04:44:38  warmerda
 * added CPLStringToComplex
 *
 * Revision 1.19  2003/02/14 22:12:07  warmerda
 * expand tabs
 *
 * Revision 1.18  2002/12/18 20:22:53  warmerda
 * fiddle with roundoff issues in DecToDMS
 *
 * Revision 1.17  2002/12/10 19:46:04  warmerda
 * modified CPLReadLine() to seek back if it overreads past a CR or LF
 *
 * Revision 1.16  2002/12/09 18:52:51  warmerda
 * added DMS conversion
 *
 * Revision 1.15  2002/03/05 14:26:57  warmerda
 * expanded tabs
 *
 * Revision 1.14  2001/12/12 17:06:57  warmerda
 * added CPLStat
 *
 * Revision 1.13  2001/07/18 04:00:49  warmerda
 * added CPL_CVSID
 *
 * Revision 1.12  2001/03/09 03:19:24  danmo
 * Set pszRLBuffer=NULL after freeing it to avoid reallocating an invalid ptr
 *
 * Revision 1.11  2001/03/05 03:37:19  warmerda
 * Improve support for recovering CPLReadLine() working buffer.
 *
 * Revision 1.10  2001/01/19 21:16:41  warmerda
 * expanded tabs
 *
 * Revision 1.9  2000/04/17 15:56:11  warmerda
 * make configuration tests always happen
 *
 * Revision 1.8  2000/04/05 21:02:47  warmerda
 * Added CPLVerifyConfiguration()
 *
 * Revision 1.7  1999/08/27 12:55:39  danmo
 * Support 0 bytes allocations in CPLRealloc()
 *
 * Revision 1.6  1999/06/25 04:38:03  warmerda
 * Fixed CPLReadLine() to work for long lines.
 *
 * Revision 1.5  1999/05/20 02:54:37  warmerda
 * Added API documentation
 *
 * Revision 1.4  1999/01/02 20:29:53  warmerda
 * Allow zero length allocations
 *
 * Revision 1.3  1998/12/15 19:01:07  warmerda
 * Added CPLReadLine().
 *
 * Revision 1.2  1998/12/03 18:30:04  warmerda
 * Use CPLError() instead of GPSError().
 *
 * Revision 1.1  1998/12/02 19:33:23  warmerda
 * New
 *
 */

#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static char **papszConfigOptions = NULL;

/************************************************************************/
/*                             CPLCalloc()                              */
/************************************************************************/

/**
 * Safe version of calloc().
 *
 * This function is like the C library calloc(), but raises a CE_Fatal
 * error with CPLError() if it fails to allocate the desired memory.  It
 * should be used for small memory allocations that are unlikely to fail
 * and for which the application is unwilling to test for out of memory
 * conditions.  It uses VSICalloc() to get the memory, so any hooking of
 * VSICalloc() will apply to CPLCalloc() as well.  CPLFree() or VSIFree()
 * can be used free memory allocated by CPLCalloc().
 *
 * @param nCount number of objects to allocate.
 * @param nSize size (in bytes) of object to allocate.
 * @return pointer to newly allocated memory, only NULL if nSize * nCount is
 * NULL.
 */

void *CPLCalloc( size_t nCount, size_t nSize )

{
    void        *pReturn;

    if( nSize * nCount == 0 )
        return NULL;
    
    pReturn = VSICalloc( nCount, nSize );
    if( pReturn == NULL )
    {
        CPLError( CE_Fatal, CPLE_OutOfMemory,
                  "CPLCalloc(): Out of memory allocating %d bytes.\n",
                  nSize * nCount );
    }

    return pReturn;
}

/************************************************************************/
/*                             CPLMalloc()                              */
/************************************************************************/

/**
 * Safe version of malloc().
 *
 * This function is like the C library malloc(), but raises a CE_Fatal
 * error with CPLError() if it fails to allocate the desired memory.  It
 * should be used for small memory allocations that are unlikely to fail
 * and for which the application is unwilling to test for out of memory
 * conditions.  It uses VSIMalloc() to get the memory, so any hooking of
 * VSIMalloc() will apply to CPLMalloc() as well.  CPLFree() or VSIFree()
 * can be used free memory allocated by CPLMalloc().
 *
 * @param nSize size (in bytes) of memory block to allocate.
 * @return pointer to newly allocated memory, only NULL if nSize is zero.
 */

void *CPLMalloc( size_t nSize )

{
    void        *pReturn;

    CPLVerifyConfiguration();

    if( nSize == 0 )
        return NULL;
    
    pReturn = VSIMalloc( nSize );
    if( pReturn == NULL )
    {
        CPLError( CE_Fatal, CPLE_OutOfMemory,
                  "CPLMalloc(): Out of memory allocating %d bytes.\n",
                  nSize );
    }

    return pReturn;
}

/************************************************************************/
/*                             CPLRealloc()                             */
/************************************************************************/

/**
 * Safe version of realloc().
 *
 * This function is like the C library realloc(), but raises a CE_Fatal
 * error with CPLError() if it fails to allocate the desired memory.  It
 * should be used for small memory allocations that are unlikely to fail
 * and for which the application is unwilling to test for out of memory
 * conditions.  It uses VSIRealloc() to get the memory, so any hooking of
 * VSIRealloc() will apply to CPLRealloc() as well.  CPLFree() or VSIFree()
 * can be used free memory allocated by CPLRealloc().
 *
 * It is also safe to pass NULL in as the existing memory block for
 * CPLRealloc(), in which case it uses VSIMalloc() to allocate a new block.
 *
 * @param pData existing memory block which should be copied to the new block.
 * @param nNewSize new size (in bytes) of memory block to allocate.
 * @return pointer to allocated memory, only NULL if nNewSize is zero.
 */


void * CPLRealloc( void * pData, size_t nNewSize )

{
    void        *pReturn;

    if ( nNewSize == 0 )
    {
        VSIFree(pData);
        return NULL;
    }

    if( pData == NULL )
        pReturn = VSIMalloc( nNewSize );
    else
        pReturn = VSIRealloc( pData, nNewSize );
    
    if( pReturn == NULL )
    {
        CPLError( CE_Fatal, CPLE_OutOfMemory,
                  "CPLRealloc(): Out of memory allocating %d bytes.\n",
                  nNewSize );
    }

    return pReturn;
}

/************************************************************************/
/*                             CPLStrdup()                              */
/************************************************************************/

/**
 * Safe version of strdup() function.
 *
 * This function is similar to the C library strdup() function, but if
 * the memory allocation fails it will issue a CE_Fatal error with
 * CPLError() instead of returning NULL.  It uses VSIStrdup(), so any
 * hooking of that function will apply to CPLStrdup() as well.  Memory
 * allocated with CPLStrdup() can be freed with CPLFree() or VSIFree().
 *
 * It is also safe to pass a NULL string into CPLStrdup().  CPLStrdup()
 * will allocate and return a zero length string (as opposed to a NULL
 * string).
 *
 * @param pszString input string to be duplicated.  May be NULL.
 * @return pointer to a newly allocated copy of the string.  Free with
 * CPLFree() or VSIFree().
 */

char *CPLStrdup( const char * pszString )

{
    char        *pszReturn;

    if( pszString == NULL )
        pszString = "";

    pszReturn = VSIStrdup( pszString );
        
    if( pszReturn == NULL )
    {
        CPLError( CE_Fatal, CPLE_OutOfMemory,
                  "CPLStrdup(): Out of memory allocating %d bytes.\n",
                  strlen(pszString) );
        
    }
    
    return( pszReturn );
}

/************************************************************************/
/*                            CPLFGets()                                */
/************************************************************************/

/**
 * Reads in at most one less than nBufferSize characters from the fp
 * stream and stores them into the buffer pointed to by pszBuffer.
 * Reading stops after an EOF or a newline. If a newline is read, it
 * is _not_ stored into the buffer. A '\0' is stored after the last
 * character in the buffer. All three types of newline terminators
 * recognized by the CPLFGets(): single '\r' and '\n' and '\r\n'
 * combination.
 *
 * @param pszBuffer pointer to the targeting character buffer.
 * @param nBufferSize maximum size of the string to read (not including
 * termonating '\0').
 * @param fp file pointer to read from.
 * @return pointer to the pszBuffer containing a string read
 * from the file or NULL if the error or end of file was encountered.
 */

char *CPLFGets( char *pszBuffer, int nBufferSize, FILE * fp )

{
    size_t nActuallyRead, i;

    if ( nBufferSize == 0 || pszBuffer == NULL || fp == NULL )
	return NULL;

    nActuallyRead =
	    VSIFRead( pszBuffer, 1, nBufferSize - 1, fp );
    if ( nActuallyRead == 0 )
	return NULL;

    pszBuffer[nActuallyRead] = '\0';

    for ( i = 0; i < nActuallyRead; i++ )
    {
        if ( pszBuffer[i] == 10 || pszBuffer[i] == 13 )
	{
	    pszBuffer[i] = '\0';
	    if ( pszBuffer[i + 1] == 10 || pszBuffer[i + 1] == 13 )
		i++;
	    VSIFSeek( fp, i + 1 - nActuallyRead, SEEK_CUR );
	    break;
	}
    }

    return pszBuffer;
}

/************************************************************************/
/*                            CPLReadLine()                             */
/************************************************************************/

/**
 * Simplified line reading from text file.
 * 
 * Read a line of text from the given file handle, taking care
 * to capture CR and/or LF and strip off ... equivelent of
 * DKReadLine().  Pointer to an internal buffer is returned.
 * The application shouldn't free it, or depend on it's value
 * past the next call to CPLReadLine().
 * 
 * Note that CPLReadLine() uses VSIFGets(), so any hooking of VSI file
 * services should apply to CPLReadLine() as well.
 *
 * CPLReadLine() maintains an internal buffer, which will appear as a 
 * single block memory leak in some circumstances.  CPLReadLine() may 
 * be called with a NULL FILE * at any time to free this working buffer.
 *
 * @param fp file pointer opened with VSIFOpen().
 *
 * @return pointer to an internal buffer containing a line of text read
 * from the file or NULL if the end of file was encountered.
 */

const char *CPLReadLine( FILE * fp )

{
    static char *pszRLBuffer = NULL;
    static int  nRLBufferSize = 0;
    int         nReadSoFar = 0;

/* -------------------------------------------------------------------- */
/*      Cleanup case.                                                   */
/* -------------------------------------------------------------------- */
    if( fp == NULL )
    {
        CPLFree( pszRLBuffer );
        pszRLBuffer = NULL;
        nRLBufferSize = 0;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Loop reading chunks of the line till we get to the end of       */
/*      the line.                                                       */
/* -------------------------------------------------------------------- */
    do {
/* -------------------------------------------------------------------- */
/*      Grow the working buffer if we have it nearly full.  Fail out    */
/*      of read line if we can't reallocate it big enough (for          */
/*      instance for a _very large_ file with no newlines).             */
/* -------------------------------------------------------------------- */
        if( nRLBufferSize-nReadSoFar < 128 )
        {
            nRLBufferSize = nRLBufferSize*2 + 128;
            pszRLBuffer = (char *) VSIRealloc(pszRLBuffer, nRLBufferSize);
            if( pszRLBuffer == NULL )
            {
                nRLBufferSize = 0;
                return NULL;
            }
        }

/* -------------------------------------------------------------------- */
/*      Do the actual read.                                             */
/* -------------------------------------------------------------------- */
        if( CPLFGets( pszRLBuffer+nReadSoFar, nRLBufferSize-nReadSoFar, fp )
            == NULL )
        {
            CPLFree( pszRLBuffer );
            pszRLBuffer = NULL;
            nRLBufferSize = 0;

            return NULL;
        }

        nReadSoFar = strlen(pszRLBuffer);

    } while( nReadSoFar == nRLBufferSize - 1
             && pszRLBuffer[nRLBufferSize-2] != 13
             && pszRLBuffer[nRLBufferSize-2] != 10 );

    return( pszRLBuffer );
}

/************************************************************************/
/*                             CPLScanLong()                            */
/************************************************************************/

/**
 * Scan up to a maximum number of characters from a string and convert
 * the result to a long.
 *
 * @param pszString String containing characters to be scanned. It may be
 * terminated with a null character.
 *
 * @param nMaxLength The maximum number of character to consider as part
 * of the number. Less characters will be considered if a null character
 * is encountered.
 * 
 * @return Long value, converted from its ASCII form.
 */

long CPLScanLong( char *pszString, int nMaxLength )
{
    char    szTemp[32];

/* -------------------------------------------------------------------- */
/*	Compute string into local buffer, and terminate it.		*/
/* -------------------------------------------------------------------- */
    strncpy( szTemp, pszString, nMaxLength );
    szTemp[nMaxLength] = '\0';

/* -------------------------------------------------------------------- */
/*	Use atol() to fetch out the result                              */
/* -------------------------------------------------------------------- */
    return atol( szTemp );
}

/************************************************************************/
/*                             CPLScanDouble()                          */
/************************************************************************/

/**
 * Scan up to a maximum number of characters from a string and convert
 * the result to a double.
 *
 * @param pszString String containing characters to be scanned. It may be
 * terminated with a null character.
 *
 * @param nMaxLength The maximum number of character to consider as part
 * of the number. Less characters will be considered if a null character
 * is encountered.
 * 
 * @return Double value, converted from its ASCII form.
 */

double CPLScanDouble( char *pszString, int nMaxLength )
{
    char    szTemp[64];
    int     i;
    double  dfValue;

/* -------------------------------------------------------------------- */
/*	Compute string into local buffer, and terminate it.		*/
/* -------------------------------------------------------------------- */
    strncpy( szTemp, pszString, nMaxLength );
    szTemp[nMaxLength] = '\0';

/* -------------------------------------------------------------------- */
/*	Make a pass through converting 'D's to 'E's.			*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < nMaxLength; i++ )
        if ( szTemp[i] == 'd' || szTemp[i] == 'D' )
            szTemp[i] = 'E';

/* -------------------------------------------------------------------- */
/*	Use atof() to fetch out the result                              */
/* -------------------------------------------------------------------- */
    dfValue = atof( szTemp );

    return dfValue;
}

/************************************************************************/
/*                      CPLPrintString()                                */
/************************************************************************/

/**
 * Copy the string pointed to by pszSrc, _not_ including the terminating
 * `\0' character, to the array pointed to by pszDest.
 *
 * @param pszDest Pointer to the destination string buffer. Should be
 * large enough to hold the resulting string.
 *
 * @param pszDest Pointer to the source buffer.
 * 
 * @param nMaxLen Maximum length of the resulting string. If string length
 * is greater than nMaxLen, it will be truncated.
 * 
 * @return Pointer to the destination string pszDest.
 */

char *CPLPrintString( char *pszDest, const char *pszSrc, int nMaxLen )
{
    char        *pszTemp = pszDest;

    if ( !pszDest )
        return NULL;

    if ( !pszSrc )
    {
        memset( pszDest, '\0', nMaxLen );
        return pszDest;
    }

    while ( nMaxLen && *pszSrc )
    {
        *pszTemp++ = *pszSrc++;
        nMaxLen--;
    }

    return pszDest;
}

/************************************************************************/
/*                         CPLPrintStringFill()                         */
/************************************************************************/

/**
 * Copy the string pointed to by pszSrc, _not_ including the terminating
 * `\0' character, to the array pointed to by pszDest. Remainder of the
 * destination string will be filled with space characters. This is only
 * difference from the PrintString().
 *
 * @param pszDest Pointer to the destination string buffer. Should be
 * large enough to hold the resulting string.
 *
 * @param pszDest Pointer to the source buffer.
 * 
 * @param nMaxLen Maximum length of the resulting string. If string length
 * is greater than nMaxLen, it will be truncated.
 * 
 * @return Pointer to the destination string pszDest.
 */

char *CPLPrintStringFill( char *pszDest, const char *pszSrc, int nMaxLen )
{
    char        *pszTemp = pszDest;

    if ( !pszDest )
        return NULL;

    if ( !pszSrc )
    {
        memset( pszDest, '\0', nMaxLen );
        return pszDest;
    }

    while ( nMaxLen && *pszSrc )
    {
        *pszTemp++ = *pszSrc++;
        nMaxLen--;
    }

    if ( nMaxLen )
        memset( pszTemp, ' ', nMaxLen );

    return pszDest;
}

/************************************************************************/
/*                          CPLPrintInt32()                             */
/************************************************************************/

/**
 * Print GInt32 value into specified string buffer. This string will not
 * be NULL-terminated.
 *
 * @param Pointer to the destination string buffer. Should be
 * large enough to hold the resulting string. Note, that the string will
 * not be NULL-terminated, so user should do this himself, if needed.
 *
 * @param iValue Numerical value to print.
 * 
 * @param nMaxLen Maximum length of the resulting string. If string length
 * is greater than nMaxLen, it will be truncated.
 * 
 * @return Pointer to the destination string buffer.
 */

char *CPLPrintInt32( char *pszBuffer, GInt32 iValue, int nMaxLen )
{
    char        szTemp[64];

    if ( !pszBuffer )
        return NULL;

    if ( nMaxLen >= 64 )
        nMaxLen = 63;

#if UINT_MAX == 65535
    sprintf( szTemp, "%*ld", nMaxLen, iValue );
#else
    sprintf( szTemp, "%*d", nMaxLen, iValue );
#endif

    return CPLPrintString( pszBuffer, szTemp, nMaxLen );
}

/************************************************************************/
/*                          CPLPrintUIntBig()                           */
/************************************************************************/

/**
 * Print GUIntBig value into specified string buffer. This string will not
 * be NULL-terminated.
 *
 * @param Pointer to the destination string buffer. Should be
 * large enough to hold the resulting string. Note, that the string will
 * not be NULL-terminated, so user should do this himself, if needed.
 *
 * @param iValue Numerical value to print.
 * 
 * @param nMaxLen Maximum length of the resulting string. If string length
 * is greater than nMaxLen, it will be truncated.
 * 
 * @return Pointer to the destination string buffer.
 */

char *CPLPrintUIntBig( char *pszBuffer, GUIntBig iValue, int nMaxLen )
{
    char        szTemp[64];

    if ( !pszBuffer )
        return NULL;

    if ( nMaxLen >= 64 )
        nMaxLen = 63;

#if defined(WIN32) && defined(_MSC_VER)
    sprintf( szTemp, "%*I64d", nMaxLen, iValue );
#elif HAVE_LONG_LONG
    sprintf( szTemp, "%*Ld", nMaxLen, iValue );
#else
    sprintf( szTemp, "%*ld", nMaxLen, iValue );
#endif

    return CPLPrintString( pszBuffer, szTemp, nMaxLen );
}

/************************************************************************/
/*                          CPLPrintDouble()                            */
/************************************************************************/

/**
 * Print double value into specified string buffer. Exponential character
 * flag 'E' (or 'e') will be replaced with 'D', as in Fortran. Resulting
 * string will not to be NULL-terminated.
 *
 * @param Pointer to the destination string buffer. Should be
 * large enough to hold the resulting string. Note, that the string will
 * not be NULL-terminated, so user should do this himself, if needed.
 *
 * @param Format specifier (for example, "%16.9E").
 *
 * @param dfValue Numerical value to print.
 * 
 * @return Pointer to the destination string buffer.
 */

char *CPLPrintDouble( char *pszBuffer, const char *pszFormat,
                      double dfValue )
{
    char        szTemp[64];
    int         i;

    if ( !pszBuffer )
        return NULL;

    sprintf( szTemp, pszFormat, dfValue );

    for( i = 0; szTemp[i] != '\0'; i++ )
    {
        if( szTemp[i] == 'E' || szTemp[i] == 'e' )
            szTemp[i] = 'D';
    }

    return CPLPrintString( pszBuffer, szTemp, 64 );
}

/************************************************************************/
/*                            CPLPrintTime()                            */
/************************************************************************/

/**
 * Print specified time value accordingly to the format options and
 * specified locale name. This function does following:
 * 
 *  - if locale parameter is not NULL, the current locale setting will be
 *  stored and replaced with the specified one;
 *  - format time value with the strftime(3) function;
 *  - restore back current locale, if was saved.
 * 
 * @param pszBuffer Pointer to the destination string buffer. Should be
 * large enough to hold the resulting string. Note, that the string will
 * not be NULL-terminated, so user should do this himself, if needed.
 *
 * @param nMaxLen Maximum length of the resulting string. If string length is
 * greater than nMaxLen, it will be truncated.
 * 
 * @param pszFormat Controls the output format. Options are the same as
 * for strftime(3) function.
 *
 * @param poBrokenTime Pointer to the broken-down time structure. May be
 * requested with the VSIGMTime() and VSILocalTime() functions.
 *
 * @param pszLocale Pointer to a character string containing locale name
 * ("C", "POSIX", "us_US", "ru_RU.KOI8-R" etc.).
 *
 * @return Pointer to the destination not NULL terminated buffer.
 */

char *CPLPrintTime( char *pszBuffer, int nMaxLen, const char *pszFormat,
                    const struct tm *poBrokenTime, char *pszLocale )
{
    char        *pszTemp = (char *)CPLMalloc( (nMaxLen + 1) * sizeof(char) );

#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
    char        *pszCurLocale = NULL;

    if ( pszLocale || EQUAL( pszLocale, "" ) )
    {
        // Save the current locale
        pszCurLocale = setlocale(LC_ALL, NULL );
        // Set locale to the specified value
        setlocale(LC_ALL, pszLocale );
    }
#endif
    
    if ( !strftime( pszTemp, nMaxLen + 1, pszFormat, poBrokenTime ) )
        memset( pszTemp, 0, nMaxLen + 1);

#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
    // Restore stored locale back
    if ( pszCurLocale )
        setlocale(LC_ALL, pszCurLocale );
#endif

    CPLPrintString( pszBuffer, pszTemp, nMaxLen );

    CPLFree( pszTemp );

    return pszBuffer;
}

/************************************************************************/
/*                       CPLVerifyConfiguration()                       */
/************************************************************************/

void CPLVerifyConfiguration()

{
/* -------------------------------------------------------------------- */
/*      Verify data types.                                              */
/* -------------------------------------------------------------------- */
    CPLAssert( sizeof(GInt32) == 4 );
    CPLAssert( sizeof(GInt16) == 2 );
    CPLAssert( sizeof(GByte) == 1 );

    if( sizeof(GInt32) != 4 )
        CPLError( CE_Fatal, CPLE_AppDefined, 
                  "sizeof(GInt32) == %d ... yow!\n", 
                  sizeof(GInt32) );

/* -------------------------------------------------------------------- */
/*      Verify byte order                                               */
/* -------------------------------------------------------------------- */
    GInt32   nTest;

    nTest = 1;

#ifdef CPL_LSB
    if( ((GByte *) &nTest)[0] != 1 )
#endif
#ifdef CPL_MSB
    if( ((GByte *) &nTest)[3] != 1 )
#endif    
        CPLError( CE_Fatal, CPLE_AppDefined, 
                  "CPLVerifyConfiguration(): byte order set wrong.\n" );
}

/************************************************************************/
/*                         CPLGetConfigOption()                         */
/************************************************************************/

const char *CPLGetConfigOption( const char *pszKey, const char *pszDefault )

{
    const char *pszResult = CSLFetchNameValue( papszConfigOptions, pszKey );

    if( pszResult == NULL )
        pszResult = getenv( pszKey );

    if( pszResult == NULL )
        return pszDefault;
    else
        return pszResult;
}

/************************************************************************/
/*                         CPLSetConfigOption()                         */
/************************************************************************/

void CPLSetConfigOption( const char *pszKey, const char *pszValue )

{
    papszConfigOptions = 
        CSLSetNameValue( papszConfigOptions, pszKey, pszValue );
}

/************************************************************************/
/*                              CPLStat()                               */
/*                                                                      */
/*      Same as VSIStat() except it works on "C:" as if it were         */
/*      "C:\".                                                          */
/************************************************************************/

int CPLStat( const char *pszPath, VSIStatBuf *psStatBuf )

{
    if( strlen(pszPath) == 2 && pszPath[1] == ':' )
    {
        char    szAltPath[10];
        
        strcpy( szAltPath, pszPath );
        strcat( szAltPath, "\\" );
        return VSIStat( szAltPath, psStatBuf );
    }
    else
        return VSIStat( pszPath, psStatBuf );
}

/************************************************************************/
/*                            proj_strtod()                             */
/************************************************************************/
static double
proj_strtod(char *nptr, char **endptr) 

{
    char c, *cp = nptr;
    double result;

    /*
     * Scan for characters which cause problems with VC++ strtod()
     */
    while ((c = *cp) != '\0') {
        if (c == 'd' || c == 'D') {

            /*
             * Found one, so NUL it out, call strtod(),
             * then restore it and return
             */
            *cp = '\0';
            result = strtod(nptr, endptr);
            *cp = c;
            return result;
        }
        ++cp;
    }

    /* no offending characters, just handle normally */

    return strtod(nptr, endptr);
}

/************************************************************************/
/*                            CPLDMSToDec()                             */
/************************************************************************/

static const char*sym = "NnEeSsWw";
static const double vm[] = { 1.0, 0.0166666666667, 0.00027777778 };

double CPLDMSToDec( const char *is )

{
    int sign, n, nl;
    char *p, *s, work[64];
    double v, tv;

    /* copy sting into work space */
    while (isspace(sign = *is)) ++is;
    for (n = sizeof(work), s = work, p = (char *)is; isgraph(*p) && --n ; )
        *s++ = *p++;
    *s = '\0';
    /* it is possible that a really odd input (like lots of leading
       zeros) could be truncated in copying into work.  But ... */
    sign = *(s = work);
    if (sign == '+' || sign == '-') s++;
    else sign = '+';
    for (v = 0., nl = 0 ; nl < 3 ; nl = n + 1 ) {
        if (!(isdigit(*s) || *s == '.')) break;
        if ((tv = proj_strtod(s, &s)) == HUGE_VAL)
            return tv;
        switch (*s) {
          case 'D': case 'd':
            n = 0; break;
          case '\'':
            n = 1; break;
          case '"':
            n = 2; break;
          case 'r': case 'R':
            if (nl) {
                return 0.0;
            }
            ++s;
            v = tv;
            goto skip;
          default:
            v += tv * vm[nl];
          skip: n = 4;
            continue;
        }
        if (n < nl) {
            return 0.0;
        }
        v += tv * vm[n];
        ++s;
    }
    /* postfix sign */
    if (*s && (p = (char *) strchr(sym, *s))) {
        sign = (p - sym) >= 4 ? '-' : '+';
        ++s;
    }
    if (sign == '-')
        v = -v;

    return v;
}


/************************************************************************/
/*                            CPLDecToDMS()                             */
/*                                                                      */
/*      Translate a decimal degrees value to a DMS string with          */
/*      hemisphere.                                                     */
/************************************************************************/

const char *CPLDecToDMS( double dfAngle, const char * pszAxis,
                         int nPrecision )

{
    int         nDegrees, nMinutes;
    double      dfSeconds, dfABSAngle, dfEpsilon;
    char        szFormat[30];
    static char szBuffer[50];
    const char  *pszHemisphere;
    
    dfEpsilon = (0.5/3600.0) * pow(0.1,nPrecision);

    dfABSAngle = ABS(dfAngle) + dfEpsilon;

    nDegrees = (int) dfABSAngle;
    nMinutes = (int) ((dfABSAngle - nDegrees) * 60);
    dfSeconds = dfABSAngle * 3600 - nDegrees*3600 - nMinutes*60;

    if( dfSeconds > dfEpsilon * 3600.0 )
        dfSeconds -= dfEpsilon * 3600.0;

    if( EQUAL(pszAxis,"Long") && dfAngle < 0.0 )
        pszHemisphere = "W";
    else if( EQUAL(pszAxis,"Long") )
        pszHemisphere = "E";
    else if( dfAngle < 0.0 )
        pszHemisphere = "S";
    else
        pszHemisphere = "N";

    sprintf( szFormat, "%%3dd%%2d\'%%.%df\"%s", nPrecision, pszHemisphere );
    sprintf( szBuffer, szFormat, nDegrees, nMinutes, dfSeconds );

    return( szBuffer );
}

/************************************************************************/
/*                         CPLStringToComplex()                         */
/************************************************************************/

void CPL_DLL CPLStringToComplex( const char *pszString, 
                                 double *pdfReal, double *pdfImag )

{
    int  i;
    int  iPlus = -1, iImagEnd = -1;

    while( *pszString == ' ' )
        pszString++;

    *pdfReal = atof(pszString);
    *pdfImag = 0.0;

    for( i = 0; pszString[i] != '\0' && pszString[i] != ' ' && i < 100; i++ )
    {
        if( pszString[i] == '+' && i > 0 )
            iPlus = i;
        if( pszString[i] == '-' && i > 0 )
            iPlus = i;
        if( pszString[i] == 'i' )
            iImagEnd = i;
    }

    if( iPlus > -1 && iImagEnd > -1 && iPlus < iImagEnd )
    {
        *pdfImag = atof(pszString + iPlus);
    }

    return;
}
