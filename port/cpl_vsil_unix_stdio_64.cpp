/**********************************************************************
 * $Id$
 *
 * Project:  CPL - Common Portability Library
 * Purpose:  Implement VSI large file api for Unix platforms with fseek64()
 *           and ftell64() such as IRIX. 
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 **********************************************************************
 * Copyright (c) 2001, Frank Warmerdam
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log$
 * Revision 1.1  2001/06/21 21:17:15  warmerda
 * New
 *
 */

#include "cpl_vsi.h"

#if defined(UNIX_STDIO_64)

/************************************************************************/
/*                              VSIFOpen()                              */
/************************************************************************/

FILE *VSIFOpenL( const char * pszFilename, const char * pszAccess )

{
    return VSIFOpen( pszFilename, pszAccess );
}

/************************************************************************/
/*                             VSIFCloseL()                             */
/************************************************************************/

int VSIFCloseL( FILE * fp )

{
    return VSIFClose( fp );
}

/************************************************************************/
/*                             VSIFSeekL()                              */
/************************************************************************/

int VSIFSeekL( FILE * fp, vsi_l_offset nOffset, int nWhence )

{
    return( fseek64( fp, nOffset, nWhence ) );
}

/************************************************************************/
/*                             VSIFTellL()                              */
/************************************************************************/

vsi_l_offset VSIFTellL( FILE * fp )

{
    return( ftell64( fp ) );
}

/************************************************************************/
/*                             VSIRewindL()                             */
/************************************************************************/

void VSIRewindL( FILE * fp )

{
    VSIFRewind( fp );
}

/************************************************************************/
/*                             VSIFFlushL()                             */
/************************************************************************/

void VSIFFlushL( FILE * fp )

{
    VSIFFlush( fp );
}

/************************************************************************/
/*                             VSIFReadL()                              */
/************************************************************************/

size_t VSIFReadL( void * pBuffer, size_t nSize, size_t nCount, FILE * fp )

{
    return VSIFRead( pBuffer, nSize, nCount, fp );
}

/************************************************************************/
/*                             VSIFWriteL()                             */
/************************************************************************/

size_t VSIFWriteL( void * pBuffer, size_t nSize, size_t nCount, FILE * fp )

{
    return VSIFWrite( pBuffer, nSize, nCount, fp );
}

/************************************************************************/
/*                              VSIFEofL()                              */
/************************************************************************/

int VSIFEofL( FILE * fp )

{
    return VSIFEof( fp );
}

#endif /* defined UNIX_STDIO_64 */

