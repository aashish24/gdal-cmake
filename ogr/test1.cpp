/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Working test harnass.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * Revision 1.1  1999/03/29 21:21:10  warmerda
 * New
 *
 */

#include "ogr_geometry.h"
#include <stdio.h>
#include <stdlib.h>

static void ReportBin( const char * );
static void CreateBin( OGRGeometry *, const char * );

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc, char ** papszArgv )

{
    if( nArgc < 3 )
    {
        printf( "Usage: test1 -reportbin bin_file\n" );
        printf( "    or test1 -createbin bin_file {point,line}\n" );
        exit( 1 );
    }

    if( strcmp( papszArgv[1], "-reportbin" ) == 0 )
    {
        ReportBin( papszArgv[2] );
    }
    else if( strcmp( papszArgv[1], "-createbin" ) == 0 )
    {
        if( strcmp( papszArgv[3], "point") == 0 )
        {
            OGRPoint	oPoint( 100, 200 );
            
            CreateBin( &oPoint, papszArgv[2] );
        }
        else if( strcmp( papszArgv[3], "line") == 0 )
        {
            OGRLineString	oLine;

            oLine.addPoint( 200, 300 );
            oLine.addPoint( 300, 400 );
            oLine.addPoint( 0, 0 );
            
            CreateBin( &oLine, papszArgv[2] );
        }
    }
}

/************************************************************************/
/*                             ReportBin()                              */
/*                                                                      */
/*      Read a WKB file into a geometry, and dump in human readable     */
/*      format.                                                         */
/************************************************************************/

void ReportBin( const char * pszFilename )

{
    FILE	*fp;
    long	length;
    unsigned char * pabyData;
    OGRGeometry *poGeom;
    OGRErr	eErr;
    
/* -------------------------------------------------------------------- */
/*      Open source file.                                               */
/* -------------------------------------------------------------------- */
    fp = fopen( pszFilename, "rb" );
    if( fp == NULL )
    {
        perror( "fopen" );

        fprintf( stderr, "Failed to open `%s'\n", pszFilename );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Get length.                                                     */
/* -------------------------------------------------------------------- */
    fseek( fp, 0, SEEK_END );
    length = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    
/* -------------------------------------------------------------------- */
/*      Read into a block of memory.                                    */
/* -------------------------------------------------------------------- */
    pabyData = (unsigned char *) OGRMalloc( length );
    if( pabyData == NULL )
    {
        fclose( fp );
        fprintf( stderr, "Failed to allocate %ld bytes\n", length );
        return;
    }

    fread( pabyData, length, 1, fp );

/* -------------------------------------------------------------------- */
/*      Instantiate a geometry from this data.                          */
/* -------------------------------------------------------------------- */
    poGeom = NULL;
    eErr = OGRGeometryFactory::createFromWkb( pabyData, &poGeom, length );

    if( eErr == OGRERR_NONE )
    {
        poGeom->dumpReadable( stdout );
    }

/* -------------------------------------------------------------------- */
/*      cleanup                                                         */
/* -------------------------------------------------------------------- */
    OGRFree( pabyData );
    
    delete poGeom;
    fclose( fp );
}

/************************************************************************/
/*                             CreateBin()                              */
/*                                                                      */
/*      Create a binary file representation from a given geometry       */
/*      object.                                                         */
/************************************************************************/

void CreateBin( OGRGeometry * poGeom, const char * pszFilename )

{
    FILE	*fp;
    unsigned char *pabyData;
    long	nWkbSize;

/* -------------------------------------------------------------------- */
/*      Translate geometry into a binary representation.                */
/* -------------------------------------------------------------------- */
    nWkbSize = poGeom->WkbSize();
    
    pabyData = (unsigned char *) OGRMalloc(nWkbSize);
    if( pabyData == NULL )
    {
        fprintf( stderr, "Unable to allocate %ld bytes.\n",
                 nWkbSize );
        return;
    }

    poGeom->exportToWkb( wkbNDR, pabyData );

/* -------------------------------------------------------------------- */
/*      Open output file.                                               */
/* -------------------------------------------------------------------- */
    fp = fopen( pszFilename, "wb" );
    if( fp == NULL )
    {
        OGRFree( pabyData );
        perror( "fopen" );
        fprintf( stderr, "Can't create file `%s'\n", pszFilename );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Write data and cleanup                                          */
/* -------------------------------------------------------------------- */
    fwrite( pabyData, nWkbSize, 1, fp );

    fclose( fp );

    OGRFree( pabyData );
}
