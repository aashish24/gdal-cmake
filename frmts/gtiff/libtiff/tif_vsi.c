/******************************************************************************
 * $Id$
 *
 * Project:  GeoTIFF Driver
 * Purpose:  Implement system hook functions for libtiff on top of CPL/VSI,
 *           including > 2GB support.  Based on tif_unix.c from libtiff
 *           distribution.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam, warmerdam@pobox.com
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
 * Revision 1.1  2001/01/03 17:03:41  warmerda
 * *** empty log message ***
 *
 */

/*
 * TIFF Library UNIX-specific Routines.
 */
#include "tiffiop.h"
#include "cpl_vsi.h"

static tsize_t
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    return VSIFReadL( buf, 1, size, (FILE *) fd );
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    return VSIFWriteL( buf, 1, size, (FILE *) fd );
}

static toff_t
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
    if( VSIFSeekL( (FILE *) fd, off, whence ) == 0 )
        return VSIFTellL( (FILE *) fd );
    else
        return off - 1;
}

static int
_tiffCloseProc(thandle_t fd)
{
    return VSIFCloseL( (FILE *) fd );
}

static toff_t
_tiffSizeProc(thandle_t fd)
{
#ifdef _AM29K
	long fsize;
	return ((fsize = lseek((int) fd, 0, SEEK_END)) < 0 ? 0 : fsize);
#else
#if USE_64BIT_API == 1
	struct stat64 sb;
	return (toff_t) (fstat64((int) fd, &sb) < 0 ? 0 : sb.st_size);
#else
	struct stat sb;
	return (toff_t) (fstat((int) fd, &sb) < 0 ? 0 : sb.st_size);
#endif
#endif
}

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	(void) fd; (void) pbase; (void) psize;
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	(void) fd; (void) base; (void) size;
}

/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF*
TIFFFdOpen(int fd, const char* name, const char* mode)
{
	return NULL;
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	int           i, a_out;
        char          access[32];
        FILE          *fp;
        TIFF          *tif;

        a_out = 0;
        access[0] = '\0';
        for( i = 0; mode[i] != '\0'; i++ )
        {
            if( mode[i] == 'r'
                || mode[i] == 'w'
                || mode[i] == '+'
                || mode[i] == 'a' )
            {
                access[a_out++] = mode[i];
                access[a_out] = '\0';
            }
        }

        strcat( access, "b" );
                    
        fp = VSIFOpenL( name, access );
	if (fp == NULL) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}

	tif = TIFFClientOpen(name, mode,
	    (thandle_t) fp,
	    _tiffReadProc, _tiffWriteProc,
	    _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
	    _tiffMapProc, _tiffUnmapProc);

        tif->tif_fd = 0;
        
	return tif;
}

void*
_TIFFmalloc(tsize_t s)
{
    return VSIMalloc((size_t) s);
}

void
_TIFFfree(tdata_t p)
{
    VSIFree( p );
}

void*
_TIFFrealloc(tdata_t p, tsize_t s)
{
    return VSIRealloc( p, s );
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

static void
unixWarningHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFwarningHandler = unixWarningHandler;

static void
unixErrorHandler(const char* module, const char* fmt, va_list ap)
{
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
}
TIFFErrorHandler _TIFFerrorHandler = unixErrorHandler;
